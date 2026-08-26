#include "AptScriptFunction.h"
#include "AptFrameStack.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "_Apt.h"
#include "AptObject/AptGlobalExtensionObject.h"
#include "AptObject/AptGlobalObject.h"
#include "AptObject/AptNativeFunction.h"
#include "MainInline.h"
#if defined(APT_DEBUGGER_ENABLE)
#include "AptDebugger/AptDebugger.h"
#endif


AptValue **AptScriptFunctionBase::spRegBlockBase             = NULL;
AptValue **AptScriptFunctionBase::spRegBlockCurrentFrameBase = NULL;
int AptScriptFunctionBase::snRegBlockCurrentFrameCount       = 0;
int AptScriptFunctionBase::snRegisterBlockSize               = 0;
AptFrameStack *AptScriptFunctionBase::spFrameStack           = NULL;
const int AptScriptFunctionBase::MAX_REGISTERS_IN_FUNCTION   = 256; // Defined by the Flash format spec.

/** Must be called before any ActionScript can run. */
void AptScriptFunctionBase::InitializeStaticData(const AptInitParams &pInitParams)
{
    snRegisterBlockSize = pInitParams.iRegArraySize;
    spRegBlockBase      = APT_MALLOC_ARRAY(AptValue *, snRegisterBlockSize);

    spRegBlockCurrentFrameBase = spRegBlockBase;

    for (int i = 0; i < snRegisterBlockSize; i++)
    {
        spRegBlockBase[i] = gpUndefinedValue;
    }

    snRegBlockCurrentFrameCount = 0;
}

/** Deletes static data created in InitializeStaticData(); asserts all functions were cleaned up properly. */
void AptScriptFunctionBase::ShutdownStaticData()
{
    APT_ASSERT(spRegBlockCurrentFrameBase == spRegBlockBase);
    APT_ASSERT(snRegBlockCurrentFrameCount == 0);

#if defined(APT_DEBUG)
    for (int i = 0; i < snRegisterBlockSize; i++)
    {
        APT_ASSERT(spRegBlockBase[i] == gpUndefinedValue);
    }
#endif
    APT_FREE_ARRAY(spRegBlockBase, AptValue *, snRegisterBlockSize);

    spRegBlockBase             = NULL;
    spRegBlockCurrentFrameBase = NULL;
}

/**
 * Must be called by an entry point (action, event, etc.) before starting a new ActionScript
 * sequence -- not needed when recursing into ActionScript functions internally.
 * @return an opaque value to pass to PopStaticData() when the sequence finishes.
 */
void *AptScriptFunctionBase::PushStaticData()
{
    APT_ASSERT(spRegBlockBase);
    APT_ASSERT(spRegBlockCurrentFrameBase);
    AptValue **pSaveBase = spRegBlockCurrentFrameBase;
    spRegBlockCurrentFrameBase += snRegBlockCurrentFrameCount;
    snRegBlockCurrentFrameCount = 0;
    return (void *)pSaveBase;
}

/**
 * Must be called by an entry point after finishing a new ActionScript sequence -- not needed when
 * recursing into ActionScript functions internally.
 * @param pPushValue the value returned from PushStaticData()
 */
void AptScriptFunctionBase::PopStaticData(void *pPushValue)
{
    APT_ASSERT(spRegBlockBase);
    AptValue **pSaveBase = (AptValue **)pPushValue;

    APT_ASSERT(pSaveBase >= spRegBlockBase && pSaveBase <= spRegBlockCurrentFrameBase);

    for (int i = 0; i < snRegBlockCurrentFrameCount; i++)
    {
        AptValue *pValue              = spRegBlockCurrentFrameBase[i];
        spRegBlockCurrentFrameBase[i] = gpUndefinedValue;
        APT_DEC(pValue);
    }

    snRegBlockCurrentFrameCount = spRegBlockCurrentFrameBase - pSaveBase;
    spRegBlockCurrentFrameBase  = pSaveBase;
}

/**
 * @param pCreatorFunction the function this one is defined from
 * @param pCurCIH the CIH this function is tied to
 */
AptScriptFunctionBase::AptScriptFunctionBase(AptVirtualFunctionTable_Indices eType, AptScriptFunctionBase *pCreatorFunction, AptCIH *pCurCIH, bool bNeedsPrototype) : AptObject(eType),
                                                                                                                                                                      mpCIH(pCurCIH),
                                                                                                                                                                      mpParentAnim(NULL),
                                                                                                                                                                      mpCreatorScope(NULL),
                                                                                                                                                                      mnFrameStackReserve(0)
{
    APT_ASSERT(spRegBlockBase);

    if (pCreatorFunction)
    {
        pCreatorFunction->CreatingNestedFunction();
        mpCreatorScope = spFrameStack;
        APT_INCSAFE(mpCreatorScope);
    }

    if (pCurCIH->isCIH() && !gAptActionInterpreter.bRunningInitActions)
    {
        // We have a calling movie clip and aren't running init actions: associate to it.
        mpParentAnim = mpCIH->GetRootAnimation();
    }
    else
    {
        // No calling movie clip, or running init actions: associate to global space.
        mpParentAnim = _AptGetAnimationAtLevel(0);
    }

    APT_INCSAFE(mpCIH);
    // mpParentAnim can be null when associating to global space instead of a movie clip, avoiding a zombie.
    if (mpParentAnim)
    {
        APT_INC(mpParentAnim);
        mpParentAnim->IncZombieCount();
    }

    // Every script function is an instance of the built-in Function class.
    Set__Proto__(gpFunctionPrototype);

    // Every function has its own unique prototype -- true script functions only.
    if (bNeedsPrototype)
    {
        AptPrototype *pConstructorPrototype = new AptPrototype();
        mNativeHash.SetPrototype(pConstructorPrototype);
        pConstructorPrototype->GetNativeHashVirtual()->Set__Proto__(gpObjectPrototype);
    }
}

/** Similar to a copy constructor: duplicates the info that should carry over from pOrigFunc. */
AptScriptFunctionBase::AptScriptFunctionBase(AptVirtualFunctionTable_Indices eType, AptScriptFunctionBase *pOrigFunc, AptCIH *pCurCIH) : AptObject(eType),
                                                                                                                                         mpCIH(pCurCIH),
                                                                                                                                         mpParentAnim(NULL),
                                                                                                                                         mnFrameStackReserve(0)
{
    APT_ASSERT(spRegBlockBase);
    APT_ASSERT(pOrigFunc);

    mpCreatorScope = pOrigFunc->mpCreatorScope;
    APT_INCSAFE(mpCreatorScope);

    if (mpCIH->isCIH() && !gAptActionInterpreter.bRunningInitActions)
    {
        mpParentAnim = mpCIH->GetRootAnimation();
    }
    else
    {
        mpParentAnim = _AptGetAnimationAtLevel(0);
    }

    APT_INCSAFE(mpCIH);
    if (mpParentAnim)
    {
        APT_INC(mpParentAnim);
        mpParentAnim->IncZombieCount();
    }

    // Take the prototype and __proto__ from the original function.
    AptValue *pTemp = pOrigFunc->mNativeHash.GetPrototype();
    mNativeHash.SetPrototype(pTemp);
    pTemp = pOrigFunc->mNativeHash.Get__Proto__();
    mNativeHash.Set__Proto__(pTemp);
}

AptScriptFunctionBase::~AptScriptFunctionBase()
{
    mpCIH          = NULL;
    mpParentAnim   = NULL;
    mpCreatorScope = NULL;
}

/**
 * Creates the frame stack, using mnFrameStackReserve as the reserve size (if set) to reduce hash
 * expansions.
 */
void AptScriptFunctionBase::CreateFrameStack()
{
    if (mnFrameStackReserve)
    {
        spFrameStack = new AptFrameStack(mpCreatorScope, mnFrameStackReserve);
    }
    else
    {
        spFrameStack = new AptFrameStack(mpCreatorScope);
    }
#ifdef APT_DEBUGGER_ENABLE
    AptDebugger::GetInstance()->SetTopFrameStack(spFrameStack);
#endif
    APT_INC(spFrameStack);
}

/** Called when a function is defining another function; creates the frame stack to be its parent. */
void AptScriptFunctionBase::CreatingNestedFunction()
{
    if (spFrameStack == NULL)
    {
        CreateFrameStack();
    }
}

/** @return the name of the class this script function represents. */
const char *AptScriptFunctionBase::GetClassName() const
{
#if defined(APT_RECORD_SCOPE_INFO)
    return mClassName.GetBuffer();
#else
    return 0;
#endif
}

/** Sets the name of the class this script function represents. */
void AptScriptFunctionBase::SetClassName(const char *name)
{
#if defined(APT_RECORD_SCOPE_INFO)
    if (mClassName.IsEmpty() && name && *name)
    {
        mClassName = name;
    }
    AptPrototype *pConstructorPrototype = static_cast<AptPrototype *>(mNativeHash.GetPrototype());
    if (pConstructorPrototype)
        pConstructorPrototype->SetClassName(mClassName.GetBuffer());
#endif
}

/**
 * @param pCreatorFunction the function that called DefineFunction
 * @param _pFunction the DefineFunction structure (from the big file)
 * @param pCurCIH the CIH this function is associated with
 */
AptScriptFunction1::AptScriptFunction1(AptScriptFunctionBase *pCreatorFunction,
                                       const AptAction_DefineFunction *_pFunction,
                                       AptCIH *pCurCIH) : AptScriptFunctionBase(AptVFT_ScriptFunction1, pCreatorFunction, pCurCIH, true),
                                                          mpFunction(_pFunction)
{
}

/** Copy-constructor-like overload. */
AptScriptFunction1::AptScriptFunction1(AptScriptFunction1 *pOrigFunc,
                                       AptCIH *pCurCIH) : AptScriptFunctionBase(AptVFT_ScriptFunction1, pOrigFunc, pCurCIH),
                                                          mpFunction(pOrigFunc->mpFunction)
{
}

/**
 * @param pCreatorFunction the function that called DefineFunction2
 * @param _pFunction the DefineFunction2 structure (from the big file)
 * @param pCurCIH the CIH this function is associated with
 */
AptScriptFunction2::AptScriptFunction2(AptScriptFunctionBase *pCreatorFunction,
                                       const AptAction_DefineFunction2 *_pFunction,
                                       AptCIH *pCurCIH) : AptScriptFunctionBase(AptVFT_ScriptFunction2, pCreatorFunction, pCurCIH, true),
                                                          mpFunction(_pFunction)
{
}

AptScriptFunction2::AptScriptFunction2(AptScriptFunction2 *pOrigFunc,
                                       AptCIH *pCurCIH) : AptScriptFunctionBase(AptVFT_ScriptFunction2, pOrigFunc, pCurCIH),
                                                          mpFunction(pOrigFunc->mpFunction)
{
}

AptScriptFunction1::~AptScriptFunction1()
{
    mpFunction = NULL;
}

AptScriptFunction2::~AptScriptFunction2()
{
    mpFunction = NULL;
}

void AptScriptFunctionBase::PreDestroy()
{
    // mpParentAnim/mpFuncAnim are intentionally not cleared here.
}

void AptScriptFunctionBase::RegisterReferences()
{
    if (APT_REFERENCES_REGISTERED(this))
        return;
    AptObject::RegisterReferences();

    APT_REGISTER_REFERENCE_SAFE(mpCreatorScope, "ParentScope", APT_REFREG_IS_APTVALUE);
    APT_REGISTER_REFERENCE_SAFE(mpCIH, "CIH", APT_REFREG_IS_APTCIH);
    APT_REGISTER_REFERENCE_SAFE(mpParentAnim, "ParentAnim", APT_REFREG_IS_APTCIH);
    return;
}

void AptScriptFunctionBase::DestroyGCPointers()
{
    APT_ASSERT(mpCIH);
    APT_ASSERT(mpParentAnim);

    APT_DECSAFE(mpCreatorScope);
    mpCreatorScope = NULL;
    APT_DECSAFE(mpCIH);
    mpCIH = NULL;
    if (mpParentAnim != NULL)
    {
        mpParentAnim->DecZombieCount();
        APT_DEC(mpParentAnim);
    }
    mpParentAnim = NULL;
    AptObject::DestroyGCPointers();
}

/** Generic hook run before each function call. */
void AptScriptFunctionBase::SetupBeforeExecution(AptScriptFunctionState *pState, AptValue *pContext, AptValue *, AptValue *)
{
    // Swap out the current scope.
    pState->mpFrameStack = spFrameStack;
    spFrameStack         = NULL;
}

/** Generic hook run after each function call. */
void AptScriptFunctionBase::CleanupAfterExecution(AptScriptFunctionState *pState)
{
    // Clean out the frame stack.
    if (spFrameStack)
    {
        mnFrameStackReserve = spFrameStack->GetNativeHashVirtual()->GetHashSize();
        APT_DEC(spFrameStack);
    }
    // Restore the previous scope.
    spFrameStack = pState->mpFrameStack;
}

/**
 * @param nIndex register index
 * @return the register's value (undefined if not set)
 */
AptValue *AptScriptFunctionBase::GetRegisterValue(int nIndex)
{
    APT_ASSERT(nIndex < MAX_REGISTERS_IN_FUNCTION);
    APT_ASSERT(nIndex < (snRegisterBlockSize - (spRegBlockCurrentFrameBase - spRegBlockBase))); // Don't overflow the register array.
    APT_ASSERT(spRegBlockCurrentFrameBase[nIndex]);                                             // Should never be NULL.

    return spRegBlockCurrentFrameBase[nIndex];
}

/** Sets the register at nIndex to pNewValue, incrementing/decrementing ref counts as needed. */
void AptScriptFunctionBase::SetRegisterValue(int nIndex, AptValue *pNewValue)
{
    APT_ASSERT(pNewValue);
    APT_ASSERT(nIndex < MAX_REGISTERS_IN_FUNCTION);
    APT_ASSERT(nIndex < (snRegisterBlockSize - (spRegBlockCurrentFrameBase - spRegBlockBase))); // Don't overflow the register array.

#if defined(APT_FORCE_CRASHES)
    if (nIndex >= (snRegisterBlockSize - (spRegBlockCurrentFrameBase - spRegBlockBase)))
    {
#if _MSC_VER
        __debugbreak();
#elif defined(__clang__) || defined(__GNUC__)
        __builtin_trap();
#else
        bool *willTrashMemory = (bool *)NULL;
        *willTrashMemory      = true;
#endif
    }
#endif

    if ((nIndex + 1) > snRegBlockCurrentFrameCount)
        snRegBlockCurrentFrameCount = nIndex + 1;

    AptValue *pOldValue                = spRegBlockCurrentFrameBase[nIndex];
    spRegBlockCurrentFrameBase[nIndex] = pNewValue;
    APT_INC(pNewValue); // Inc the new value first, in case it's the same as the old at refcount 1.
    APT_DEC(pOldValue);
}

/**
 * DefineFunction2-specific pre-execution setup: DF2 functions can preload values into registers
 * and get their own set of registers at each call level.
 */
void AptScriptFunction2::SetupBeforeExecution(
    AptScriptFunctionState *pState,
    AptValue *pContext,
    AptValue *pOverrideThis,
    AptValue *pOverrideSuper)
{
    AptScriptFunctionBase::SetupBeforeExecution(pState, pContext, pOverrideThis, pOverrideSuper);

    pState->mpRegBlockPreviousFrameBase = spRegBlockCurrentFrameBase;
    spRegBlockCurrentFrameBase += snRegBlockCurrentFrameCount;
    snRegBlockCurrentFrameCount = 0;

#if APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM
    mpRegBlockMyBase = spRegBlockCurrentFrameBase;
#endif

    AptValue *pTemp = NULL;
    int nStartReg   = 1;

    if (mpFunction->getDF2Flag(DF2_PreloadThisFlag))
    {
        if (pOverrideThis)
        {
            // The caller passed a 'this' pointer (the object being created): we must be inside a
            // super constructor. Make sure 'this' is assigned an AptObject, not an AptPrototype
            // (which is what findChild() below would give us).
            pTemp = pOverrideThis;
        }
        else
        {
            pTemp = mpCIH->findChild(StringPool::GetString(SC_this), NULL);
        }
        SetRegisterValue(nStartReg++, pTemp);
    }
    if (mpFunction->getDF2Flag(DF2_PreloadArgumentsFlag))
    {
        // The arguments object isn't implemented (extra object creation/checking cost, and we ask
        // scripts not to rely on it), so preload undefined instead.
        APT_ASSERT(false && "Arguments Array is unsupported in Apt at this time.");
        SetRegisterValue(nStartReg++, gpUndefinedValue);
    }
    if (mpFunction->getDF2Flag(DF2_PreloadSuperFlag))
    {
        if (pOverrideSuper)
        {
            pTemp = pOverrideSuper;
        }
        else
        {
            // Try to get super from pContext first, falling back to mpCIH.
            pTemp = pContext->findChild(StringPool::GetString(SC_super), NULL);
        }

        if (pTemp == NULL || !pTemp->getIsDefined())
        {
            pTemp = mpCIH->findChild(StringPool::GetString(SC_super), NULL);
        }

        SetRegisterValue(nStartReg++, pTemp);
    }

    if (mpFunction->getDF2Flag(DF2_PreloadRootFlag))
    {
        AptNativeString pTmpStr = "_root";
        pTemp                   = mpCIH->findChild(&pTmpStr, NULL);

        SetRegisterValue(nStartReg++, pTemp);
    }

    if (mpFunction->getDF2Flag(DF2_PreloadParentFlag))
    {
        AptNativeString pTmpStr = "_parent";
        pTemp                   = mpCIH->findChild(&pTmpStr, NULL);
        if (pTemp == NULL)
        {
            pTemp = gpUndefinedValue;
        }

        SetRegisterValue(nStartReg++, pTemp);
    }
    if (mpFunction->getDF2Flag(DF2_PreloadGlobalFlag))
    {
        SetRegisterValue(nStartReg++, AptGetLib()->mpGlobalGlobalObject);
    }
}

/** Cleans up the registers used and restores the previous register stack values. */
void AptScriptFunction2::CleanupAfterExecution(AptScriptFunctionState *pState)
{
#if APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM
    APT_ASSERT(mpRegBlockMyBase == spRegBlockCurrentFrameBase);
#endif
    AptScriptFunctionBase::CleanupAfterExecution(pState);

    for (int i = 0; i < snRegBlockCurrentFrameCount; i++)
    {
        AptValue *pValue              = spRegBlockCurrentFrameBase[i];
        spRegBlockCurrentFrameBase[i] = gpUndefinedValue;
        APT_DEC(pValue);
    }

    snRegBlockCurrentFrameCount = spRegBlockCurrentFrameBase - pState->mpRegBlockPreviousFrameBase;
    spRegBlockCurrentFrameBase  = pState->mpRegBlockPreviousFrameBase;
}

/** Creates a script function object for a bytecode block, reusing AptScriptFunctionBase's setup/scoping. */
AptScriptFunctionByteCodeBlock::AptScriptFunctionByteCodeBlock(const uint8_t *pBytecodeBase,
                                                               int blockSize,
                                                               AptConstantPool constantPool,
                                                               const char *pName,
                                                               AptCIH *pCurCIH,
                                                               AptScriptFunctionBase *pCreatorFunction) : AptScriptFunctionBase(AptVFT_ScriptFunctionByteCodeBlock, pCreatorFunction, pCurCIH, false),
                                                                                                          mpByteCodeBase(pBytecodeBase),
                                                                                                          mnByteCodeSize(blockSize),
                                                                                                          mpName(pName),
                                                                                                          mConstantPool(constantPool)
{
}
