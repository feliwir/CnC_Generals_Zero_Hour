/**
 * This file defines various classes related to AptScriptFunction Overview: AptScriptFunctionBase is the base class that implements the common interface and shared functionality of all script functions. There are three implementations of this class: 1. AptScriptFunction1 : The one and only original based off the DefineFuncion structure of the flash players of yore, and still used sporadically by the flash players today. 2. AptScriptFunction2 : The latest and greatest from our friends at Macromedia is the DefineFunction2 object, which has a bunch of super-fine pre-loading options. this is what most functions are in Flash player 6 r65 and up. This helps speed out. 3. AptScriptFunctionByteCodeBlock : My very own creation, used when we don't have a function definition per-se, we just have a block of actionscript that needs to work like a scription function (i.e. it needs local variables and such.) Features provided by the base class include an event structure that provides a flexible approach to function calls that can be extended as needs require with little or no change to the areas that use functions (hopefully).
 */

#pragma once
#include "_Apt.h"
#include "Apt.h"
#include "AptObject/AptObject.h"
#include "AptValue/AptFrameStack.h"
#include "_AptActions.h"
#include "_AptValue.h"

using AptScriptFunctionState = struct _AptScriptFunctionState
{

  protected:
    AptFrameStack *mpFrameStack;            // Saving this state allows recursion.
    AptValue **mpRegBlockPreviousFrameBase; // Storing this here makes it not need to be a member of AptScriptFunctionBase
    friend class AptScriptFunctionBase;     // AptScriptFunctions can access these members.
    friend class AptScriptFunction2;
};

// Changed in 0.18.00
class AptScriptFunctionBase : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    //-------------------------------------------------
    // Public members... eventually we should make these private, but no hurry.
    // AptCharacter *              mpFuncAnim;
    AptCIH *mpCIH;
    AptCIH *mpParentAnim;

    //-------------------------------------------------
    // Pseudo-copy constructor.
    virtual AptScriptFunctionBase *Duplicate(AptCIH *pCurCIH) = 0;

    //-------------------------------------------------
    // members overrided from superclass.
    virtual void RegisterReferences();
    virtual void DestroyGCPointers();
    virtual void PreDestroy();

    //-------------------------------------------------
    // Informational functions.
    virtual const char *GetName() const       = 0;
    virtual uint32_t GetNumArguments()        = 0;
    virtual const uint8_t *GetByteCodeBase()  = 0;
    virtual uint32_t GetByteCodeSize()        = 0;
    virtual AptConstantPool GetConstantPool() = 0;

    virtual const char *GetClassName() const;
    virtual void SetClassName(const char *name);

    virtual const char *GetArgumentName(const int32_t nIndex) const = 0;

    //-------------------------------------------------
    // Event / Process Functions, for implementation specific tasks.
    virtual void SetupBeforeExecution(AptScriptFunctionState *pState, AptValue *pContext, AptValue *pOverrideThis, AptValue *pOverrideSuper); // Needs context for some operations.
    virtual void SetArgument(AptValue *pValue, int nIndex) = 0;
    virtual void CleanupAfterExecution(AptScriptFunctionState *pState);

    //-------------------------------------------------
    // static Event/Process Functions for global setup before / cleanup after execution
    static void InitializeStaticData(const AptInitParams &pInitParams);
    static void ShutdownStaticData();
    static void *PushStaticData();
    static void PopStaticData(void *pPushValue);

    //-------------------------------------------------
    // static register functions. These maintain the register array.
    // (along with the PushStaticData and PopStaticData) AptScriptFunction2 also
    // messes with the stack stuff in SetupBeforeExecution.
    static AptValue *GetRegisterValue(int nIndex);
    static void SetRegisterValue(int nIndex, AptValue *pNewValue);

    //-------------------------------------------------
    // Scoping functions, Inlined for speed. (They are pretty simple)
    APT_INLINE
    bool ExistsInLocalScope(AptNativeString *pVarName)
    {
        if (spFrameStack == NULL)
        {
            // Nothing has been put into this frame yet. Check the parent if we have one.
            if (mpCreatorScope != NULL)
            {
                return mpCreatorScope->ExistsInLocalScope(pVarName);
            }
            return false; // Nothing, we obviously don't have it.
        }

        return spFrameStack->ExistsInLocalScope(pVarName);
    }

    APT_INLINE
    void SetInLocalScope(AptNativeString *pVarName, AptValue *pValue)
    {
        if (!spFrameStack)
        {
            // This must be the first addition, create the FrameStack.
            CreateFrameStack();
        }

        spFrameStack->SetInLocalScope(pVarName, pValue);
    }

    APT_INLINE
    bool SetWhereExistsInScopeChain(AptNativeString *pVarName, AptValue *pValue)
    {
        if (spFrameStack == NULL)
        {
            // Nothing has been put into this frame yet. Check the parent if we have one.
            if (mpCreatorScope != NULL)
            {
                return mpCreatorScope->SetWhereExistsInScopeChain(pVarName, pValue);
            }
            return false;
        }

        return spFrameStack->SetWhereExistsInScopeChain(pVarName, pValue);
    }

    APT_INLINE
    AptValue *GetInScopeChain(AptNativeString *pVarName)
    {
        if (spFrameStack == NULL)
        {
            // Nothing has been put into this frame yet. Check the parent if we have one.
            if (mpCreatorScope != NULL)
            {
                return mpCreatorScope->GetInScopeChain(pVarName);
            }
            return NULL;
        }

        return spFrameStack->GetInScopeChain(pVarName);
    }

    APT_INLINE
    AptFrameStack *GetFrameStack()
    {
        return spFrameStack;
    }

    APT_INLINE
    void SetTopFrameStack(AptFrameStack *frameStack)
    {
        spFrameStack = frameStack;
    }

  protected:
    //-------------------------------------------------
    // Constructors
    AptScriptFunctionBase(AptVirtualFunctionTable_Indices eType, AptScriptFunctionBase *pCreatorFunction, AptCIH *pCurCIH, bool bNeedsPrototype);
    AptScriptFunctionBase(AptVirtualFunctionTable_Indices eType, AptScriptFunctionBase *pOrigFunc, AptCIH *pCurCIH); // Copy constructor

    //-------------------------------------------------
    // This is called to create the frame stack (duh).
    // Right now it uses the mnFrameStackReserve if present as the reserve size to reduce
    // hash expands, but the seize prediction can easily be made to be much smarted if
    // we have the time to spend on it.
    void CreateFrameStack();

    //-------------------------------------------------
    // Event called when a function is definning another function.
    // In this case we have to create our frame stack so that it can be the parent
    // of the created function.
    virtual void CreatingNestedFunction();

    //-------------------------------------------------
    // Protected Members

    AptFrameStack *mpCreatorScope; // This is stored at creation time and passed to created Frame Stacks
    uint16_t mnFrameStackReserve;  // Stores the size of the frame stack on exit. (used for reserve on next call).

#if APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM
    AptValue **mpRegBlockMyBase; // Stores the Register Frame Base Address
#endif

#if defined(APT_RECORD_SCOPE_INFO)
    AptNativeString mClassName;
#endif

    virtual ~AptScriptFunctionBase();

    static AptValue **spRegBlockBase;             // Allocated pointer. used mainly as a reference.
    static int snRegisterBlockSize;               // Total size of the register block. used mainly as a reference.
    static AptValue **spRegBlockCurrentFrameBase; // Current Frame Base pointer.
    static int snRegBlockCurrentFrameCount;       // Current Size of the currect frame. (maintained by SetRegisterValue)

    static AptFrameStack *spFrameStack; // This store the current Local Variable Scope.

    static const int MAX_REGISTERS_IN_FUNCTION; // Created to get rid of the Magic number which is the max # registers.

  private:
    // don't copy
    AptScriptFunctionBase(const AptScriptFunctionBase &);
    AptScriptFunctionBase &operator=(const AptScriptFunctionBase &);
};

// ##############################################################################
//  Implementation of AptScriptFunctionBase API for define function objects.
class AptScriptFunction1 : public AptScriptFunctionBase
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptScriptFunction1(AptScriptFunctionBase *pCreatorFunction,
                       const AptAction_DefineFunction *_pFunction,
                       AptCIH *pCurCIH);

    //-------------------------------------------------
    // informational functions (just get data from the AptAction_DefineFunction.

    virtual const char *GetName() const
    {
        return mpFunction->szName;
    }

    virtual uint32_t GetNumArguments()
    {
        return mpFunction->nParams;
    }

    virtual const uint8_t *GetByteCodeBase()
    {
        return (const uint8_t *)(mpFunction + 1); // Bytecodes start immediately after the function definition.
    }

    virtual uint32_t GetByteCodeSize()
    {
        return mpFunction->nCodeSize;
    }

    virtual AptConstantPool GetConstantPool()
    {
        return mpFunction->constantPool;
    }

    virtual const char *GetArgumentName(const int32_t nIndex) const
    {
        APT_ASSERT(nIndex < mpFunction->nParams);
        return mpFunction->aszParams[nIndex];
    }

    //-------------------------------------------------
    // Event overrides

    // no pre-loading, just toss the parameters in the local Scope.
    virtual void SetArgument(AptValue *pValue, int nIndex)
    {
        if (!spFrameStack)
        {
            CreateFrameStack();
        }
        AptNativeString strParam(mpFunction->aszParams[nIndex]);
        spFrameStack->SetInLocalScope(&strParam, pValue);
    }

    //-------------------------------------------------
    // pseudo copy constructor.
    virtual AptScriptFunctionBase *Duplicate(AptCIH *pCurCIH)
    {
        return new AptScriptFunction1(this, pCurCIH);
    }

  protected:
    //-------------------------------------------------
    // protected constructor, this copies the info from the passed.
    // use Duplicate to access.
    AptScriptFunction1(AptScriptFunction1 *pOrigFunc,
                       AptCIH *pCurCIH);

    virtual ~AptScriptFunction1();

    //-------------------------------------------------
    // pointer to the Function definition from the apt file.
    const AptAction_DefineFunction *mpFunction;
};

// ##############################################################################
//  Implementation of AptScriptFunctionBase API for define function 2 objects.
class AptScriptFunction2 : public AptScriptFunctionBase
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptScriptFunction2(AptScriptFunctionBase *pCreatorFunction,
                       const AptAction_DefineFunction2 *_pFunction,
                       AptCIH *pCurCIH);

    //-------------------------------------------------
    // informational functions (just get data from the AptAction_DefineFunction2.

    virtual const char *GetName() const
    {
        return mpFunction->szName;
    }

    virtual uint32_t GetNumArguments()
    {
        return mpFunction->nParams;
    }

    virtual const uint8_t *GetByteCodeBase()
    {
        return (const uint8_t *)(mpFunction + 1); // Bytecodes start immediately after the function definition.
    }

    virtual uint32_t GetByteCodeSize()
    {
        return mpFunction->nCodeSize;
    }

    virtual AptConstantPool GetConstantPool()
    {
        return mpFunction->constantPool;
    }

    virtual const char *GetArgumentName(const int32_t nIndex) const
    {
        APT_ASSERT(nIndex < mpFunction->nParams);
        return mpFunction->aszParams[nIndex].szParamName;
    }

    //-------------------------------------------------
    // Event overrides

    // pre-loading arguments as instructed. Others goto local scope.
    virtual void SetArgument(AptValue *pValue, int nIndex)
    {
        if (mpFunction->aszParams[nIndex].nRegister != 0)
        {
            SetRegisterValue(mpFunction->aszParams[nIndex].nRegister, pValue);
        }
        else
        {
            if (!spFrameStack)
            {
                CreateFrameStack();
            }

            AptNativeString sName(mpFunction->aszParams[nIndex].szParamName);
            spFrameStack->SetInLocalScope(&sName, pValue);
        }
    }

    // Grabs a new register frame.
    virtual void SetupBeforeExecution(AptScriptFunctionState *pState, AptValue *pContext, AptValue *pOverrideThis, AptValue *pOverrideSuper);
    // returns previous register frame.
    virtual void CleanupAfterExecution(AptScriptFunctionState *pState);

    //-------------------------------------------------
    // pseudo copy constructor.
    virtual AptScriptFunctionBase *Duplicate(AptCIH *pCurCIH)
    {
        return new AptScriptFunction2(this, pCurCIH);
    }

  protected:
    //-------------------------------------------------
    // protected constructor, this copies the info from the passed.
    // use Duplicate to access.
    AptScriptFunction2(AptScriptFunction2 *pOrigFunc,
                       AptCIH *pCurCIH);

    virtual ~AptScriptFunction2();

    //-------------------------------------------------
    // pointer to the Function definition from the apt file.
    const AptAction_DefineFunction2 *mpFunction;
};

// ##############################################################################
//  Implementation of AptScriptFunctionBase API for generic byte code blocks.
class AptScriptFunctionByteCodeBlock : public AptScriptFunctionBase
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptScriptFunctionByteCodeBlock(const uint8_t *pBytecodeBase,
                                   int blockSize,
                                   AptConstantPool constantPool,
                                   const char *pName,
                                   AptCIH *pCurCIH,
                                   AptScriptFunctionBase *pCreatorFunction = NULL);

    //-------------------------------------------------
    // informational functions (just get data obtained in the constructor.
    virtual const char *GetName() const
    {
        return mpName;
    }

    virtual uint32_t GetNumArguments()
    {
        return 0;
    }

    virtual const uint8_t *GetByteCodeBase()
    {
        return mpByteCodeBase;
    }

    virtual uint32_t GetByteCodeSize()
    {
        return mnByteCodeSize;
    }

    virtual AptConstantPool GetConstantPool()
    {
        return mConstantPool;
    }

    virtual const char *GetArgumentName(const int32_t nIndex) const
    {
        return NULL;
    }

    //-------------------------------------------------
    // These should not have arguments, although if needed it would be easy to do
    virtual void SetArgument(AptValue *pValue, int nIndex)
    {
        APT_ASSERT(false);
        return;
    }

    //-------------------------------------------------
    // This should not be duplicated. The function cannot be accessed by script code directly anyway.
    virtual AptScriptFunctionBase *Duplicate(AptCIH *pCurCIH)
    {
        APT_ASSERT(false);
        return NULL;
    }

  protected:
    //-------------------------------------------------
    // Information stored by the constructor.
    // Note that none of these are garbage collected, the base class takes care of the
    // only GC'd objects.
    const uint8_t *mpByteCodeBase;
    const int mnByteCodeSize;
    const char *mpName;
    AptConstantPool mConstantPool;

    virtual ~AptScriptFunctionByteCodeBlock()
    {
        // Nothing to Do...
    }
};
