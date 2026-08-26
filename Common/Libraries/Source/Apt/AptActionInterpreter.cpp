#include "_Apt.h"
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptCharacterInst.h"
#include "AptDebugHelper.h"
#include "AptFrameStack.h"
#include "Display/AptRenderingContext.h"
#include "Display/AptDisplayListState.h"
#include "AptGlobal.h"
#include "AptValue/AptValueVector.h"
#include "AptObject/AptStringObject.h"
#include "AptObject/AptDate.h"
#include "AptObject/AptMathObj.h"
#include "AptObject/AptKey.h"
#include "AptObject/AptLoadVars.h"
#include "AptObject/AptSound.h"
#include "AptObject/AptTextFormat.h"
#include "AptObject/AptError.h"
#include "AptObject/AptExternalFunction.h"
#include "AptObject/AptStage.h"
#include "AptObject/AptScriptColour.h"
#include "AptObject/AptMovieClip.h"
#include "AptObject/AptMovieClipLoader.h"
#include "AptObject/AptNativeFunction.h"
#include "AptObject/AptXml.h"
#include "AptObject/AptUtil.h"
#include "AptObject/AptGlobalObject.h"
#include "AptExtObject.h"
#include "AptCallStack.h"
// #include "AptDebugger/AptDebugger.h"


#include "MainInline.h"

// lint -dAPT_GET_SET_MEMBER_CHECK=0

#if !defined(APT_ENABLE_INLINE)
#include "AptActionInterpreter.inl"
#include "Apt.inl"
#endif



#define _TONEXTALIGNED(_)                                                                                     \
    {                                                                                                         \
        _ = (unsigned char *)((((uintptr_t)_) + (APT_PLATFORM_PTR_SIZE - 1)) & ~(APT_PLATFORM_PTR_SIZE - 1)); \
    }

const char *FSCOMMAND = "FSCommand:";


// #define APT_AS_DEBUGGING    1         // turn this on to have some extra AS debugging functionality. Search below for this #define.

static AptCallStack sAptOptCallStack;
AptCallStack *gAptOptCallStack = &sAptOptCallStack;

#ifdef EA_SHIP
bool gAptOptCallStackGetScopeInfo = false;
#else
bool gAptOptCallStackGetScopeInfo = true;
#endif // EA_SHIP

bool AptActionInterpreter::ENABLE_AGGRESIVE_ZOMBIE_CLEANUP = false;

static bool _isNaN(AptValue *pValue)
{
    if (pValue->isInteger() || pValue->isFloat())
    {
        return false; // Is a number
    }
    if (pValue->isUndefined() || pValue->isNone()) // undefined is a number according to Flash
    {
        //(1.1:rrv:28/28): AS2 begin
        return (AptGetSwfVersion() >= 7); // Curious. It seems that from version 7 undefined is not a number
    }

    if (!pValue->isString())
    {
        return true; // Not a number
    }

    // String case. Let's find if its content is a number
    AptNativeString szBuf;
    pValue->toString(szBuf);

    int n = szBuf.Size();
    if (n == 0)
    {
        return true; // Empty string: not a number
    }

    uint8_t c = static_cast<uint8_t>(szBuf[0]);

    if (n > 2 && c == '0' && (szBuf[1] == 'x' || szBuf[1] == 'X'))
    {
        // Hexadecimal case
        char *pBuf = NULL;
        strtol(szBuf.c_str(), &pBuf, 16);
        return (*pBuf != 0); // If pBuf is void, then we know we have a good int val;
    }

    bool bFloat     = (c == '.'); // Only one dot is allowed
    bool bHasDigits = isdigit((int)c) != 0;

    if (c != '.' && c != '-' && c != '+' && !bHasDigits)
    {
        return true; // Not a number: the first character is not valid
    }

    bool bExponential = false;

    for (int i = 1; i < n; ++i)
    {
        c = static_cast<uint8_t>(szBuf[i]);
        if (c == '.')
        {
            if (bFloat || bExponential)
            {
                return true; // No a number: more than one dot or a dot after the e.
            }
            bFloat = true; // Only one dot is allowed
        }
        else if (c == 'e' || c == 'E')
        {
            i++;
            if (bExponential || !bHasDigits)
            {
                return true; // Not a number: more than one e or no digits before the e
            }
            if (i < n)
            {
                c = static_cast<uint8_t>(szBuf[i]);
                if (c != '-' && c != '+' && !isdigit((int)c))
                {
                    return true; // Not a number: the first character after the e is not valid
                }
            }
            bExponential = true; // Only one e is allowed
        }
        else
        {
            if (!isdigit((int)c))
            {
                return true;
            }
            bHasDigits = true;
        }
    }
    return bHasDigits ? false : true; // If there is at least one digit, the string contains a valid number
}

AptActionInterpreter gAptActionInterpreter;

static StringCode gaszPropertyNames[22] =
    {
        SC__x,
        SC__y,
        SC__xscale,
        SC__yscale,
        SC__currentframe,
        SC__totalframes,
        SC__alpha,
        SC__visible,
        SC__width,
        SC__height,
        SC__rotation,
        SC__target,
        SC__framesloaded,
        SC__name,
        SC__droptarget,
        SC__url,
        SC__highquality,
        SC__focusrect,
        SC__soundbuftime,
        SC__quality,
        SC__xmouse,
        SC__ymouse,
};

void AptActionInterpreter::initialize(const AptInitParams &aptInitParms)
{
    gAptOptCallStack->Resize(aptInitParms.iCallStackDepth);

    stack.Init(aptInitParms.iStackSize);

    withStack.init(aptInitParms.iCallStackDepth);
    setTargetStack.init(aptInitParms.iCallStackDepth);
    thisStack.init(aptInitParms.iCallStackDepth);
    createdObjectsStack.init(aptInitParms.iCallStackDepth);

    bShutDown            = false;
    mnStackFrameBase     = 0;
    mbSkipTraceBytecodes = aptInitParms.bSkipTraceBytecodes;
    mnActiveIntervals    = 0;

#if defined(APT_DEBUG)
    debugCallStack.Init(aptInitParms.iCallStackDepth); // We can simply use this size since it should not get any larger
#endif


    // Added generic initialize functions and such for Scriptfunction cleanup / scoping fixes.
    AptScriptFunctionBase::InitializeStaticData(aptInitParms);
    AptMovieClipLoader::Initialize(aptInitParms);
}

void AptActionInterpreter::shutdown(void)
{
    stack.Shutdown();
    withStack.shutdown();
    setTargetStack.shutdown();
    thisStack.shutdown();
    createdObjectsStack.shutdown();
    // Added generic initialize functions and such for Scriptfunction cleanup / scoping fixes.
    AptScriptFunctionBase::ShutdownStaticData();

#if defined(APT_DEBUG)
    debugCallStack.Shutdown();
#endif


    APT_ASSERT(mnStackFrameBase == 0);

    gAptOptCallStack->Resize(0);
}

void AptActionInterpreter::stackPushIndirect(AptValue *const pValue)
{
    APT_ASSERT(pValue);
    AptValue *pPushValue;

    if (pValue->isLookup())
    {
        pPushValue = constantPool.apItems[pValue->c_lookup()->nLookup];
    }
    else if (pValue->isRegister())
    {
        int iRegNum = pValue->c_register()->nVal;
        APT_ASSERT(iRegNum >= 0);
        pPushValue = AptScriptFunctionBase::GetRegisterValue(iRegNum);
    }
    else
    {
        pPushValue = pValue;
    }

    stack.Push(pPushValue);
}

void AptActionInterpreter::_parseStream(unsigned char *aActionStream, unsigned char *pBase, AptConstFile *aConstantFile, intptr_t *pnCurrentConstantIndex)
{
    bool bUnresolve             = (aConstantFile == 0);
    unsigned char *pInstruction = aActionStream;

    if (!bUnresolve)
    {
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }

    for (;;)
    {
        Actions eAction = static_cast<Actions>(*pInstruction++);
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        APT_ASSERT(sGlobalTable[eAction].mCheckAlignment == eAction);
#endif

        if (eAction == AptActionEnd)
            break;

        switch (eAction)
        {
        case AptActionDictCallFuncPop:
        case AptActionDictCallFuncSetVar:
        case AptActionDictCallMethodPop:
        case AptActionDictCallMethodSetVar:
        case AptActionPushByte:
        case AptActionStringDictByteGetVar:
        case AptActionStringDictByteGetMember:
        case AptActionPushStringDictByte:
        case AptActionPushRegister:
        {
            pInstruction++;
            break;
        }

        case AptActionPushWord:
        case AptActionPushStringDictWord:
        {
            pInstruction += 2;
            break;
        }

        case AptActionPushDWord:
        case AptActionPushFloat:
        case AptActionTraceStart:
        {
            pInstruction += 4;
            break;
        }

        case AptActionPushString:
        case AptActionPushStringGetVar:
        case AptActionPushStringGetMember:
        case AptActionPushStringSetVar:
        case AptActionPushStringSetMember:
        {
            // these opcodes take one byte as parameter so skip that and go to next one.
            _TONEXTALIGNED(pInstruction);
            AptAction_PushString *pData = (AptAction_PushString *)pInstruction;
            pInstruction += sizeof(AptAction_PushString);

            APT_RU(pData->szStringToBePushed);
            break;
        }

        case AptActionPush:
        case AptActionDefineDictionary:
        {
            _TONEXTALIGNED(pInstruction);
            AptAction_Push *pData = (AptAction_Push *)pInstruction;
            pInstruction += sizeof(AptAction_Push);

            if (bUnresolve)
            {
                for (int i = 0; i < pData->items.nItems; i++)
                {
                    AptValue *pValue = pData->items.apItems[i];

                    if (pValue->isString())
                    {
                        StringPool::RemoveFromPool(pValue->c_string());
                    }
                    else
                    {
                        APT_DEC(pValue);
                    }
                    pData->items.apItems[i] = (AptValue *)(*pnCurrentConstantIndex);
                    (*pnCurrentConstantIndex)++;
                }
            }
            else // bUnresolve is false
            {
                APT_RESOLVE(pData->items.apItems);

                for (int i = 0; i < pData->items.nItems; i++)
                {
                    intptr_t nConstTableIndex = (intptr_t)pData->items.apItems[i];
                    APT_ASSERT(nConstTableIndex == *pnCurrentConstantIndex);
                    (*pnCurrentConstantIndex)++;
                    // this constant table deal is kinda crappy. we're depending on the compiler running through
                    // all the constants in the same call order as the resolve is done in, so that when we encounter
                    // a constant we know that it's just the next one in the list. this is done so that we
                    // don't have to have extra storage for the position in the constant file while still being
                    // able to do an unresolve (restoring binary file to exactly the same as it was before the
                    // resolve).

                    AptVirtualFunctionTable_Indices eType = aConstantFile->aConstants[nConstTableIndex].eType;
                    AptValue *pAtom                       = NULL;
                    if (eType == AptVFT_StringValue) // AptVFT_StringObject is not emitted by pipeline
                    {
                        unsigned char *pBase = (unsigned char *)aConstantFile;
                        APT_RESOLVE(aConstantFile->aConstants[nConstTableIndex].szString);

                        pAtom = StringPool::GetFromPool(aConstantFile->aConstants[nConstTableIndex].szString);
                        //                          pAtom->c_string()->cpy(aConstantFile->aConstants[nConstTableIndex].szString);

                        //                            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "String\t%ld\t%s", i, aConstantFile->aConstants[nConstTableIndex].szString);
                        APT_UNRESOLVE(aConstantFile->aConstants[nConstTableIndex].szString); // we won't have the constant file during an unresolve, so fix here
                    }
                    else if (eType == AptVFT_Float)
                    {
                        float f = aConstantFile->aConstants[nConstTableIndex].fFloat;
                        if (f == 0.f)
                        {
                            pAtom = AptInteger::Create(0);
                        }
                        else
                        {
                            pAtom = AptFloat::Create(aConstantFile->aConstants[nConstTableIndex].fFloat);
                        }
                        //                            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Float\t%ld\t%f", i, aConstantFile->aConstants[nConstTableIndex].fFloat);
                    }
                    else if (eType == AptVFT_Integer)
                    {
                        pAtom = AptInteger::Create(aConstantFile->aConstants[nConstTableIndex].nInteger);
                        //                            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Integer\t%ld\t%ld", i, aConstantFile->aConstants[nConstTableIndex].nInteger);
                    }
                    else if (eType == AptVFT_Lookup)
                    {
                        pAtom = AptLookup::Create(aConstantFile->aConstants[nConstTableIndex].nLookup);
                        //                            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Lookup\t%ld\t%ld", i, aConstantFile->aConstants[nConstTableIndex].nLookup);
                    }
                    else if (eType == AptVFT_Boolean)
                    {
                        pAtom = AptBoolean::Create(aConstantFile->aConstants[nConstTableIndex].bBoolean != 0);
                        //                            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Boolean\t%ld\t%ld", i, (int)aConstantFile->aConstants[nConstTableIndex].bBoolean);
                    }
                    else if (eType == AptVFT_Register)
                    {
                        pAtom = AptRegister::Create(aConstantFile->aConstants[nConstTableIndex].nRegister);
                        //                            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Register\t%ld\t%ld", i, aConstantFile->aConstants[nConstTableIndex].nRegister);
                    }
                    else if (eType == AptVFT_None)
                    {
                        pAtom = gpUndefinedValue;
                    }
                    APT_ASSERT(pAtom);
                    pData->items.apItems[i] = pAtom;
                    if (pAtom->isString() == false)
                    {
                        APT_INC(pAtom);
                    }

                    if ((i % 16) == 0)
                    {
                        //  Clean all the const values generated
                        //  This should do nothing (except clean the vector)
                        //  We do this often to avoid to fill significantly the vector when a big dictionary is defined
                        // AptGetLib()->mpValuesToRelease->ReleaseValues();
                        AptGetLib()->mpValuesToRelease->ReleaseValues();
                    }
                }
            }

            if (bUnresolve)
            {
                APT_UNRESOLVE(pData->items.apItems);
            }
            break;
        }

        case AptActionBranchIfTrue:
        case AptActionBranchAlways:
        case AptActionBranchIfFalse:
        {
            _TONEXTALIGNED(pInstruction);
            // AptAction_BranchAddress *pData = (AptAction_BranchAddress *)pInstruction;
            pInstruction += sizeof(AptAction_BranchAddress);
            break;
        }

        case AptActionGetUrl:
        {
            _TONEXTALIGNED(pInstruction);
            AptAction_GetUrl *pData = (AptAction_GetUrl *)pInstruction;
            pInstruction += sizeof(AptAction_GetUrl);

            APT_RU(pData->szUrl);
            APT_RU(pData->szWin);
            break;
        }

        case AptActionGotoFrame:
        {
            _TONEXTALIGNED(pInstruction);
            pInstruction += sizeof(AptAction_GotoFrame);
            break;
        }

        case AptActionGotoFrame2:
        {
            _TONEXTALIGNED(pInstruction);
            pInstruction += sizeof(AptAction_GotoFrame2);
            break;
        }

        case AptActionSetTarget:
        {
            _TONEXTALIGNED(pInstruction);
            AptAction_SetTarget *pData = (AptAction_SetTarget *)pInstruction;
            pInstruction += sizeof(AptAction_SetTarget);

            APT_RU(pData->szTarget);
            break;
        }

        case AptActionGotoLabel:
        {
            _TONEXTALIGNED(pInstruction);
            AptAction_GotoLabel *pData = (AptAction_GotoLabel *)pInstruction;
            pInstruction += sizeof(AptAction_GotoLabel);

            APT_RU(pData->szLabel);
            break;
        }

        case AptActionDefineFunction:
        {
            _TONEXTALIGNED(pInstruction);
            AptAction_DefineFunction *pData = (AptAction_DefineFunction *)pInstruction;
            pInstruction += sizeof(AptAction_DefineFunction);

            //              APT_ASSERT((bUnresolve && pData->pCIH) || (!bUnresolve && pData->pCIH == 0));
            APT_RU(pData->szName);
            if (!bUnresolve)
                APT_RESOLVE(pData->aszParams);
            for (int i = 0; i < pData->nParams; i++)
            {
                APT_RU(pData->aszParams[i]);
            }
            if (bUnresolve)
            {
                APT_UNRESOLVE(pData->aszParams);
                //                  pData->pCIH = 0;
                pData->constantPool.nItems  = 0x98765432;
                pData->constantPool.apItems = (AptValue **)0x12345678;
            }
            break;
        }

        case AptActionStoreRegister:
        {
            _TONEXTALIGNED(pInstruction);
            // AptAction_StoreRegister *pData = (AptAction_StoreRegister *)pInstruction;
            pInstruction += sizeof(AptAction_StoreRegister);
            break;
        }

        case AptActionWith:
        {
            _TONEXTALIGNED(pInstruction);
            AptAction_With *pData = (AptAction_With *)pInstruction;
            pInstruction += sizeof(AptAction_With);

            // this was commented in 0.16.00 as new implementation of with tag in swfc 0.34 is changed
            // APT_RU(pData->pEnd);
            if (bUnresolve)
            {
                // do nothing as not unresolving it does not affect much
                pData->pEnd = (unsigned char *)(pData->pEnd - (intptr_t)pInstruction);
            }
            else
            {
                pData->pEnd = (unsigned char *)(pInstruction + (intptr_t)pData->pEnd);
            }
            break;
        }

        case AptActionDefineFunction2:
        {
            _TONEXTALIGNED(pInstruction);
            AptAction_DefineFunction2 *pData = (AptAction_DefineFunction2 *)pInstruction;
            pInstruction += sizeof(AptAction_DefineFunction2);

            APT_RU(pData->szName);
            if (!bUnresolve)
            {
                APT_RESOLVE(pData->aszParams);
            }
            for (int i = 0; i < pData->nParams; i++)
            {
                APT_RU(pData->aszParams[i].szParamName);
            }
            if (bUnresolve)
            {
                APT_UNRESOLVE(pData->aszParams);
                pData->constantPool.nItems  = 0x98765432;
                pData->constantPool.apItems = (AptValue **)0x12345678;
            }
            break;
        }

        case AptActionTry:
        {
            _TONEXTALIGNED(pInstruction);
            AptAction_TryCatchFinallyBlock *pData = (AptAction_TryCatchFinallyBlock *)pInstruction;

            // Note that we increment pInstruction to the base of the try block (which is directly
            // followed by the catch block, which is directly followed by the finally block, this
            // allows all of the blocks to be parsed without further recursing this function.
            pInstruction += sizeof(AptAction_TryCatchFinallyBlock);

            if (!pData->putCaughtObjectInRegister())
            {
                APT_RU(pData->szCaughtVarName);
            }
            break;
        }

        case AptActionCallFuncAndPop:
        case AptActionCallFunction:
        case AptActionInvalid:
        case AptActionEnd:
        case AptActionNextFrame:
        case AptActionPrevFrame:
        case AptActionPlay:
        case AptActionStop:
        case AptActionToggleQuality:
        case AptActionStopSounds:
        case AptActionAdd:
        case AptActionSubtract:
        case AptActionMultiply:
        case AptActionDivide:
        case AptActionEquals:
        case AptActionLessThan:
        case AptActionAnd:
        case AptActionOr:
        case AptActionNot:
        case AptActionStringEquals:
        case AptActionStringLength:
        case AptActionSubString:
        case AptActionPop:
        case AptActionToInteger:
        case AptActionGetVariable:
        case AptActionSetVariable:
        case AptActionSetTarget2:
        case AptActionStringAdd:
        case AptActionGetProperty:
        case AptActionSetProperty:
        case AptActionCloneSprite:
        case AptActionRemoveSprite:
        case AptActionTrace:
        case AptActionStartDragMovie:
        case AptActionStopDragMovie:
        case AptActionStringLessThan:
        case AptActionRandom:
        case AptActionMBLength:
        case AptActionAsciiToChar:
        case AptActionCharToAscii:
        case AptActionGetTimer:
        case AptActionMBSubString:
        case AptActionMBAsciiToChar:
        case AptActionMBCharToAscii:
        case AptActionDelete:
        case AptActionDelete2:
        case AptActionDefineLocal:
        case AptActionReturn:
        case AptActionModulo:
        case AptActionNewObject:
        case AptActionDefineLocal2:
        case AptActionInitArray:
        case AptActionInitObject:
        case AptActionTypeOf:
        case AptActionTargetPath:
        case AptActionEnumerate:
        case AptActionAdd2:
        case AptActionLessThan2:
        case AptActionEquals2:
        case AptActionToNumber:
        case AptActionToString:
        case AptActionPushDuplicate:
        case AptActionStackSwap:
        case AptActionGetMember:
        case AptActionSetMember:
        case AptActionIncrement:
        case AptActionDecrement:
        case AptActionCallMethod:
        case AptActionNewMethod:
        case AptActionEnumerate2:
        case AptActionPushThis:
        case AptActionPushGlobal:
        case AptActionPush0:
        case AptActionPush1:
        case AptActionCallFuncSetVar:
        case AptActionCallMethodPop:
        case AptActionCallMethodSetVar:
        case AptActionBitAnd:
        case AptActionBitOr:
        case AptActionBitXor:
        case AptActionBitLShift:
        case AptActionBitRShift:
        case AptActionBitURShift:
        case AptActionStrictEquals:
        case AptActionGreater:
        case AptActionPushThisVariable:
        case AptActionPushGlobalVariable:
        case AptActionPushZeroSetVar:
        case AptActionPushTrue:
        case AptActionPushFalse:
        case AptActionPushNULL:
        case AptActionPushUndefined:
        case AptActionWaitForFrame:
        case AptActionGetUrl2:
        case AptActionCallFrame:
        case LastAptAction:
        case AptActionCastOp:
        case AptActionImplementsOp:
        case AptActionThrow:
        case AptActionInstanceOf:
        case AptActionExtends:
        case AptActionBreakpoint:
            break;

#if !defined(APT_USE_SOUND_OBJECT) && (defined(APT_PLATFORM_GAMECUBE) || defined(APT_PLATFORM_PLAYSTATION2))
        case AptActionStopSounds:
            break;
#endif

            // Not including a default case to help with build checking - if someone
            // adds a new bytecode, they will likely want to add a case here, and we
            // hope that a compiler will warn them of that.
        }

        //  Clean all the const values
        // AptGetLib()->mpValuesToRelease->ReleaseValues();
        if (!bUnresolve)
        {
            AptGetLib()->mpValuesToRelease->ReleaseValues();
        }
    }
}

void AptActionInterpreter::unresolveStream(unsigned char *aActionStream, unsigned char *pBase, intptr_t *pnCurrentConstantIndex)
{
    _parseStream(aActionStream, pBase, 0, pnCurrentConstantIndex);
}

void AptActionInterpreter::resolveStream(unsigned char *aActionStream, unsigned char *pBase, AptConstFile *aConstantFile, intptr_t *pnCurrentConstantIndex)
{
    APT_ASSERT(aActionStream);
    APT_ASSERT(pBase);
    APT_ASSERT(aConstantFile);
    APT_ASSERT(pnCurrentConstantIndex);
    if (pnCurrentConstantIndex != NULL)
    {
        _parseStream(aActionStream, pBase, aConstantFile, pnCurrentConstantIndex);
    }
}

void AptActionInterpreter::loadVariables(AptValue *pContext, AptValue *pWith, const AptNativeString *pURL)
{
    // get variables from user

    AptValue *pValue = NULL;
    if (pURL == NULL) // NEWLY ADDED FOR  LoadVars.sendAndLoad
    {
        APT_ASSERT(AptGetUserFuncs().pfnLoadVariablesNULL);
        pValue = AptGetUserFuncs().pfnLoadVariablesNULL();
    }
    else
    {
        pValue = AptGetUserFuncs().pfnLoadVariables(pURL->c_str());
    }

    AptNativeString loadVarsString;
    pValue->Append_ToString(loadVarsString);

    const char *szCur = loadVarsString.c_str();

    AptNativeString sKey;
    AptNativeString sValue;

    // decode variable in to string pairs and set the desired target
    for (;;)
    {
        szCur = urlDecode(szCur, sKey, sValue);
        if (!szCur)
            break;

        if (sKey.IsEmpty())
        {
            APT_ASSERT(pURL != NULL);
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "loadVariable for '%s' returned empty variable name\n", /*lint -e(413) */ pURL->c_str());
            continue; // don't try setting nothing
        }

        // set variables here
        AptString *pVariable = AptString::Create();
        pVariable->str       = sValue;
        setVariable(pContext, pWith, &sKey, pVariable, true);
    }
}

AptValue *AptActionInterpreter::getObject(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pPathName)
{

    // empty target means return current object
    if (pPathName->Size() == 0)
    {
        return pCurrentContext;
    }

    AptNativeString sVar;
    AptValue *pContext;
    getContext(pCurrentContext, pWith, pPathName, &pContext, sVar);
    if (pContext)
    {
        AptValue *pValue = pContext->findChild(&sVar, pWith); // Calls FindChild now, it would miss some cases.
        if (pValue && pValue->ContainsNativeHashVirtual())
        {
            return pValue;
        }
    }
    return NULL;
}

bool AptActionInterpreter::getContext(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pVarName, AptValue **ppContext, AptNativeString &sName)
{
    //  First we test if the string is simple
    //  IE without . / :
    //  ASCII values:
    //      0x24    $
    //      0x2e    .
    //      0x2f    /
    //      0x30    0
    //      0x39    9
    //      0x3a    :
    //      0x41    A
    //      0x5a    Z
    //      0x5f    _
    //      0x61    a
    //      0x7a    z

    if (pWith == NULL) // If pWith is not NULL, the is simple check is not needed
    {
        const char *pBuffer = pVarName->c_str();
        char cCar;
        bool bSimple = true;
        for (;;)
        {
            cCar = *pBuffer++;
            // APT_ASSERT(cCar != '$');        // '$' is acceptable here.
            if (cCar == 0)
            {
                break;
            }
            if (cCar < '0')
            {
                // APT_ASSERT((cCar == '.') || (cCar == '/'));
                bSimple = false;
                break;
            }
            if (cCar == ':')
            {
                bSimple = false;
                break;
            }

            //  All the other cases '0' - '9' 'A' - 'Z' 'a' - 'z'
        }

        if (bSimple)
        {
            sName      = *pVarName;
            *ppContext = pCurrentContext;
            return false;
        }
    }

    //  We can't use the simple optimal code, use the complex parsing
    char szBuf[APT_SCARY_MAX_NATIVE_STRING_LENGTH];
    bool bRet = getContext(pCurrentContext, pWith, pVarName, ppContext, szBuf);
    sName     = szBuf;
    return bRet;
}
bool AptActionInterpreter::getContext(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pVarName, AptValue **ppContext, char *szName)
{
    const char *pCurChar;
    AptValue *pContext = pCurrentContext;
    AptValue *pNewContext;
    bool bId              = false;
    const char *szVarName = pVarName->c_str();

    szName[0] = '\0';
    if (szVarName[0] == '/')
    {
        // get root
        pContext = GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem();

        pCurChar   = &szVarName[1];
        *ppContext = pContext;
        bId        = true;
    }
    else
    {
        *ppContext = pCurrentContext;
        pCurChar   = &szVarName[0];
    }

    char szNextDir[APT_SCARY_MAX_NATIVE_STRING_LENGTH];
    char *pNextDir = szNextDir;
    for (;;)
    {
        switch (*pCurChar)
        {
        case '.':
            if (pCurChar[1] == '.')
            {
                *pNextDir++ = *pCurChar++;
                *pNextDir++ = *pCurChar++;
                continue;
            }
            else if (pCurChar[1] == '\0') // Eval() returns unexpected result
            {
                *ppContext = NULL;
                return (bId);
            }

            // Fall through is intended. (getting passed dir constructs.)
            // case '/': // this was removed since '/' notation isn't used to evaluate the context. // 2/8
            *pNextDir = 0;
            {
                AptNativeString strNextDir(szNextDir);
                pContext = pContext->findChild(&strNextDir, pWith);
            }
            pWith = NULL;
            if (pContext == NULL)
            {
                *ppContext = NULL;
                return (bId);
            }
            pCurChar++;
            pNextDir = szNextDir;
            break;
        case ':':
            // bId = false;
            *pNextDir = 0;
            {
                AptNativeString strNextDir(szNextDir);
                pNewContext = pContext->findChild(&strNextDir, pWith);
            }
            pWith = NULL;
            if (pNewContext)
            {
                *ppContext = pNewContext;
                strcpy(szName, pCurChar + 1);
                return (bId);
            }
            pCurChar++;
            pNextDir = szNextDir;
            break;
        case '\0':
            *pNextDir  = 0;
            *ppContext = pWith ? pWith : pContext;
            strcpy(szName, szNextDir);
            return (bId);
        default:
            *pNextDir++ = *pCurChar++;
            break;
        }
    }
}

bool AptActionInterpreter::setVariable(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pVarName, AptValue *pValue, int bGlobal, int bLookInFunctionScope, int bIsMember)
{
    AptValue *pContext;
    AptNativeString sVar;

    if (bIsMember == false) // 3/8
    {
        getContext(pCurrentContext, pWith, pVarName, &pContext, sVar);
    }
    else
    {
        pContext = pCurrentContext;
        sVar     = *pVarName;
    }

    if (pContext == NULL)
        return false;
    if (pContext->isCIH() && pContext->c_cih()->IsShapeInst())
        return false;

    if (pContext->isUndefined() == false)
    {
        if (pContext->objectMemberSet(pContext, &sVar, pValue))
        {
            return (true);
        }
    }

    if (bGlobal)
    {
        // search for local of specified name, if name doesn't exist locally then set globally
        if (bLookInFunctionScope && mpCurrentFunction)
        {
            if (mpCurrentFunction->SetWhereExistsInScopeChain(&sVar, pValue))
            {
                return (true);
            }
        }

        AptNativeHash *pNativeHash = pContext->GetNativeHashVirtual();
        // APT with() output does not match Flash - 1/2
        // if we are setting some key to null for 'with' object then first check if pWith has that key otherwise we go back to original context
        if (pContext == pWith && pValue == NULL)
        {
            AptValue *pLookupValue = pNativeHash->Lookup(&sVar);
            if (!pLookupValue)
            {
                pNativeHash = pCurrentContext->GetNativeHashVirtual();
            }
        } // end of fix

        if (pNativeHash != NULL)
        {
            pNativeHash->Set(&sVar, pValue);
            pNativeHash->UpdateObjectMethods(pContext, &sVar, (pValue == NULL || pValue->isUndefined()) ? 1 : 0);
            if ((pContext == gpGlobalObjectPrototype) && pValue != NULL && pValue->isScriptFunction())
            {
                pValue->GetNativeHashVirtual()->GetPrototype()->GetNativeHashVirtual()->Set__Proto__(NULL);
            }
        }
        else
        {
            // matching behavior in getVariable
            if (!pContext->isCIH() && (mpCurrentFunction) && !bIsMember) // getVariable scoping problem
            {
                AptNativeHash *pThisNativeHash = mpCurrentFunction->mpParentAnim->GetNativeHashVirtual();
                if (pThisNativeHash != NULL) // Null pointer exception in AptActionInterpreter::getVariable
                {
                    pThisNativeHash->Set(&sVar, pValue);
                }
            }
            else if (pContext->isCIH() && pContext->c_cih()->IsButtonInst())
            {
                APT_ASSERT(false && "SHOULD NOT GET HERE ANYMORE");
                AptNativeHash *pParentNativeHash = pContext->c_cih()->GetDisplayListParent()->GetNativeHash();
                if (pParentNativeHash != NULL)
                {
                    pParentNativeHash->Set(&sVar, pValue);
                }
            }
        }
    }
    else
    {
        //  set local, or global if no local stack exists
        if (mpCurrentFunction)
        {
            mpCurrentFunction->SetInLocalScope(&sVar, pValue);
        }
        else
        {
            AptNativeHash *pNativeHash = pContext->GetNativeHashVirtual();
            if (pNativeHash != NULL)
            {
                pNativeHash->Set(&sVar, pValue);
                pNativeHash->UpdateObjectMethods(pContext, &sVar, (pValue == NULL || pValue->isUndefined()) ? 1 : 0);
            }
        }
    }
    return (true);
}

AptValue *AptActionInterpreter::cbCallMethod_setInterval(AptValue *pContext, int nParams)
{
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    APT_ASSERT(nParams >= 2);
    AptValue *pCBValue       = gAptActionInterpreter.stackAt(0);
    AptValue *pIntervalValue = gAptActionInterpreter.stackAt(1);
    int i;
    int nParamStart = 2; // This value for when parameters are passed into the setInterval function

    if (pCBValue->isUndefined())
    {
#if !defined(DO_COVERAGE)
        APT_DEBUGPRINT(APT_DEBUG_MSG_NORMAL_LVL, "[INTERVAL] Undefined callback specified for setInterval, returning undefined.\n");
#endif
        return gpUndefinedValue;
    }

    // allocate timer

    int32_t nIntervalId = AptIntervalTimer::GenerateId();

    for (i = 0; i < GetTargetSim()->GetAnimationTarget()->GetMaxIntervalTimers(); i++)
    {
        if (!GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].bValid)
        {
            GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].bValid   = true;
            GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].pContext = gpUndefinedValue;

            // added test case to check if object passed was of movieClip type or not.

            // removed dependably on what type of object it was, if it isn't a function we move on.
            //                  also removed duplicate checks and such.
            if (!pCBValue->isFunction()) // This case is for when a function from a object is passed in the setInterval function
            {
                // At this point the pCBValue should be a recognizable object
                APT_ASSERT(pCBValue->isObject() || pCBValue->c_cih()->IsSpriteInst() || pCBValue->c_cih()->IsAnimationInst());
                AptValue *pTemp = pCBValue; // keep copy of the function context

                pIntervalValue           = gAptActionInterpreter.stackAt(2);
                AptString *pStringLevel1 = (AptString *)gAptActionInterpreter.stackAt(1);

                // In any case this will find the function.
                pCBValue = pCBValue->findChild(pStringLevel1->GetInternalString(), NULL); // get function pointer

                APT_ASSERT(pCBValue);

                nParamStart = 3; // reset the param start position on stack since one extra position is the actual function name

                // save the context to pContext
                GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].pContext = pTemp;
            }

            GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].pCBFunction = pCBValue; // need to check if pCBValue is an object.. if it is... then stackAt(2) is the function from the obj to use..
            APT_INC(GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].pCBFunction);
            APT_INC(GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].pContext);
            GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].fInterval   = pIntervalValue->toFloat();
            GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].fCurTime    = GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].fInterval;
            GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].nIntervalId = nIntervalId;

            if (nParams > nParamStart) // check if there are parameters
            {
                int j;
                for (j = 0; j < nParams - nParamStart; j++) // push parameters on stack..
                {
                    GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].pParams.push(gAptActionInterpreter.stackAt(j + nParamStart));
                }
            }
            ++(gAptActionInterpreter.mnActiveIntervals);
            APT_DEBUGPRINT(APT_DEBUG_MSG_NORMAL_LVL, "[INTERVAL] <INFO> Set(%d): There are now %d interval timers active out of %d.\n", nIntervalId, gAptActionInterpreter.mnActiveIntervals, GetTargetSim()->GetAnimationTarget()->GetMaxIntervalTimers());
            break;
        }
    }
    if (i == GetTargetSim()->GetAnimationTarget()->GetMaxIntervalTimers())
    {
        // no free interval timer slot; ignore the setInterval call (increase AptInitParams::iMaxIntervalFunctions to avoid this)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "setInterval: no free interval timer slot\n");
        return gpUndefinedValue;
    }

    return AptInteger::Create(nIntervalId);
}
AptValue *AptActionInterpreter::cbCallMethod_clearInterval(AptValue *pContext, int nParams)
{
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    AptValue *pIndex = gAptActionInterpreter.stackAt(0);

    // Don't let an undefined value cancel timer 0!!!
    if (!pIndex->isUndefined())
    {
        int32_t nIntervalId = pIndex->toInteger();

        for (int i = 0; i < GetTargetSim()->GetAnimationTarget()->GetMaxIntervalTimers(); i++)
        {
            if (GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].bValid)
            {
                if (GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].bValid &&
                    GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].nIntervalId == nIntervalId)
                {
                    --(gAptActionInterpreter.mnActiveIntervals);
                    APT_DEC(GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].pCBFunction);
                    APT_DECSAFE(GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].pContext);
                    GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].bValid = false;
                    GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].CleanParams();
                    GetTargetSim()->GetAnimationTarget()->GetIntervalTimers()[i].nIntervalId = 0;
                    APT_DEBUGPRINT(APT_DEBUG_MSG_NORMAL_LVL, "[INTERVAL] <INFO> Clear(%d): There are now %d interval timers active out of %d.\n", nIntervalId, gAptActionInterpreter.mnActiveIntervals, GetTargetSim()->GetAnimationTarget()->GetMaxIntervalTimers());
                }
            }
        }
    }
    return gpUndefinedValue;
}
AptValue *AptActionInterpreter::cbCallMethod_hitTest(AptCIH *pCIH, int nParams)
{
    APT_UNREF(pCIH);
    APT_UNREF(nParams);
    AptValue *pRet = gpUndefinedValue;

    if (nParams == 1)
    {
        // movie <-> movie hitTest
    }
    else if (nParams > 1)
    {
        // postion <-> movie hitTest
        float fX, fY;
        fX = gAptActionInterpreter.stackAt(0)->toFloat();
        fY = gAptActionInterpreter.stackAt(1)->toFloat();

        // if (nParams > 2)
        //{
        //     bRectHitTest = gAptActionInterpreter.stackAt(2)->toInteger();
        // }

        // do hit test
        // if (bRectHitTest)  // TODO: make non bounding-rect hittest
        {
            AptRect rect;
            pCIH->GetBoundingRect(&rect);

            if (fX >= rect.fLeft && fX <= rect.fRight &&
                fY >= rect.fTop && fY <= rect.fBottom)
            {
                pRet = AptInteger::Create(1);
            }
        }
    }
    return pRet;
}

AptValue *AptActionInterpreter::cbCallMethod_isNaN(AptValue *pContext, int nParams)
{
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    APT_ASSERT(nParams <= 1);

    if (nParams == 0)
        return AptBoolean::Create(true);

    AptValue *pValue = gAptActionInterpreter.stackAt(0);
    return AptBoolean::Create(_isNaN(pValue));
}

void _unEscape(AptNativeString &sEsc);
void _escape(AptNativeString &sEsc);

// added unescape function as a global actionscript function.
AptValue *AptActionInterpreter::cbCallMethod_unescape(AptValue *pContext, int nParams)
{
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    APT_ASSERT(nParams <= 1);

    // create a new string
    AptString *pRetValue = AptString::Create();
    AptValue *pValue     = gAptActionInterpreter.stackAt(0);
    if (pValue->isString())
    {
        AptNativeString szBuf;
        pValue->toString(szBuf);
        _unEscape(szBuf); // this will unescape the string and result will be in szBuf itself

        pRetValue->str = szBuf;
    }
    return pRetValue;
}

// added unescape function as a global actionscript function.
// added escape function as a global actionscript function.
AptValue *AptActionInterpreter::cbCallMethod_escape(AptValue *pContext, int nParams)
{
    // TO be done not yet complete
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    APT_ASSERT(nParams <= 1);

    // create a new string
    AptString *pRetValue = AptString::Create();
    if (nParams == 0)
    {
        return pRetValue;
    }
    AptValue *pValue = gAptActionInterpreter.stackAt(0);
    if (pValue->isString())
    {
        AptNativeString szBuf;
        pValue->toString(szBuf);
        _escape(szBuf); // this will escape the string and result will be in szBuf itself

        pRetValue->str += szBuf;
    }
    return pRetValue;
}

// added Booolean function as a global actionscript function.
AptValue *AptActionInterpreter::cbCallMethod_boolean(AptValue *pContext, int nParams)
{
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    APT_ASSERT(nParams <= 1);

    // create a new string
    if (nParams == 0)
    {
        return gpUndefinedValue;
    }

    AptValue *pValue = gAptActionInterpreter.stackAt(0);
    if (pValue->isCIH() || pValue->isObject())
    {
        // If expression is a movie clip or an object, the return value is true.
        return (AptBoolean::Create(true));
    }
    else if (pValue == gpUndefinedValue)
    {
        // If expression is undefined, the return value is false.
        return (AptBoolean::Create(false));
    }
    else if (!pValue->isFloat() && !pValue->isInteger())
    {
        // 05-08-2006: Flash docs say:
        // "If expression is a number, the return value is true if the
        // number is not 0; otherwise the return value is false."
        if (!_isNaN(pValue)) // Checks for isNaN first
        {
            if (pValue->toFloat())
            {
                return (AptBoolean::Create(true));
            }
        }
        else if (pValue->isBoolean())
        {
            // If expression is a Boolean value, the return value is expression.
            // 05-08-2006: moved this "else if" block here.
            return (AptBoolean::Create(pValue->c_boolean()->GetBool()));
        }
        else if (pValue->isString())
        {
            // 05-08-2006: Flash docs say:
            // "If expression is a string, the result is true if the string has a length greater
            // than 0; the value is false for an empty string."
            if (AptGetSwfVersion() >= 7)
            {
                bool bEmpty = pValue->c_string()->GetInternalString()->IsEmpty();
                return AptBoolean::Create(!bEmpty);
            }
        }
        return (AptBoolean::Create(false));
    }
    else if (pValue->isFloat() || pValue->isInteger())
    {
        // If expression is a number, the return value is true if the number is not zero,
        //  otherwise the return value is false.
        if (pValue->toFloat())
        {
            return (AptBoolean::Create(true));
        }
        else
        {
            return (AptBoolean::Create(false));
        }
    }
    return (AptBoolean::Create(false));
}

AptValue *AptActionInterpreter::cbCallMethod_parseInt(AptValue *pContext, int nParams)
{
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    APT_ASSERT(nParams >= 1);

    int iRadix            = 10; // default value for the radix
    const int RADIX_LOWER = 2;
    const int RADIX_UPPER = 36;

    AptValue *pStringToParse = gAptActionInterpreter.stackAt(0);
    if (!pStringToParse->isString())
    {
        return gpUndefinedValue;
    }

    const char *szString = pStringToParse->c_string()->GetInternalString()->GetBuffer();

    while (*szString == ' ') // discards leading spaces
    {
        szString++;
    }

    int nInteger    = 0;
    bool bHasDigits = false;
    bool bNegative  = false;

    if (*szString == '-')
    {
        bNegative = true;
        szString++;
    }

    if (*szString == '0')
    {
        iRadix = 8;
        szString++;
        bHasDigits = true;
        if (*szString == 'x' || *szString == 'X')
        {
            iRadix = 16;
            szString++;
            if (bNegative)
            {
                // Flash doesn't accept -0x. However, it accepts negative numbers with 16 or other prefix as 2nd parameter.
                return gpUndefinedValue;
            }
        }
    }

    // Ignore the default radix if it comes with a radix parameter
    if (nParams > 1)
    {
        AptValue *pRadix = gAptActionInterpreter.stackAt(1);
        if (pRadix->isInteger())
        {
            iRadix = pRadix->toInteger();
        }
    }
    if ((iRadix <= RADIX_LOWER) || (iRadix >= RADIX_UPPER))
    {
        return gpUndefinedValue;
    }

    while (*szString) // parses the stting until a non numeric character is found
    {
        char c     = *szString++;
        int iDigit = RADIX_UPPER + 1;
        if (c >= '0' && c <= '9')
        {
            iDigit = c - '0';
        }
        else if (c >= 'A' && c <= 'Z')
        {
            iDigit = c - 'A' + 10;
        }
        else if (c >= 'a' && c <= 'z')
        {
            iDigit = c - 'a' + 10;
        }
        if (iDigit >= iRadix) // Stops parsing when it finds the first unvalid character for this conversion
        {
            break;
        }
        bHasDigits = true;
        nInteger   = nInteger * iRadix + iDigit;
    }

    if (!bHasDigits) // The string doesn't start with numeric characters
    {
        return gpUndefinedValue;
    }

    if (bNegative)
    {
        nInteger = -nInteger;
    }

    return AptInteger::Create(nInteger);
}

AptValue *AptActionInterpreter::cbCallMethod_parseFloat(AptValue *pContext, int nParams)
{
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    APT_ASSERT(nParams >= 1);

    AptValue *pStringToParse = gAptActionInterpreter.stackAt(0);
    if (!pStringToParse->isString())
    {
        return gpUndefinedValue;
    }

    float fValue   = 0;
    float fDecimal = 0.1f;
    bool bAfterDot = false;
    bool bValid    = false;
    bool bExponent = false;
    bool bNegative = false;

    const char *szString = pStringToParse->c_string()->GetInternalString()->GetBuffer();

    while (*szString == ' ') // discards leading spaces
    {
        szString++;
    }

    if (*szString == '-')
    {
        bNegative = true;
        szString++;
    }

    while (*szString) // parses the stting until a non numeric character is found
    {
        char c = *szString++;
        if (c >= '0' && c <= '9')
        {
            if (bAfterDot)
            {
                fValue += fDecimal * (c - '0');
                fDecimal *= 0.1f;
            }
            else
            {
                fValue = fValue * 10 + (c - '0');
            }
            bValid = true;
        }
        else if (c == '.')
        {
            if (bAfterDot)
            {
                break; // dot already appeared so a new dot is not part of the number
            }
            bAfterDot = true;
        }
        else
        {
            bExponent = (c == 'e' || c == 'E');
            break; // stops parsing as soon as it finds a non digit character
        }
    }

    if (!bValid) // The string doesn't start with numeric characters
    {
        return gpUndefinedValue;
    }

    if (bExponent)
    {
        float fExponent = 0;
        while (*szString) // discard leading spaces
        {
            char c = *szString++;
            if (c < '0' || c > '9')
            {
                break;
            }
            fExponent = fExponent * 10 + (c - '0');
        }
        fValue = fValue * pow(
#if defined(APT_PLATFORM_XBOX)
                              10.0f,
#else
                              10,
#endif // APT_PLATFORM_XBOX
                              fExponent);
    }

    if (bNegative)
    {
        fValue = -fValue;
    }

    return AptFloat::Create(fValue);
}

AptValue *AptActionInterpreter::cbCallMethod_ASSetPropFlags(AptValue *pContext, int nParams)
{
    APT_UNREF(pContext);
    APT_UNREF(nParams);
    APT_ASSERT(nParams <= 3); // Can Have 3 or 4 params.

    // TODO: not implementing this just yet... it will take up too much memory...
    return gpUndefinedValue;
}

enum NameStringType
{
    NameString_Slash,
    NameString_Dot
};
static void _getNameString(AptCIH *pContext, AptNativeString &sBuf, NameStringType eNameStringType)
{
    APT_ASSERT(pContext->IsSpriteInstBase() || pContext->IsButtonInst() || pContext->IsLevelInst() || pContext->IsDynamicTextInst() || pContext->IsImageInst());
    const char *szSep = (eNameStringType == NameString_Slash ? "/" : ".");

    if (pContext->GetDisplayListParent() == NULL)
    {
        APT_ASSERT(pContext->IsSpriteInstBase() || pContext->IsButtonInst() || pContext->IsLevelInst() || pContext->IsDynamicTextInst() || pContext->IsImageInst());
        if ((eNameStringType == NameString_Slash && pContext->GetDepth() != 0) // ### still here
            || (eNameStringType == NameString_Dot))
        {
            char szTemp[16];

            sprintf(szTemp, "_level%d", pContext->GetDepth());
            sBuf = szTemp;
        }
        // if (pContext->IsAnimationInst())
        //{
        //   // this block added because for AptCharacterAnimationInst has a pFile
        //   // in it which stores name of movie.
        //   // especially when getName gets called from getBytesTotal, getBytesLoaded it should know
        //   // the name of the movieclip even if it is a animation instance.
        //   // Apt callbacks those functions with name of movieclip returned from here.
        //   sBuf += pContext->GetAnimationInst()->pFile->GetName() ;
        // }
        return;
    }
    else
    {
        // get the name of everything up to pContext
        AptCIH *pParent = pContext->GetDisplayListParent();
        _getNameString(pParent, sBuf, eNameStringType);

        if (pContext->GetInstanceName().IsEmpty() == false)
        {
            sBuf += szSep + pContext->GetInstanceName();
        }
        else
        {
            // if there's no name and the name is asked for, flash "instance%d" where %d is .. something. we use the depth (even though that's not what they use)
            int nInstanceNumber = pContext->GetDepth();

            AptNativeString sNewName;
            sBuf += szSep;
            sNewName.Format("instance%ld", nInstanceNumber);
            sBuf += sNewName;

            pContext->SetInstanceName(sNewName);

            pParent->GetCharacterInst()->GetNativeHash()->Set(&sNewName, pContext);
        }
    }
}
/*
void AptActionInterpreter::getName(AptCIH *pCIH, char *szBuf)
{
    OLDSTRINGFUNCTIONPRINT();
    AptNativeString sBuf;
    getName(pCIH, sBuf);
    strcpy(szBuf, sBuf.ConstRawPtr());
}
*/
void AptActionInterpreter::getName2(AptCIH *pCIH, AptNativeString &sBuf)
{
    sBuf = "";
    _getNameString(pCIH, sBuf, NameString_Slash); // ### still here
    if (sBuf.Size() == 0)
    {
        sBuf = "/";
    }
}

void AptActionInterpreter::getName(AptCIH *pCIH, AptNativeString &sBuf)
{
    sBuf = "";
    _getNameString(pCIH, sBuf, NameString_Dot); // ### still here
    APT_ASSERT(sBuf.Size() != 0);
}

AptValue *AptActionInterpreter::_doCloneSprite(AptCIH *pCurrentCIH, AptValue *pWith, AptValue *pSource,
                                               AptValue *pTarget, int nDepthInt, AptValue *pInitObject)
{
    // pInitObject is defualted to NULL.
    AptValue *pSourceContext = NULL;
    valueToObject(pCurrentCIH, pWith, pSource, &pSourceContext);
    TO_STRING(pTarget, psBuf);

    if (pSourceContext)
    {
        AptCIH *pSourceCIH = pSourceContext->c_cih();
        if (!pSourceCIH->GetDisplayListParent())
        {
            // if parent is not present just return.
            return gpUndefinedValue;
        }

        // AptCIH *pInserted = pSourceCIH->GetDisplayListParent()->InsertChild(pSourceCIH->GetCharacterInst()->GetCharacterWritable(), nDepthInt, psBuf, pInitObject);
        AptCIH *pInserted = pSourceCIH->GetDisplayListParent()->InsertChild(pSourceCIH, pSourceCIH->GetCharacterInst()->GetCharacterWritable(), nDepthInt, const_cast<AptNativeString *>(psBuf), pInitObject);

        // Copies all state over.
        pInserted->GetCharacterInst()->CopyRenderDataFrom(pSourceCIH->GetCharacterInst());
        // this code was more or less copied from AptDisplayList.instantiateCharacter.
        // It's useful and would probably do well refactored into its own function somewhere.
        // There's an inefficiency here, as this process is repeated from the call to AptDisplayList.instantiateCharacter
        // in the..->InsertChild() above, but the matrix (and other data) is overwritten in..->CopyRenderDataFrom(..)
        if (pInitObject && pInserted->ContainsNativeHashVirtual())
        {
            AptNativeHash *pObjHash = pInitObject->GetNativeHashVirtual();
            if (pObjHash)
            {
                for (AptHashItem *pInitItem = pObjHash->GetFirstItem(); pInitItem; pInitItem = pObjHash->GetNextItem(pInitItem))
                {
                    // TODO: theoretically, on every hash key there's supposed to be a DontEnum/DontDelete flag; we don't have that so hardcode some dontenum's here..
                    if ((pInitItem->Key.Equal(*StringPool::GetString(SC___proto__))) ||
                        (pInitItem->Key.Equal(*StringPool::GetString(SC_prototype))))
                    {
                        continue;
                    }
                    gAptActionInterpreter.setVariable(pInserted, NULL, &pInitItem->Key, pInitItem->mValue, true);
                }
            }
        }

        GetTargetSim()->GetAnimationTarget()->TickNewInsts();

        return (pInserted);
    }
    return gpUndefinedValue;
}

#if defined APT_AS_DEBUGGING
int32_t gnAptCompareDebugString = 0; // set it to 1 to check for debug variable.
char gszAptGetVarCompareStr[256];    // just grab this in memory window and set the variable you are looking for.
#endif
AptValue *AptActionInterpreter::getVariable(AptValue *pCurrentContext,
                                            AptValue *pWith,
                                            const AptNativeString *pVarName,
                                            int bGlobal,
                                            int bLookInFunctionScope,
                                            int bIsMember)
{

    AptValue *pContext;
    AptNativeString sVar;
    const AptNativeString *sVar1 = NULL;

    if (pCurrentContext == gpUndefinedCIH) // simply return undefined if the context is gpUndefinedCIH
    {
        return gpUndefinedValue;
    }
#if defined APT_AS_DEBUGGING
    if (gnAptCompareDebugString)
    {
        if (stricmp(pVarName->c_str(), gszAptGetVarCompareStr) == 0)
        {
            gnAptCompareDebugString++;
        }
    }
#endif


    // For localization, we load strings of the form $xyz into MovieClip.prototype when in Flash.
    // These become variables (that start with a $) that are worked with directly in Flash.
    // But, we don't do this in the game, so want them to evaluate to themselves so that they
    // can be fiddled with, and then the game can localize normally.
    if (*pVarName->c_str() == '$')
    {
        AptString *pString = AptString::Create();
        pString->cpy(pVarName);
        return pString;
    }

    bool bRet = false; // 4/8
    if (!bIsMember)
    {
        bRet  = getContext(pCurrentContext, pWith, pVarName, &pContext, sVar);
        sVar1 = &sVar;
    }
    else
    {
        pContext = pCurrentContext;
        sVar1    = pVarName;
    }

    // It is not allowed to have properties with no name!
    // i.e. something like myObject[""] should return undefined.
    if (sVar1->IsEmpty())
    {
        return gpUndefinedValue;
    }

    AptValue *pRet = NULL;
    if (bRet == true && pContext)
    {
        // if bRet is true that means context has a '/' at beginning so call findchild on it.
        pRet = pContext->findChild(sVar1, pWith);
        if (pRet != NULL)
        {
            return (pRet);
        }
    }
    //  Search local first

    if (bLookInFunctionScope && mpCurrentFunction)
    {
        pRet = mpCurrentFunction->GetInScopeChain((AptNativeString *)sVar1);
        if (pRet != NULL)
        {
            return (pRet);
        }
    }

    if ((pContext == NULL) || pContext->isUndefined())
    {
        //  The context is NULL or undefined
        // try without 'with' if we didn't get anything
        return (pWith ? getVariable(pCurrentContext, NULL, pVarName, bGlobal) : gpUndefinedValue);
    }

    //  From now on the context is not NULL, we don't need to test it anymore

    //  Search the member lookup
    pRet = pContext->objectMemberLookup(pContext, sVar1);

    // 1 of 1 -
    // IF the object does not have the Member var then objectMemberLookup should return NULL. This is our cue to
    // continue looking. If it returns undefined value then the member exists but is presently undefined.
    // This was causing ambiguity in that some members are in fact undefined, and searching should stop!
    if (pRet != NULL)
    {
        return (pRet);
    }

    if (bGlobal && bIsMember) // EATech 106301, 106040
    {
        pRet = pContext->findChild(sVar1, pContext, bIsMember != 0);
    }
    else
    {
        pRet = pContext->findChild(sVar1, pWith);
    }

    if (pRet != NULL)
    {
        return (pRet);
    }

    if (pWith != NULL)
    {
        //  Do the same search with pWith == NULL
        return (getVariable(pCurrentContext, NULL, pVarName, bGlobal));
    }

    // Check the parent's hash table (could be an event handler; created in parent's hash)
    if (!pContext->isCIH() && (mpCurrentFunction) && !bIsMember) // getVariable scoping problem
    {
        AptNativeHash *pThisNativeHash = mpCurrentFunction->mpParentAnim->GetNativeHashVirtual();
        if (pThisNativeHash != NULL) // Null pointer exception in AptActionInterpreter::getVariable
        {
            pRet = pThisNativeHash->Lookup(sVar1);
            if (pRet)
            {
                return (pRet);
            }
        }
    }

    // added this new callback in 0.15.03 as FR 302 Add ASSERT to check for reading from an uninitialized variable.
    // This might get called many times, as there could be many occasions in actionscript where value is undefined or uninitiaized.
    // actionscriptor sometimes might be expecting a undefined value
    // This is the only right place to add this callback as later in comparison operators or any other action tag when this undefinedValue
    // is poped, ther eis no way to figure out whether the undefinedValue is because of a un-initialized variable or it is
    // expected like that or a variable is initialized but has a value as undefinedvalue.
    // At this point in getVariable, we have gone thru all the scopes where we want to look for a variable and now
    // we are not able to find it so returning undefined value.

    if (AptGetUserFuncs().pfnUninitializedVarAccess)
    {
        // this will be only called if it is defined.
        // implementation of this callback is optional
        AptGetUserFuncs().pfnUninitializedVarAccess((char *)pVarName->c_str());
    }

    return (gpUndefinedValue);
}

void AptActionInterpreter::valueToObject(AptValue *pCurrentContext, AptValue *pWith, AptValue *pVal, AptValue **ppInst)
{
    if (pVal->isCIH() || pVal->ContainsNativeHashVirtual())
    {
        *ppInst = pVal;
    }
    else if (pVal->isString())
    {
        *ppInst = getObject(pCurrentContext, pWith, pVal->c_string()->GetInternalString());
    }
    else
    {
        APT_ASSERT(NOT_REACHED);
    }
}

#if defined(APT_FORCE_CRASHES)
// This change comes from NCAA CL 345758
// to make sure that this function doesn't get inlined in opt builds
extern void AdapterBufferOverrun();

// placed in a separate function so the symbol shows up in the debugger
void AdapterBufferOverrun()
{
    APT_ASSERT(false && "Stack corrupted - check adapter functions for buffer overrun");
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

void AptActionInterpreter::callFunction(AptValue *pContext,
                                        AptValue *pFuncDef,
                                        int nStackParams,
                                        AptValue *pOverrideThis,
                                        AptValue *pOverrideSuper, bool bEnableGetScope)
{
    int i;
    int nStackElementsPre = stack.GetSize() - nStackParams;

    // this change comes from NCAA CL 345758
#if defined(APT_FORCE_CRASHES)
    unsigned int stackGuard[] = {0xABCDEFFD, 0xABCDEFFD, 0xABCDEFFD, 0xABCDEFFD};
#endif

    bEnableGetScope &= gAptOptCallStackGetScopeInfo;

    if (pFuncDef && (pFuncDef->isNativeFunction() || pFuncDef->isExternalFunction()))
    {
        AptConstantPool origConstantPool = constantPool;

        if (bEnableGetScope)
        {
            AptStackItem &item = gAptOptCallStack->Top();
            item.context       = pContext;
            if (!item.scope && pContext)
            {
                const char *name = pContext->GetClassName();
                item.scope       = (name && *name) ? (const char *)name : 0;
            }
        }

        AptValue *pRetValue = NULL;
        if (pFuncDef->isNativeFunction())
        {
            AptNativeFunction *pNativeFunction = pFuncDef->c_nativefunction();
            pRetValue                          = pNativeFunction->Call(pContext, nStackParams);
        }
        else if (pFuncDef->isExternalFunction())
        {
            AptExternalFunction *externalFunction = pFuncDef->c_externalfunction();
            pRetValue                             = externalFunction->Call(nStackParams); // No context passed to these
        }
        else
        {
            APT_FAIL("Not a function");
        }

        // $TempSave
        stack.PopAndPush(nStackParams, pRetValue);
        constantPool = origConstantPool;
    }
    else if (pFuncDef && pFuncDef->isScriptFunction())
    {

        AptConstantPool origConstantPool         = constantPool;
        AptScriptFunctionBase *pPreviousFunction = mpCurrentFunction;

        mpCurrentFunction = pFuncDef->c_scriptfunction();
        constantPool      = mpCurrentFunction->GetConstantPool();

        bool bInvalidCall = false;

        AptCIH *pCIH = mpCurrentFunction->mpCIH;
        if (pCIH->isCIH())
        {
            AptCIH::AptCIHState eCIHState = pCIH->GetCIHState();

            bInvalidCall |= pCIH->isUndefined();
            bInvalidCall |= (eCIHState == AptCIH::AptCIHState_Unloaded);
            bInvalidCall |= (pCIH->IsLevelInst() && ((eCIHState == AptCIH::AptCIHState_Normal) || (eCIHState == AptCIH::AptCIHState_Zombie)));
        }

        // Be more specific on what can and can't run.
        // During Movie transition can call functions of unloaded movie. - added extra check for unloaded movie.
        if (bInvalidCall)
        {
            stack.Pop(nStackParams);
            stack.PushNoInc(gpUndefinedValue);
            APT_DECSAFE(mpCurrentFunction->mpCIH);
            mpCurrentFunction->mpCIH = gpUndefinedCIH;
        }
        else
        {
            // adding scope info
            if (bEnableGetScope && pCIH->isCIH())
            {
                AptStackItem &item = gAptOptCallStack->Top();
                item.file          = pCIH->IsAnimationInst() ? (const char *)pCIH->GetAnimationInst()->mpFile.Get()->GetName().c_str() : NULL;
                item.context       = pContext;
                if (!item.scope && pContext)
                {
                    const char *name = pContext->GetClassName();
                    item.scope       = (name && *name) ? (const char *)name : 0;
                }
            }

            thisStack.push(pContext);

            AptScriptFunctionState pFuncState;

            // Whoa, watch out, an Actionscript function can remove itself from while it is executing.
            // Make sure that the reference count does not hit zero until we finish running it.
            APT_INC(mpCurrentFunction);
            mpCurrentFunction->SetupBeforeExecution(&pFuncState, pContext, pOverrideThis, pOverrideSuper);

            {
                int nExpectedArgs = mpCurrentFunction->GetNumArguments();
                int nMin          = nExpectedArgs < nStackParams ? nExpectedArgs : nStackParams;

                // Apt-0.18.06 Event trying to take a parameter will assert.
                // What if there are no parameters on stack but function expects few parameters this can happen if an event
                // handler takes a parameter, but when called from runactions, there are no parameters on stack.
                if (nMin > stack.GetSize())
                {
#if !defined(DO_COVERAGE)
                    APT_ASSERT(nMin <= stack.GetSize() && "Number of arguments passed on stack are fewer than expected number of parameters for Actionscript function being executed");
#endif
                    nMin              = stack.GetSize();
                    nStackParams      = nMin;
                    nStackElementsPre = nMin;
                }

                // pass up to nMin from the Stack.
                for (i = 0; i < nMin; i++)
                {
                    AptValue *pParam = stack.At(i);
                    mpCurrentFunction->SetArgument(pParam, i);
                }

                // The Rest (if Any) are undefined Values.
                for (/* continue */; i < nExpectedArgs; i++)
                {
                    mpCurrentFunction->SetArgument(gpUndefinedValue, i);
                }

                // No need to Increment them because they are already placed on functions as arguments.
                stack.Pop(nStackParams);
            }

            // run the function
            {

                AptCIH *pRoot                    = mpCurrentFunction->mpParentAnim->GetRootAnimation();
                AptCharacterInst *pCharacterInst = pRoot ? pRoot->GetAnimationInst() : NULL;
                runStream(mpCurrentFunction->GetByteCodeBase(), mpCurrentFunction->mpCIH, mpCurrentFunction->GetByteCodeSize(), pCharacterInst);
            }

            mpCurrentFunction->CleanupAfterExecution(&pFuncState);
            APT_DEC(mpCurrentFunction);

            thisStack.pop();
        }

        mpCurrentFunction = pPreviousFunction;
        constantPool      = origConstantPool;
    }
    else
    {
        stack.Pop(nStackParams);
        stack.PushNoInc(gpUndefinedValue);
    }

    // 07/19/04 For Try Catch support, we need to be able to unwind the stack
    // In this case we have to throw away everything above where we started this function
    // at.
    int nStackElementsPost = stack.GetSize();

    if (doUnwindStack())
    {
        // Throw away anything that was added after we started.
        if (nStackElementsPost > nStackElementsPre)
            stack.Pop(nStackElementsPost - nStackElementsPre);
    }
    else
    {
        APT_ASSERT(nStackElementsPost == nStackElementsPre ||
                   nStackElementsPost == (nStackElementsPre + 1));
    }

    // this change comes from NCAA CL 345758
#if defined(APT_FORCE_CRASHES)
    if (stackGuard[0] != 0xABCDEFFD || stackGuard[3] != 0xABCDEFFD)
    {
        AdapterBufferOverrun();
    }
#endif
}

void *AptActionInterpreter::PrepareForExecution(const char *pszLocationinfo)
{
    APT_ASSERT(!hasThrownValue());
    return AptScriptFunctionBase::PushStaticData();
}

void *AptActionInterpreter::PrepareForExecution(AptActionSetup *pActionSetup)
{
    APT_ASSERT(!hasThrownValue());

#if defined(APT_DEBUG)
    debugCallStack.Push(new DebugCallStackInfoT(pActionSetup->m_strFunction, pActionSetup->m_pFuncValue, pActionSetup->m_eType));
#endif


    return AptScriptFunctionBase::PushStaticData();
}

void AptActionInterpreter::CleanupAfterExecution(const char *pszLocationinfo, void *pPassedValue)
{
    if (hasThrownValue())
    {
        AptValue *pThrown = getThrownValue();
        TO_STRING(pThrown, pAns);
#if defined(APT_PLATFORM_PSP)
        // changed code to use -- instead of \" as psp Sony compiler was giving compiler error.
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript un-caught exception encountered during -- %s --\n", pszLocationinfo);
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript error message:-- %s --\n", pAns->c_str());
#else
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript un-caught exception encountered during \"%s\"\n", pszLocationinfo);
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript error message: \"%s\"\n", pAns->c_str());
#endif
        clearThrownValue();
    }

    AptScriptFunctionBase::PopStaticData(pPassedValue);
}

void AptActionInterpreter::CleanupAfterExecution(void *pPassedValue, AptActionSetup *pActionSetup)
{
    if (hasThrownValue())
    {
        AptValue *pThrown = getThrownValue();
        TO_STRING(pThrown, pAns);

#if defined(APT_PLATFORM_PSP)
        // changed code to use -- instead of \" as psp Sony compiler was giving compiler error.
#if defined(APT_DEBUG)
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript un-caught exception encountered during -- %s --\n", pActionSetup->m_strFunction);
#else
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript un-caught exception encountered\n");
#endif
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript error message:-- %s --\n", pAns->c_str());
#else
#if defined(APT_DEBUG)
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript un-caught exception encountered during \"%s\"\n", pActionSetup->m_strFunction);
#else
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript un-caught exception encountered\n");
#endif
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "<WARNING> Actionscript error message: \"%s\"\n", pAns->c_str());
#endif
        clearThrownValue();
    }

    AptScriptFunctionBase::PopStaticData(pPassedValue);

#if defined(APT_DEBUG)
    debugCallStack.Pop();
#endif

}

// XXX BUGBUG WARNING: this function INCs the return value (because the constructor may do something that causes is it to be DECd prematurely)
AptValue *AptActionInterpreter::_createObject(const uint8_t *instruction, AptValue *pCurrentContext, AptValue *pCurWith, const AptNativeString *szObject, int nParams, bool bRunConstructor)
{
    AptObject *pObject = NULL;

    AptValue *pConstructor = getVariable(pCurrentContext, pCurWith, szObject);

    // Check if szObject refers to an AptExtObject.
    // _createObject can instantiate an AptExtObject if it
    // has a constructor, which we assume is named __New__.
    if (!pConstructor->isUndefined() && pConstructor->getVtblIndex() == AptVFT_Extension)
    {
        APT_ASSERT(!pConstructor->CanCreateScriptObject());
        AptValue *pFunction = static_cast<AptExtObject *>(pConstructor)->Lookup(StringPool::GetString(SC___New__));
        if (pFunction)
        {

#ifdef APT_DEBUGGER_ENABLE
            bool isScriptActionFlag = AptDebugger::GetInstance()->PushCallStack(pFunction, "<constructor>", instruction, pConstructor);
#endif

            gAptOptCallStack->Push((const char *)"<constructor>", 0, 0, (const char *)szObject->GetBuffer());
            // Create a new instance of that subclass of AptExtObject.
            callFunction(pConstructor, pFunction, nParams, 0, 0, true);
            AptValue *pNewObject = stackGetPop();
            pNewObject->SetClassName(szObject->GetBuffer()); // adding scope info
            APT_INC(pNewObject);
            gAptOptCallStack->Pop();

#ifdef APT_DEBUGGER_ENABLE
            if (isScriptActionFlag)
            {
                AptDebugger::GetInstance()->PopCallStack(pFunction);
            }
#endif

            return pNewObject;
        }
        else
        {
            // On the C++ side, your Apt Extension Object
            // needs a method named __New__ in order to be
            // instantiable.
            // This assert is harmless, but your object
            // just won't get instantiated.
            APT_ASSERT(false);
        }
    }

    // 07/19/04 Fixed issue where only AptScriptfunction and AptNativeFunction  actually made it into this
    if (!pConstructor->isUndefined() && pConstructor->CanCreateScriptObject())
    {
#if defined(APT_USE_SOUND_OBJECT)
        if (szObject->EqualNoCase("Sound"))
        {
            pObject = new AptSound(pCurrentContext->c_cih());
        }
        else
#endif
            if (szObject->EqualNoCase("Array"))
        {
            AptArray *pArray = new AptArray();

            // Choose the correct constructor (it can be
            // somewhat ambiguous.) Basically, if there is only one argument,
            // and it is a positive integer (possibly in the form of a float)
            // then it is the size of the array, not a value in the array.
            if (nParams == 1 &&
                ((stackAt(0)->isInteger() && stackAt(0)->toInteger() >= 0) ||
                 (stackAt(0)->isFloat() && (fmod(stackAt(0)->toFloat(), 1.f) == 0.f))) // is it an integer in disguise?
            )
            {
                int nElements = stackAt(0)->toInteger();
                APT_ASSERT(nElements >= 0); // Must be positive integer.
                // This creates the needed objects.
                pArray->set(nElements - 1, gpUndefinedValue);
            }
            else
            {
                // In this case the params are stack element values.
                for (int i = 0; i < nParams; i++)
                {
                    pArray->set(i, stackAt(i));
                }
            }
            pObject = pArray;
        }
        else if (szObject->EqualNoCase("String"))
        {
            AptString *pString = AptString::Create();
            if (nParams == 1)
                stackAt(0)->Append_ToString(pString->str);

            // Watch out, mostly global objects are either implementable (Error,Array,Sound,Date, etc)
            // or they are global instances of object that are not (and cannot be) instantiated in actionscript
            // (i.e. Global, Key, XML, etc) String is both! I.e. you can do String.fromKeyCode() but still
            // instantiate a new string object. To resolve this, getVariable will return the global string object
            // but in this case we need to get the string constructor instead!
            pConstructor = AptGetLib()->mpGlobalGlobalObject->Lookup(StringPool::GetString(SC_String));
            pObject      = new AptStringObject(pString);
        }
        else if (szObject->EqualNoCase("Date"))
        {
            AptDate *pDate = new AptDate(
                nParams > 0 ? stackAt(0)->toInteger() : -1,
                nParams > 1 ? stackAt(1)->toInteger() : -1,
                nParams > 2 ? stackAt(2)->toInteger() : -1,
                nParams > 3 ? stackAt(3)->toInteger() : -1,
                nParams > 4 ? stackAt(4)->toInteger() : -1,
                nParams > 5 ? stackAt(5)->toInteger() : -1,
                nParams > 6 ? stackAt(6)->toInteger() : -1);
            pObject = pDate;
        }
        else if (szObject->EqualNoCase("TextFormat"))
        {

            // 1 of 1 - wrong param was passed as Alignment (was the URL param)

            AptTextFormat *pTextFormat = new AptTextFormat(
                nParams > 0 ? stackAt(0) : gpUndefinedValue,                                   // Font name
                nParams > 1 ? stackAt(1)->toFloat() : -1,                                      // Font size
                nParams > 2 ? stackAt(2)->toInteger() : -1,                                    // Font color
                nParams > 3 ? stackAt(3)->toInteger() : -1,                                    // bold
                nParams > 4 ? stackAt(4)->toInteger() : -1,                                    // italic
                nParams > 5 ? stackAt(5)->toInteger() : -1,                                    // underline
                nParams > 6 ? stackAt(6)->toInteger() : 0,                                     // url
                nParams > 7 ? stackAt(7)->toInteger() : 0,                                     // target
                nParams > 8 ? stackAt(8) : gpUndefinedValue,                                   // Alignment
                nParams > 9 ? stackAt(9)->toInteger() : -1,                                    // lMargin
                nParams > 10 ? stackAt(10)->toInteger() : -1,                                  // rMargin
                nParams > 11 ? stackAt(11)->toInteger() : -1,                                  // Indent
                nParams > 12 ? stackAt(12)->toInteger() : TextFormat::UNDEFINED_LEADING_VALUE, // Leading
                nParams > 13 ? stackAt(13)->toInteger() : TextFormat::UNDEFINED_TRACKING_VALUE // Tracking
            );
            pObject = pTextFormat;
        }
#if APT_USE_SCRIPTCOLOUR_OBJECT
        else if (szObject->EqualNoCase("Color"))
        {
            AptValue *pValue = stackAt(0);
            if (pValue->isString()) // DG When string is passed to Color class constructor
            {
                AptValue *pCIH = getObject(pCurrentContext, NULL, pValue->c_string()->GetInternalString());
                pObject        = new AptScriptColour(pCIH);
            }
            else
            {
                pObject = new AptScriptColour(pValue);
            }
        }
#endif
        else if (szObject->EqualNoCase("MovieClip"))
        {
            pObject = new AptMovieClip();
        }
        else if (szObject->EqualNoCase("MovieClipLoader"))
        {
            pObject = new AptMovieClipLoader();
        }
#if APT_USE_XML_OBJECT
        else if (szObject->EqualNoCase("XML"))
        {
            if (nParams == 1)
                pObject = new AptXml(stackAt(0));
            else
                pObject = new AptXml();
        }
#endif
#if APT_USE_LOADVARS_OBJECT
        else if (szObject->EqualNoCase("LoadVars"))
        {
            pObject = new AptLoadVars();
        }
#endif
        // Added support for Global Error Object. (Release 17.0)
        else if (szObject->EqualNoCase("Error"))
        {
            if (nParams == 1)
            {
                TO_STRING(stackAt(0), psMessage);
                pObject = new AptError(*psMessage);
            }
            else
            {
                pObject = new AptError();
            }
        }
        //-----------------------
        else
        {
            pObject = new AptObject(AptVFT_Object);
        }
        APT_INC(pObject);
        AptNativeHash *pNativeHash      = pConstructor->GetNativeHashVirtual();
        AptValue *pConstructorPrototype = pNativeHash->GetPrototype();

        if (pConstructorPrototype == NULL)
        {
            AptPrototype *pTemp = new AptPrototype();
            pTemp->SetSuperConstructor(pConstructor);
            pConstructorPrototype = pTemp;
            pNativeHash->SetPrototype(pConstructorPrototype);
        }

        pConstructor->SetClassName(szObject->GetBuffer()); // adding scope info

        pObject->SetHasClass(1); // for super in 0.13.00
        pObject->setInMainInst(1);
        pObject->Set__Proto__(pConstructorPrototype);

        pObject->SetClassName(szObject->GetBuffer()); // adding scope info

#if defined(APT_USE_DEBUG_NAMES)
        // When debugging we often want to know the name of classes.
        // This logic grabs the debug name we set during setMember and puts it in the
        // hashtables for the class's prototype.  This will not work outside of creating
        // an instance, so classes with only static functions don't get much from this
        if (pConstructor->isScriptFunction())
        {
            const AptNativeString *nameKeyString = StringPool::GetString(SC__debugName);
            AptNativeHash *protoHash             = pConstructorPrototype->GetNativeHashVirtual();
            AptValue *currentName                = protoHash->Lookup(nameKeyString);
            if (NULL == currentName)
            {
                AptValue *constructorName = pNativeHash->Lookup(nameKeyString);
                if (NULL != constructorName)
                {
                    protoHash->Set(nameKeyString, constructorName);
                }
            }
        }
#endif

#if APT_TRACK_IMPLEMENTED_INTERFACES
        // Added support for tracking implemented interfaces. (Release 17.0)
        if (pConstructor->isObject() || pConstructor->isScriptFunction())
        {
            int nImplementedObjects;
            AptArray *prototypes = ((AptObject *)pConstructor)->GetImplementedObjects(&nImplementedObjects);
            if (nImplementedObjects > 0)
            {
                pObject->SetImplementedObjects(prototypes, nImplementedObjects);
            }
        }
#endif

        if (bRunConstructor)
        {
            // Need to put the object on the this stack so that member operations will work during the constructor.
            thisStack.push(pObject);

#ifdef APT_DEBUGGER_ENABLE
            bool isScriptActionFlag = AptDebugger::GetInstance()->PushCallStack(pConstructor, "<constructor>", instruction, pObject);
#endif

            gAptOptCallStack->Push((const char *)"<constructor>", 0, 0, (const char *)szObject->GetBuffer());

#if defined(APT_DEBUG)
            debugCallStack.Push(new DebugCallStackInfoT(szObject->c_str(), pObject, AptActionType_ConstructorFunc));
#endif


            // push object onto stack so that super constructor(s) can find it
            createdObjectsStack.push(pObject);

            callFunction(pObject, pConstructor, nParams);

            APT_ASSERT(createdObjectsStack.top() == pObject);
            createdObjectsStack.pop();

            stack.Pop(); // throw away return value from ctor
            thisStack.pop();
#if defined(APT_DEBUG)
            debugCallStack.Pop();
#endif

            gAptOptCallStack->Pop();

#ifdef APT_DEBUGGER_ENABLE
            if (isScriptActionFlag)
            {
                AptDebugger::GetInstance()->PopCallStack(pConstructor);
            }
#endif
        }

        pObject->setInMainInst(0);
    }
    else
    {
        stackSafePop(nParams); // pop all the object parameters off of stack
    }

    return pObject;
}

void AptActionInterpreter::_doEnumerate(AptValue *pCurrentContext, AptValue *pCurWith)
{
    AptValue *pObject = stackAt(0);

    if (pObject->isString()) // this is Enumerate
    {
        pObject = getVariable(pCurrentContext, pCurWith, pObject->c_string()->GetInternalString());
    }
    else
    {
        // this is Enumerate2
    }

    // $TempSave
    stackPopNoDec(); // Don't Dec pObject,  Will Decrement it Later!
    stackPushNoInc(gpUndefinedValue);

    AptNativeHash *pNativeHash = pObject->GetNativeHashVirtual();
    while (pNativeHash)
    {
        for (AptHashItem *pItem = pNativeHash->GetFirstItem(); pItem; pItem = pNativeHash->GetNextItem(pItem))
        {
            // TODO: theoretically, on every hash key there's supposed to be a DontEnum/DontDelete flag; we don't have that so hardcode some dontenum's here..
            if ((pItem->Key.EqualNoCase(*StringPool::GetString(SC___proto__))) ||
                (pItem->Key.EqualNoCase(*StringPool::GetString(SC_prototype))))
            {
                continue;
            }
            AptString *pString = AptString::Create();
            pString->cpy(&pItem->Key);
            stackPush(pString);
        }
        AptValue *pProto = pNativeHash->Get__Proto__();
        pNativeHash      = pProto ? pProto->GetNativeHashVirtual() : 0;
    }

    APT_DEC(pObject); // Left over Decrement from above.
}

static AptString *_concatAsStrings(AptValue *pA, AptValue *pB)
{
    AptString *pBA = AptString::Create();

    pB->Append_ToString(pBA->str);
    pA->Append_ToString(pBA->str);

    return pBA;
}

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
#define FILL_ACTION(action, function) {action, function}
#else
#define FILL_ACTION(action, function) {function}
#endif

AptActionInterpreter::FunctionTable AptActionInterpreter::sGlobalTable[LastAptAction] =
    {
        // 0x000
        FILL_ACTION(AptActionEnd, AptActionInterpreter::_FunctionAptActionEnd),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionNextFrame, &AptActionInterpreter::_FunctionAptActionNextFrame),
        FILL_ACTION(AptActionPrevFrame, &AptActionInterpreter::_FunctionAptActionPrevFrame),
        FILL_ACTION(AptActionPlay, &AptActionInterpreter::_FunctionAptActionPlay),
        FILL_ACTION(AptActionStop, &AptActionInterpreter::_FunctionAptActionStop),
        // 0x008
        FILL_ACTION(AptActionToggleQuality, &AptActionInterpreter::_FunctionAptActionToggleQuality),
#if defined(APT_USE_SOUND_OBJECT)
        FILL_ACTION(AptActionStopSounds, &AptActionInterpreter::_FunctionAptActionStopSounds),
#endif
        FILL_ACTION(AptActionAdd, &AptActionInterpreter::_FunctionAptActionAdd),
        FILL_ACTION(AptActionSubtract, &AptActionInterpreter::_FunctionAptActionSubtract),
        FILL_ACTION(AptActionMultiply, &AptActionInterpreter::_FunctionAptActionMultiply),
        FILL_ACTION(AptActionDivide, &AptActionInterpreter::_FunctionAptActionDivide),
        FILL_ACTION(AptActionEquals, &AptActionInterpreter::_FunctionAptActionEquals),
        FILL_ACTION(AptActionLessThan, &AptActionInterpreter::_FunctionAptActionLessThan),
        // 0x010
        FILL_ACTION(AptActionAnd, &AptActionInterpreter::_FunctionAptActionAnd),
        FILL_ACTION(AptActionOr, &AptActionInterpreter::_FunctionAptActionOr),
        FILL_ACTION(AptActionNot, &AptActionInterpreter::_FunctionAptActionNot),
        FILL_ACTION(AptActionStringEquals, &AptActionInterpreter::_FunctionAptActionStringEquals),
        FILL_ACTION(AptActionStringLength, &AptActionInterpreter::_FunctionAptActionStringLength),
        FILL_ACTION(AptActionSubString, &AptActionInterpreter::_FunctionAptActionSubString),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionPop, &AptActionInterpreter::_FunctionAptActionPop),
        // 0x018
        FILL_ACTION(AptActionToInteger, &AptActionInterpreter::_FunctionAptActionToInteger),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionGetVariable, &AptActionInterpreter::_FunctionAptActionGetVariable),
        FILL_ACTION(AptActionSetVariable, &AptActionInterpreter::_FunctionAptActionSetVariable),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        // 0x020
        FILL_ACTION(AptActionSetTarget2, &AptActionInterpreter::_FunctionAptActionSetTarget2),
        FILL_ACTION(AptActionStringAdd, &AptActionInterpreter::_FunctionAptActionStringAdd),
        FILL_ACTION(AptActionGetProperty, &AptActionInterpreter::_FunctionAptActionGetProperty),
        FILL_ACTION(AptActionSetProperty, &AptActionInterpreter::_FunctionAptActionSetProperty),
        FILL_ACTION(AptActionCloneSprite, &AptActionInterpreter::_FunctionAptActionCloneSprite),
        FILL_ACTION(AptActionRemoveSprite, &AptActionInterpreter::_FunctionAptActionRemoveSprite),
        FILL_ACTION(AptActionTrace, &AptActionInterpreter::_FunctionAptActionTrace),
        FILL_ACTION(AptActionStartDragMovie, &AptActionInterpreter::_FunctionAptActionStartDragMovie),
        // 0x028
        FILL_ACTION(AptActionStopDragMovie, &AptActionInterpreter::_FunctionAptActionStopDragMovie),
        FILL_ACTION(AptActionStringLessThan, &AptActionInterpreter::_FunctionAptActionStringLessThan),
        FILL_ACTION(AptActionThrow, &AptActionInterpreter::_FunctionAptActionThrow),               // Added 0x2A for Flash7/AS2 in Apt Release 17.0
        FILL_ACTION(AptActionCastOp, &AptActionInterpreter::_FunctionAptActionCastOp),             // Added 0x2B for Flash7/AS2 in Apt Release 17.0
        FILL_ACTION(AptActionImplementsOp, &AptActionInterpreter::_FunctionAptActionImplementsOp), // Added 0x2C for Flash7/AS2 in Apt Release 17.0
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        // 0x030
        FILL_ACTION(AptActionRandom, &AptActionInterpreter::_FunctionAptActionRandom),
        FILL_ACTION(AptActionMBLength, &AptActionInterpreter::_FunctionAptActionMBLength),
        FILL_ACTION(AptActionCharToAscii, &AptActionInterpreter::_FunctionAptActionCharToAscii),
        FILL_ACTION(AptActionAsciiToChar, &AptActionInterpreter::_FunctionAptActionAsciiToChar),
        FILL_ACTION(AptActionGetTimer, &AptActionInterpreter::_FunctionAptActionGetTimer),
        FILL_ACTION(AptActionMBSubString, &AptActionInterpreter::_FunctionAptActionMBSubString),
        FILL_ACTION(AptActionMBCharToAscii, &AptActionInterpreter::_FunctionAptActionMBCharToAscii),
        FILL_ACTION(AptActionMBAsciiToChar, &AptActionInterpreter::_FunctionAptActionMBAsciiToChar),
        // 0x038
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionDelete, &AptActionInterpreter::_FunctionAptActionDelete),
        FILL_ACTION(AptActionDelete2, &AptActionInterpreter::_FunctionAptActionDelete2),
        FILL_ACTION(AptActionDefineLocal, &AptActionInterpreter::_FunctionAptActionDefineLocal),
        FILL_ACTION(AptActionCallFunction, &AptActionInterpreter::_FunctionAptActionCallFunction),
        FILL_ACTION(AptActionReturn, &AptActionInterpreter::_FunctionAptActionReturn),
        FILL_ACTION(AptActionModulo, &AptActionInterpreter::_FunctionAptActionModulo),
        // 0x040
        FILL_ACTION(AptActionNewObject, &AptActionInterpreter::_FunctionAptActionNewObject),
        FILL_ACTION(AptActionDefineLocal2, &AptActionInterpreter::_FunctionAptActionDefineLocal2),
        FILL_ACTION(AptActionInitArray, &AptActionInterpreter::_FunctionAptActionInitArray),
        FILL_ACTION(AptActionInitObject, &AptActionInterpreter::_FunctionAptActionInitObject),
        FILL_ACTION(AptActionTypeOf, &AptActionInterpreter::_FunctionAptActionTypeOf),
        FILL_ACTION(AptActionTargetPath, &AptActionInterpreter::_FunctionAptActionTargetPath),
        FILL_ACTION(AptActionEnumerate, &AptActionInterpreter::_FunctionAptActionEnumerate),
        FILL_ACTION(AptActionAdd2, &AptActionInterpreter::_FunctionAptActionAdd2),
        // 0x048
        FILL_ACTION(AptActionLessThan2, &AptActionInterpreter::_FunctionAptActionLessThan2),
        FILL_ACTION(AptActionEquals2, &AptActionInterpreter::_FunctionAptActionEquals2),
        FILL_ACTION(AptActionToNumber, &AptActionInterpreter::_FunctionAptActionToNumber),
        FILL_ACTION(AptActionToString, &AptActionInterpreter::_FunctionAptActionToString),
        FILL_ACTION(AptActionPushDuplicate, &AptActionInterpreter::_FunctionAptActionPushDuplicate),
        FILL_ACTION(AptActionStackSwap, &AptActionInterpreter::_FunctionAptActionStackSwap),
        FILL_ACTION(AptActionGetMember, &AptActionInterpreter::_FunctionAptActionGetMember),
        FILL_ACTION(AptActionSetMember, &AptActionInterpreter::_FunctionAptActionSetMember),
        // 0x050
        FILL_ACTION(AptActionIncrement, &AptActionInterpreter::_FunctionAptActionIncrement),
        FILL_ACTION(AptActionDecrement, &AptActionInterpreter::_FunctionAptActionDecrement),
        FILL_ACTION(AptActionCallMethod, &AptActionInterpreter::_FunctionAptActionCallMethod),
        FILL_ACTION(AptActionNewMethod, &AptActionInterpreter::_FunctionAptActionNewMethod),
        FILL_ACTION(AptActionInstanceOf, &AptActionInterpreter::_FunctionAptActionInstanceOf),
        FILL_ACTION(AptActionEnumerate2, &AptActionInterpreter::_FunctionAptActionEnumerate2),
        FILL_ACTION(AptActionPushThis, &AptActionInterpreter::_FunctionAptActionPushThis),
        FILL_ACTION(AptActionInvalid, NULL),
        // 0x058
        FILL_ACTION(AptActionPushGlobal, &AptActionInterpreter::_FunctionAptActionPushGlobal),
        FILL_ACTION(AptActionPush0, &AptActionInterpreter::_FunctionAptActionPush0),
        FILL_ACTION(AptActionPush1, &AptActionInterpreter::_FunctionAptActionPush1),
        FILL_ACTION(AptActionCallFuncAndPop, &AptActionInterpreter::_FunctionAptActionCallFuncAndPop),
        FILL_ACTION(AptActionCallFuncSetVar, &AptActionInterpreter::_FunctionAptActionCallFuncSetVar),
        FILL_ACTION(AptActionCallMethodPop, &AptActionInterpreter::_FunctionAptActionCallMethodPop),
        FILL_ACTION(AptActionCallMethodSetVar, &AptActionInterpreter::_FunctionAptActionCallMethodSetVar),
        FILL_ACTION(AptActionInvalid, NULL),
        // 0x060
        FILL_ACTION(AptActionBitAnd, &AptActionInterpreter::_FunctionAptActionBitAnd),
        FILL_ACTION(AptActionBitOr, &AptActionInterpreter::_FunctionAptActionBitOr),
        FILL_ACTION(AptActionBitXor, &AptActionInterpreter::_FunctionAptActionBitXor),
        FILL_ACTION(AptActionBitLShift, &AptActionInterpreter::_FunctionAptActionBitLShift),
        FILL_ACTION(AptActionBitRShift, &AptActionInterpreter::_FunctionAptActionBitRShift),
        FILL_ACTION(AptActionBitURShift, &AptActionInterpreter::_FunctionAptActionBitURShift),
        FILL_ACTION(AptActionStrictEquals, &AptActionInterpreter::_FunctionAptActionStrictEquals),
        FILL_ACTION(AptActionGreater, &AptActionInterpreter::_FunctionAptActionGreater),
        // 0x068
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionExtends, &AptActionInterpreter::_FunctionAptActionExtends), // Added 0x69 for Flash7/AS2 in Apt Release 17.0
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        // 0x070
        FILL_ACTION(AptActionPushThisVariable, &AptActionInterpreter::_FunctionAptActionPushThisVariable),
        FILL_ACTION(AptActionPushGlobalVariable, &AptActionInterpreter::_FunctionAptActionPushGlobalVariable),
        FILL_ACTION(AptActionPushZeroSetVar, &AptActionInterpreter::_FunctionAptActionPushZeroSetVar),
        FILL_ACTION(AptActionPushTrue, &AptActionInterpreter::_FunctionAptActionPushTrue),
        FILL_ACTION(AptActionPushFalse, &AptActionInterpreter::_FunctionAptActionPushFalse),
        FILL_ACTION(AptActionPushNULL, &AptActionInterpreter::_FunctionAptActionPushNULL),
        FILL_ACTION(AptActionPushUndefined, &AptActionInterpreter::_FunctionAptActionPushUndefined),
        FILL_ACTION(AptActionTraceStart, &AptActionInterpreter::_FunctionAptActionTraceStart),
        // 0x078
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        // 0x080
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionGotoFrame, &AptActionInterpreter::_FunctionAptActionGotoFrame),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionGetUrl, &AptActionInterpreter::_FunctionAptActionGetUrl),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionStoreRegister, &AptActionInterpreter::_FunctionAptActionStoreRegister),
        // 0x088
        FILL_ACTION(AptActionDefineDictionary, &AptActionInterpreter::_FunctionAptActionDefineDictionary),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionWaitForFrame, &AptActionInterpreter::_FunctionAptActionWaitForFrame),
        FILL_ACTION(AptActionSetTarget, &AptActionInterpreter::_FunctionAptActionSetTarget),
        FILL_ACTION(AptActionGotoLabel, &AptActionInterpreter::_FunctionAptActionGotoLabel),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionDefineFunction2, &AptActionInterpreter::_FunctionAptActionDefineFunction2),
        FILL_ACTION(AptActionTry, &AptActionInterpreter::_FunctionAptActionTry), // Added 0x8F for Flash7/AS2 in Apt Release 17.0
                                                                                 // 0x090
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionWith, &AptActionInterpreter::_FunctionAptActionWith),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionPush, &AptActionInterpreter::_FunctionAptActionPush),
        FILL_ACTION(AptActionInvalid, NULL),
        // 0x098
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionBranchAlways, &AptActionInterpreter::_FunctionAptActionBranchAlways),
        FILL_ACTION(AptActionGetUrl2, &AptActionInterpreter::_FunctionAptActionGetUrl2),
        FILL_ACTION(AptActionDefineFunction, &AptActionInterpreter::_FunctionAptActionDefineFunction),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionBranchIfTrue, &AptActionInterpreter::_FunctionAptActionBranchIfTrue),
        FILL_ACTION(AptActionCallFrame, &AptActionInterpreter::_FunctionAptActionCallFrame),
        FILL_ACTION(AptActionGotoFrame2, &AptActionInterpreter::_FunctionAptActionGotoFrame2),
        // 0x0A0
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionPushString, &AptActionInterpreter::_FunctionAptActionPushString),
        FILL_ACTION(AptActionPushStringDictByte, &AptActionInterpreter::_FunctionAptActionPushStringDictByte),
        FILL_ACTION(AptActionPushStringDictWord, &AptActionInterpreter::_FunctionAptActionPushStringDictWord),
        FILL_ACTION(AptActionPushStringGetVar, &AptActionInterpreter::_FunctionAptActionPushStringGetVar),
        FILL_ACTION(AptActionPushStringGetMember, &AptActionInterpreter::_FunctionAptActionPushStringGetMember),
        FILL_ACTION(AptActionPushStringSetVar, &AptActionInterpreter::_FunctionAptActionPushStringSetVar),
        FILL_ACTION(AptActionPushStringSetMember, &AptActionInterpreter::_FunctionAptActionPushStringSetMember),
        // 0x0A8
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionInvalid, NULL),
        FILL_ACTION(AptActionStringDictByteGetVar, &AptActionInterpreter::_FunctionAptActionStringDictByteGetVar),
        FILL_ACTION(AptActionStringDictByteGetMember, &AptActionInterpreter::_FunctionAptActionStringDictByteGetMember),
        // 0x0B0
        FILL_ACTION(AptActionDictCallFuncPop, &AptActionInterpreter::_FunctionAptActionDictCallFuncPop),
        FILL_ACTION(AptActionDictCallFuncSetVar, &AptActionInterpreter::_FunctionAptActionDictCallFuncSetVar),
        FILL_ACTION(AptActionDictCallMethodPop, &AptActionInterpreter::_FunctionAptActionDictCallMethodPop),
        FILL_ACTION(AptActionDictCallMethodSetVar, &AptActionInterpreter::_FunctionAptActionDictCallMethodSetVar),
        FILL_ACTION(AptActionPushFloat, &AptActionInterpreter::_FunctionAptActionPushFloat),
        FILL_ACTION(AptActionPushByte, &AptActionInterpreter::_FunctionAptActionPushByte),
        FILL_ACTION(AptActionPushWord, &AptActionInterpreter::_FunctionAptActionPushWord),
        FILL_ACTION(AptActionPushDWord, &AptActionInterpreter::_FunctionAptActionPushDWord),
        // 0x0B8
        FILL_ACTION(AptActionBranchIfFalse, &AptActionInterpreter::_FunctionAptActionBranchIfFalse),
        FILL_ACTION(AptActionPushRegister, &AptActionInterpreter::_FunctionAptActionPushRegister),

        FILL_ACTION(AptActionBreakpoint, &AptActionInterpreter::_FunctionAptActionBreakpoint),
};

const uint8_t *AptActionInterpreter::runStream(const uint8_t *aActionStream, AptCIH *pCurrentContext, int nMaxStreamBytes, AptCharacterInst *pParentCharacter)
{
    if (nMaxStreamBytes == -1 && pCurrentContext)
    {
        thisStack.push(pCurrentContext);
    }

    LocalContextT context;
    context.pCurrentContext = pCurrentContext;
    context.pCurWith        = NULL;
    context.pInstruction    = aActionStream;
    context.pRemoveWithAt   = NULL;
    // do this only one time and keep the super ready for comparisons.
    context.pSuper                             = getVariable(pCurrentContext, NULL, StringPool::GetString(SC_super));
    context.bEncounteredReturn                 = false;
    context.pParentCharacter                   = pParentCharacter;
    Actions eAction                            = AptActionInvalid;                      // Apt20 optimization - need to initialize eAction since we may read the value before setting it for the first time.
    bool bMaxStreamBytesGreaterThanEqualToZero = (nMaxStreamBytes >= 0 ? true : false); // Apt20 optimization - do compare once and save result to use later.
    const uint8_t *pEndOfActionStream          = aActionStream + nMaxStreamBytes;       // Apt20 optimization - Set pointer to end of bytecode stream to test in while loop if we have passed the end of the stream.

    {

        int nPrevoiousStackFrameBase = mnStackFrameBase;
        mnStackFrameBase             = stack.GetSize();

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        int nStackSizePre = mnStackFrameBase;
#endif

        // Added for Flash Player 7 and AS 2.0 support. (Release 17.0)
        // This will stop execution when an exception is thrown (or whenever we need to unwind the stack.
        while (doUnwindStack() == false)
        {
            APT_ASSERT(!context.pRemoveWithAt || context.pInstruction <= context.pRemoveWithAt);
            if ((context.pInstruction == context.pRemoveWithAt) && context.pRemoveWithAt) // Apt20 optimization - changed order of operations, most of the time the first comparison will fail, causing the second comparison to not occur (saving cycles)
            {
                APT_DEC(context.pCurWith);
                context.pCurWith      = NULL;
                context.pRemoveWithAt = NULL;
            }

            // Jump out of loop if we encountered AptActionReturn or AptActionEnd (both will set this boolean to TRUE to break out).
            if (context.bEncounteredReturn) // Apt20 optimization - when AptActionEnd is executed, this variable will be set to true. This eliminates an extra else-if statement that used to be here.
            {
                if ((eAction == AptActionEnd) && bMaxStreamBytesGreaterThanEqualToZero)
                {
                    stackPush(gpUndefinedValue);
                }
                break;
            }

            eAction = (Actions)*context.pInstruction++; // Apt20 optimization - moved down so eAction retains old value for AptActionEnd check above.

            // Check for end of stream first in case the function/method
            // borders on an end action (so that we don't execute the end action unintentionally!)
            if ((context.pInstruction > pEndOfActionStream) && bMaxStreamBytesGreaterThanEqualToZero) // Apt20 optimization - changed order of operations, most of the time the first comparison will fail, causing the second comparison to not occur (saving cycles)
            {
                stackPush(gpUndefinedValue);
                break;
            }

            APT_ASSERT(pCurrentContext == NULL || pCurrentContext->getRefCount() > 0);

            //  Call the corresponding method
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
            APT_ASSERT(sGlobalTable[eAction].mCheckAlignment == eAction);
#endif
            sGlobalTable[eAction].mFunctionPointer(this, &context); // <-- Execute byte code

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
            // Check for Stack problems.
            int nStackSizeCurrent = stack.GetSize();
            APT_ASSERT(nStackSizeCurrent >= nStackSizePre);

#if APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM
            // This Check is only valid for small object allocator.
            // It just makes sure that the objects on the stack have not been deleted.
            for (int i = mnStackFrameBase; i < stack.GetSize(); i++)
            {
                APT_ASSERT(stack.At(i)->mnValueData != 0x0d0d0d0d);
            }
#endif
#endif
        }

        // Added Stack Frames. When any stream is completed, we pull ensure we go back to the previous frame.
        int nStackSizePost = stack.GetSize();

        // Apt20 optimization - changed order of operations, most of the time the first comparison will fail, causing the second comparison to not occur (saving cycles). Also removed extra else-if statement.
        if (nStackSizePost > mnStackFrameBase)
        {
            if (bMaxStreamBytesGreaterThanEqualToZero)
            {
                stack.Pop(nStackSizePost - mnStackFrameBase - 1);
            }
            else
            {
                stack.Pop(nStackSizePost - mnStackFrameBase);
            }
        }
        // Restore the previous stack Frame value.
        mnStackFrameBase = nPrevoiousStackFrameBase;

    }

    if (nMaxStreamBytes == -1)
    {
        thisStack.pop();
    }

    // APT_ASSERT(context.pCurWith == NULL); // commented for super crap

    int32_t iStackSize = stack.GetSize();
    bool bRelease      = false;

    if (iStackSize == 0)
    {
        //  The stack is empty, we can remove all the intermediate objects
        bRelease = true;
    }
    else if ((iStackSize == 1) && (stack.At(0) == gpUndefinedValue))
    {
        //  The stack contains one element, but it's undefined value
        bRelease = true;
    }

    if (bRelease)
    {
        if (AptGetLib()->mpValuesToRelease->GetNumValues() != 0)
        {
            //  There are some values to collect, we do it
            //  The same test is done internally but by doing that we avoid to call this function
            //  if not needed
            AptGetLib()->mpValuesToRelease->ReleaseValues();
        }
    }

    return context.pInstruction;
}

static inline bool IsAsciiChar(int32_t c)
{
    return (c <= 127) && (c >= 0);
}

void _escape(AptNativeString &sEsc)
{
    char strBuffer[6]  = "";
    char strBuffer1[2] = "";
    AptNativeString sUnescapedCopy;
    sUnescapedCopy.Reserve(sEsc.Size() * 3); //  The escaped string could be at the max 3 times same size of the original string

    const char *pBuffer = sEsc.c_str();
    unsigned char cChar;
    strBuffer1[1] = 0;
    for (;;)
    {
        cChar = *pBuffer++;
        if (cChar == 0)
        {
            break;
        }
        if (!IsAsciiChar(cChar) || !isalnum(cChar))
        {
            sprintf(strBuffer, "%%%X", cChar);
            sUnescapedCopy += strBuffer;
        }
        else
        {
            strBuffer1[0] = cChar;
            sUnescapedCopy += strBuffer1;
        }
    }
    sEsc = sUnescapedCopy;
}

static char _escape2Char(const char a, const char b)
{
    // First check for the case "% " where it isn't a hex sequence, in this case Flash replaces it with "".
    if (!isxdigit(a))
    {
        return b; // i.e. "50% accuracy" would be "50accuracy"; same behavior as Flash.
    }

    APT_ASSERT(isxdigit(b));

    char sBuf[3];
    sBuf[0] = a;
    sBuf[1] = b;
    sBuf[2] = 0;

    unsigned long x = strtoul(sBuf, 0, 16);
    return (char)x;
}

/**
    @brief Converts a string with HTML ESC-sequences to one without.
    The Unicode is not ambiguous with standard ASCII code, so no need to use here UTF8 functions
*/
void _unEscape(AptNativeString &sEsc)
{
    char strBuffer[2] = "*";
    AptNativeString sUnescapedCopy;
    sUnescapedCopy.Reserve(sEsc.Size()); //  The unescaped string should be around the same size of the original string

    const char *pBuffer = sEsc.c_str();
    unsigned char cChar;

    for (;;)
    {
        cChar = *pBuffer++;
        if (cChar == 0)
        {
            break;
        }
        if (cChar == '+')
        {
            strBuffer[0] = ' ';
        }
        else if (cChar == '%' && (*pBuffer) != 0) // unescaping already unescaped strings
        {
            strBuffer[0] = _escape2Char(*pBuffer, *(pBuffer + 1));
            pBuffer += 2;
        }
        else
        {
            strBuffer[0] = cChar;
        }
        sUnescapedCopy += strBuffer;
    }

    sEsc = sUnescapedCopy;
}

//  The Unicode is not ambiguous with standard ASCII code, so no need to use here UTF8 functions
const char *AptActionInterpreter::urlDecode(const char *szURL, AptNativeString &sKey, AptNativeString &sValue)
{
    APT_ASSERT(szURL != NULL);

    const char *szCur    = szURL;
    const char *szEquals = NULL;
    sKey.Clear();
    sValue.Clear();
    for (; szCur && *szCur && *szCur != '&'; szCur++)
    {
        if (*szCur == '=')
        {
            szEquals = szCur;
        }
    }
    if (szEquals)
    {
        sKey.Append(szURL, szEquals - szURL);
        _unEscape(sKey);
        ++szEquals;
        /*lint --esym(794, szCur) */
        sValue.Append(szEquals, szCur - szEquals);
        _unEscape(sValue);
        if (*szCur == '&')
        {
            szCur++;
        }
    }
    else
    {
        szCur = NULL;
    }
    return szCur;
}

bool AptActionInterpreter::isFSCommand(const char *szCommand)
{
    // Check to see if it is an actual FSCommand
    if (strncmp(szCommand, FSCOMMAND, strlen(FSCOMMAND)) == 0)
        return true;

    return false;
}

int AptActionInterpreter::doFSCommand(const char *szCommand, const char *szParams)
{
    // Check to see if it is an actual FSCommand
    APT_ASSERT(isFSCommand(szCommand));

    szCommand = &szCommand[strlen(FSCOMMAND)];
    AptGetUserFuncs().pfnCommand(szCommand, szParams);

    return 1;
}

void AptActionInterpreter::_FunctionAptActionEnd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    // Apt20 optimization - when AptActionEnd is executed, set context.bEncounteredReturn to true to break out of
    // runStream while loop after the next iteration (saves an extra "else-if" for each loop in runStream).
    pLocalContext->bEncounteredReturn = 1;
    return;

    // Not the case anymore, optimized runStream while loop and by doing so, we need to execute AptActionEnd. But we just set a boolean & return.
    // APT_ASSERT(false);              //  Not Reached. (We should never get here)
}

void AptActionInterpreter::_FunctionAptActionNextFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptCIH *pCIH = pLocalContext->pCurrentContext->c_cih();
    pCIH->jumpToFrame(pCIH->GetSpriteInstBase()->mnFrame + 1);
    pCIH->SetIsPlaying(false);
}

void AptActionInterpreter::_FunctionAptActionPrevFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptCIH *pCIH = pLocalContext->pCurrentContext->c_cih();
    pCIH->jumpToFrame(pCIH->GetSpriteInstBase()->mnFrame - 1);
    pCIH->SetIsPlaying(false);
}

void AptActionInterpreter::_FunctionAptActionPlay(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptCIH *pCurrentContext = pLocalContext->pCurrentContext;
    if (!pCurrentContext->isUndefined() && !pCurrentContext->IsLevelInst())
    {
        if (pLocalContext->pCurWith && pLocalContext->pCurWith->isCIH())
        {
            pLocalContext->pCurWith->c_cih()->SetIsPlaying(true);
        }
        else if (pLocalContext->pCurrentContext->isCIH())
        {
            pCurrentContext->c_cih()->SetIsPlaying(true);
        }
    }
}

void AptActionInterpreter::_FunctionAptActionStop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptCIH *pCurrentContext = pLocalContext->pCurrentContext;
    if (!pCurrentContext->isUndefined() && !pCurrentContext->IsLevelInst() && pCurrentContext->GetCharacterInst() != NULL)
    {
        pCurrentContext->c_cih()->SetIsPlaying(false);
    }
}

void AptActionInterpreter::_FunctionAptActionToggleQuality(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(false && " [APT] \"Toggle Quality\" is NOT supported. Please do not use this in your actionscript\n"); //  Not implemented
}

#if defined(APT_USE_SOUND_OBJECT)

void AptActionInterpreter::_FunctionAptActionStopSounds(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
}

#endif

void AptActionInterpreter::_FunctionAptActionAdd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:1/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        // If they are both integers don't waste time with float arithmetic.
        if (pA->isInteger() && pB->isInteger())
        {
            int iA  = pA->c_integer()->GetInt(); // No conversion needed.
            int iB  = pB->c_integer()->GetInt();
            pResult = AptInteger::Create(iA + iB);
        }
        else
        {
            float fA = pA->toFloat(); // Conversion may be needed.
            float fB = pB->toFloat();
            pResult  = AptFloat::Create(fA + fB);
        }
    }
    //(1.1:rrv:1/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionSubtract(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:2/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        // If they are both integers don't waste time with float arithmetic.
        if (pA->isInteger() && pB->isInteger())
        {
            int iA  = pA->c_integer()->GetInt(); // No conversion needed.
            int iB  = pB->c_integer()->GetInt();
            pResult = AptInteger::Create(iB - iA);
        }
        else
        {
            float fA = pA->toFloat(); // Conversion may be needed.
            float fB = pB->toFloat();
            pResult  = AptFloat::Create(fB - fA);
        }
    }
    //(1.1:rrv:2/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionMultiply(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:3/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        // If they are both integers don't waste time with float arithmetic.
        if (pA->isInteger() && pB->isInteger())
        {
            int iA  = pA->c_integer()->GetInt(); // No conversion needed.
            int iB  = pB->c_integer()->GetInt();
            pResult = AptInteger::Create(iA * iB);
        }
        else
        {
            float fA = pA->toFloat(); // Conversion may be needed.
            float fB = pB->toFloat();
            pResult  = AptFloat::Create(fA * fB);
        }
    }
    //(1.1:rrv:3/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionDivide(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;
    float fFloat1;
    float fFloat2;

    //(1.1:rrv:4/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        fFloat1 = pA->toFloat();
        fFloat2 = pB->toFloat();

        if (fFloat1 == 0.f)
        {
            pResult = gpUndefinedValue;
        }
        else
        {
            // Can't do integer division here even if they are integers.
            pResult = AptFloat::Create(fFloat2 / fFloat1);
        }
    }
    //(1.1:rrv:4/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

#if (APT_CURR_DEBUG_MSG_LVL >= APT_DEBUG_MSG_ASSERT_LVL)
#if defined(APT_EQUALITY_TYPECHECK_ASSERT)
// comment todo, response to ion request for more type safety
static bool ComparisonTypeCheck(AptValue *a, AptValue *b)
{
    if (a->isUndefined() || b->isUndefined())
    {
        return true;
    }
    AptVirtualFunctionTable_Indices typeA = a->getVtblIndex(),
                                    typeB = b->getVtblIndex();
    return ((typeA == typeB) ||
            (typeA == AptVFT_Float && typeB == AptVFT_Integer) ||
            (typeB == AptVFT_Float && typeA == AptVFT_Integer));
}
#endif
#endif

void AptActionInterpreter::_FunctionAptActionEquals(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

#if defined(APT_EQUALITY_TYPECHECK_ASSERT)
    APT_ASSERT(ComparisonTypeCheck(pA, pB) && "types are not comparable with ==");
#endif

    //(1.1:rrv:5/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        // If they are both integers don't waste time with float arithmetic.
        if (pA->isInteger() && pB->isInteger())
        {
            int iA  = pA->c_integer()->GetInt(); // No conversion needed.
            int iB  = pB->c_integer()->GetInt();
            pResult = AptBoolean::Create(iA == iB);
        }
        else
        {
            float fA = pA->toFloat(); // Conversion may be needed.
            float fB = pB->toFloat();
            pResult  = AptBoolean::Create(fabsf(fA - fB) < APT_EPSILON);
        }
    }
    //(1.1:rrv:5/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionLessThan(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:6/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // lif(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        // If they are both integers don't waste time with float arithmetic.
        if (pA->isInteger() && pB->isInteger())
        {
            int iA  = pA->c_integer()->GetInt(); // No conversion needed.
            int iB  = pB->c_integer()->GetInt();
            pResult = AptBoolean::Create(iB < iA);
        }
        else
        {
            float fA = pA->toFloat(); // Conversion may be needed.
            float fB = pB->toFloat();
            pResult  = AptBoolean::Create(fB < fA);
        }
    }
    //(1.1:rrv:6/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionAnd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:7/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        // If they are both integers don't waste time with float arithmetic.
        if (pA->isInteger() && pB->isInteger())
        {
            int iA  = pA->c_integer()->GetInt(); // No conversion needed.
            int iB  = pB->c_integer()->GetInt();
            pResult = AptBoolean::Create(iA && iB);
        }
        else
        {
            float fA = pA->toFloat(); // Conversion may be needed.
            float fB = pB->toFloat();
            pResult  = AptBoolean::Create((fA != 0.f && fB != 0.f) ? true : false);
        }
    }
    //(1.1:rrv:7/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionOr(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:8/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        // If they are both integers don't waste time with float arithmetic.
        if (pA->isInteger() && pB->isInteger())
        {
            int iA  = pA->c_integer()->GetInt(); // No conversion needed.
            int iB  = pB->c_integer()->GetInt();
            pResult = AptBoolean::Create(iA || iB);
        }
        else
        {
            float fA = pA->toFloat(); // Conversion may be needed.
            float fB = pB->toFloat();
            pResult  = AptBoolean::Create((fA != 0.f || fB != 0.f) ? true : false);
        }
    }
    //(1.1:rrv:8/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionNot(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue = pInterpreter->stackAt(0);

    AptValue *pRetValue = AptBoolean::Create(!pValue->toBool());

    pInterpreter->stackPop();
    pInterpreter->stackPush(pRetValue);
}

void AptActionInterpreter::_FunctionAptActionStringEquals(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA = pInterpreter->stackAt(0);
    AptValue *pB = pInterpreter->stackAt(1);

    AptValue *pResult = NULL;
    int nResult       = 0;

    //(1.1:rrv:10/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        if (pA->isUndefined())
        {
            nResult++;
        }
        if (pB->isUndefined())
        {
            nResult++;
        }

        switch (nResult)
        {
        case 0:
            // neither one is undefined, so drop thru
            break;
        case 1:
            // if only one value is undefined, return undefined
            pResult = gpUndefinedValue;
            break;
        case 2:
            // if both values are undefined, they are equal!
            pResult = AptBoolean::Create(true);
            break;
        }
    }

    if (pResult == NULL)
    {
        TO_STRING(pA, psAString);
        TO_STRING(pB, psBString);
        pResult = AptBoolean::Create(*psAString == *psBString);
    }
    //(1.1:rrv:10/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionStringLength(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA = pInterpreter->stackAt(0);
    TO_STRING(pA, psAString);
    AptInteger *pResult = AptInteger::Create(psAString->GetLength());
    pInterpreter->stackPop(1);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionSubString(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pCount = pInterpreter->stackAt(0);
    AptValue *pIndex = pInterpreter->stackAt(1);
    AptValue *pStr   = pInterpreter->stackAt(2);

    int nCount, nIndex;
    nCount = pCount->toInteger();
    nIndex = pIndex->toInteger();

    --nIndex; //  1 based instead of 0
    if (nIndex < 0)
    {
        nIndex = 0;
    }

    TO_STRING(pStr, psString);

    AptString *pSubStr = AptString::Create();
    if (nCount == 0)
    {
        pSubStr->str.Clear();
    }
    else if (nCount < 0)
    {
        pSubStr->str = psString->UTF8_Mid(nIndex);
    }
    else
    {
        pSubStr->str = psString->UTF8_Mid(nIndex, nCount);
    }

    pInterpreter->stackPop(3);
    pInterpreter->stackPush(pSubStr);
}

void AptActionInterpreter::_FunctionAptActionPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    int32_t stackElements = pInterpreter->stack.GetSize();

    // Added code to prevent AS code from popping beyond the current stack frame. Unfortunately Flash does this
    // way to often. Note that the check is only for the AS opcode, not for our internal pops, we are handling
    // the stack correctly, it is only the AS compiler being lazy.
    if (stackElements > pInterpreter->mnStackFrameBase)
    {
        pInterpreter->stackPop();
    }
    if (stackElements == 1)
    {
        //  The stack is empty, we can remove all the intermediate objects
        if (AptGetLib()->mpValuesToRelease->GetNumValues() != 0)
        {
            //  There are some values to collect, we do it
            //  The same test is done internally but by doing that we avoid to call this function
            //  if not needed
            AptGetLib()->mpValuesToRelease->ReleaseValues();
        }
    }
}

void AptActionInterpreter::_FunctionAptActionToInteger(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue = pInterpreter->stackAt(0);

    int nInt;
    AptValue *pResult = NULL;

    //(1.1:rrv:11/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the ADD operation is not defined, so return undefined
        if (pValue->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        nInt    = pValue->toInteger();
        pResult = AptInteger::Create(nInt);
    }
    //(1.1:rrv:11/27): AS2 end

    pInterpreter->stackPop();
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionGetVariable(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pName = pInterpreter->stackAt(0);

    if (!pName->isUndefined())
    {
        AptValue *pValue = NULL;
        TO_STRING(pName, psBuf);
        pValue = pInterpreter->getVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, psBuf, true);
        pInterpreter->stackPop();
        pInterpreter->stackPush(pValue);
    }
    else
    {
        // undef is on top of the stack, do not push or pop anything since top of the stack already is a undef
    }
}

void AptActionInterpreter::_FunctionAptActionSetVariable(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    AptValue *pValue = pInterpreter->stackAt(0);
    AptValue *pName  = pInterpreter->stackAt(1);

    TO_STRING(pName, psBuf);

    pInterpreter->setVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, psBuf, pValue, true);
    pInterpreter->stackPop(2);

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionSetTarget2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pObject = pInterpreter->stackAt(0);

    TO_STRING(pObject, psBuf);

    if (0 == psBuf->Size())
    {
        APT_DECSAFE(pLocalContext->pCurWith);
        pLocalContext->pCurWith = 0;
    }
    else
    {
        APT_ASSERT(!pLocalContext->pCurWith);
        AptValue *pTarget = getObject(pLocalContext->pCurrentContext, pLocalContext->pCurWith, psBuf);
        APT_ASSERT(pTarget);
        pLocalContext->pRemoveWithAt = 0;
        pLocalContext->pCurWith      = pTarget;
        APT_INC(pLocalContext->pCurWith);
    }

    pInterpreter->stackPop();
}

void AptActionInterpreter::_FunctionAptActionStringAdd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA = pInterpreter->stackAt(0);
    AptValue *pB = pInterpreter->stackAt(1);

    AptValue *pResult = NULL;

    pResult = _concatAsStrings(pA, pB);

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionGetProperty(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pIndex  = pInterpreter->stackAt(0);
    AptValue *pTarget = pInterpreter->stackAt(1);

    AptValue *pObject;
    valueToObject(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pTarget, &pObject);

    if (pObject)
    {
        unsigned int nInt = pIndex->toInteger(); // they're normally floats, but sometimes ints from what i can tell. they're always actually integral numbers.

        APT_ASSERT(nInt < (sizeof(gaszPropertyNames) / sizeof(gaszPropertyNames[0])));
        // lint --e(662) PcLint warning Warning 662: Possible creation of out-of-bounds pointer (2147483626 beyond end of data) by operator '['
        AptValue *pPropValue = pInterpreter->getVariable(pObject, pLocalContext->pCurWith, StringPool::GetString(gaszPropertyNames[nInt]), true);

        pInterpreter->stackPop(2);
        pInterpreter->stackPush(pPropValue);
    }
    else
    {
        pInterpreter->stackPop(2);
        pInterpreter->stackPush(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionSetProperty(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue  = pInterpreter->stackAt(0);
    AptValue *pIndex  = pInterpreter->stackAt(1);
    AptValue *pTarget = pInterpreter->stackAt(2);

    AptValue *pObject;
    valueToObject(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pTarget, &pObject);
    unsigned int nInt = pIndex->toInteger(); // they're normally floats, but sometimes ints from what i can tell. they're always actually integral numbers.
    if (pObject)
    {
        APT_ASSERT(nInt < (sizeof(gaszPropertyNames) / sizeof(gaszPropertyNames[0])));
        // lint --e(662) PcLint warning Warning 662: Possible creation of out-of-bounds pointer (2147483626 beyond end of data) by operator '['
        pInterpreter->setVariable(pObject, pLocalContext->pCurWith, StringPool::GetString(gaszPropertyNames[nInt]), pValue, true);
    }

    pInterpreter->stackPop(3);
}

void AptActionInterpreter::_FunctionAptActionCloneSprite(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pDepth  = pInterpreter->stackAt(0);
    AptValue *pTarget = pInterpreter->stackAt(1);
    AptValue *pSource = pInterpreter->stackAt(2);
    int nDepthInt     = pDepth->toInteger();

    pInterpreter->_doCloneSprite(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pSource, pTarget, nDepthInt);

    pInterpreter->stackPop(3);
}

void AptActionInterpreter::_FunctionAptActionRemoveSprite(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pPath = pInterpreter->stackAt(0);
    AptValue *pObject;

    if (!pPath->isUndefined()) // this is to allow Apt to continue executing if the path is not found
    {
        valueToObject(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pPath, &pObject);
        if (pObject && pObject->isCIH())
        {
            AptCIH *pCIH                            = pObject->c_cih();
            AptCharacterSpriteInstBase *pParentInst = pCIH->GetDisplayListParent()->GetSpriteInstBase();
            pParentInst->mDisplayList.removeClonedObject(pCIH);
        }
    }
    pInterpreter->stackPop();
}

void AptActionInterpreter::_FunctionAptActionTrace(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{


    AptValue *pValue = pInterpreter->stackAt(0);

    TO_STRING(pValue, psBuf);

    {
        AptGetUserFuncs().pfnDebugPrint("AptTrace: %s", psBuf->ConstRawPtr());

        AptDebugHelper::GetInstance()->AddTrace(*psBuf);
    }

#ifdef APT_DEBUGGER_ENABLE
    AptDebugger::GetInstance()->Trace(psBuf->ConstRawPtr());
#endif

    pInterpreter->stackPop();

}

void AptActionInterpreter::_FunctionAptActionStartDragMovie(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pTarget = pInterpreter->stackAt(0);

    if (pTarget->isString())
    {
        AptValue *pContext = NULL;
        AptNativeString sVar;
        getContext(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pTarget->c_string()->GetInternalString(), &pContext, sVar);
        pTarget = pInterpreter->getVariable(pContext, pLocalContext->pCurWith, &sVar, true);
    }

    APT_ASSERT(pTarget->isCIH());
    int nParams = 3;

    APT_INC(pTarget);
    GetTargetSim()->GetAnimationTarget()->SetDragMC(pTarget);     // Set pDragMC to new target
    GetTargetSim()->GetAnimationTarget()->GetDragPos()->tx = 0.f; // Reset mDragPos Matrix
    GetTargetSim()->GetAnimationTarget()->GetDragPos()->ty = 0.f;
    GetTargetSim()->GetAnimationTarget()->GetDragPos()->a  = -9999.f;
    GetTargetSim()->GetAnimationTarget()->GetDragPos()->b  = -9999.f;
    GetTargetSim()->GetAnimationTarget()->GetDragPos()->c  = -9999.f;
    GetTargetSim()->GetAnimationTarget()->GetDragPos()->d  = -9999.f;

#if defined(APT_PLATFORM_WINDOWS)
    if (pTarget->c_cih()->IsSpriteInst())
    {
        pTarget->c_cih()->GetSpriteInst()->SetDropTarget("");
    }
#endif

    // If lockCenter parameter is false, set the offset vector
    if (!pInterpreter->stackAt(1)->isInteger())
    {
        // GetTargetSim()->GetAnimationTarget()->GetDragPos()->tx = GetTargetSim()->GetAnimationTarget()->nXMousePos - pTarget->c_cih()->matrix.tx;
        // GetTargetSim()->GetAnimationTarget()->GetDragPos()->ty = GetTargetSim()->GetAnimationTarget()->nYMousePos - pTarget->c_cih()->matrix.ty;

        GetTargetSim()->GetAnimationTarget()->GetDragPos()->tx = GetTargetSim()->GetAnimationTarget()->GetXMousePos() - pTarget->c_cih()->GetPositionMatrixConst()->tx;
        GetTargetSim()->GetAnimationTarget()->GetDragPos()->ty = GetTargetSim()->GetAnimationTarget()->GetYMousePos() - pTarget->c_cih()->GetPositionMatrixConst()->ty;
    }

    // If the boundary box parameters are set, update the mDragPos matrix
    if (pInterpreter->stackAt(2)->isInteger())
    {
        nParams += 4;
        GetTargetSim()->GetAnimationTarget()->GetDragPos()->d = pInterpreter->stackAt(3)->toFloat();
        GetTargetSim()->GetAnimationTarget()->GetDragPos()->c = pInterpreter->stackAt(4)->toFloat();
        GetTargetSim()->GetAnimationTarget()->GetDragPos()->b = pInterpreter->stackAt(5)->toFloat();
        GetTargetSim()->GetAnimationTarget()->GetDragPos()->a = pInterpreter->stackAt(6)->toFloat();
    }
    pInterpreter->stackPop(nParams);
}

void AptActionInterpreter::_FunctionAptActionStopDragMovie(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pDrag = GetTargetSim()->GetAnimationTarget()->GetDragMC();
    APT_DECSAFE(pDrag);

#if defined(APT_PLATFORM_WINDOWS)

    if (pDrag->isCIH() && pDrag->c_cih()->IsSpriteInst())
    {
        AptCIH *pDragCIH = pDrag->c_cih();
        AptRect aRect;
        float fStopX = pDragCIH->GetProceduralProperty(AptProceduralProperty_X);
        float fStopY = pDragCIH->GetProceduralProperty(AptProceduralProperty_Y);

        // Look for the first symbol (sprite, movie clip, text, custom control... ) under the dragged symbol
        AptCIH *pCur = GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem();
        bool bFound  = false;

        while (pCur && !bFound)
        {
            const AptCIH *pChild = pCur->GetFirstChild();
            while (pChild && !bFound)
            {
                if (pChild != pDragCIH && !pChild->IsShapeInst())
                {
                    pChild->GetBoundingRect(&aRect);
                    if (aRect.fBottom >= fStopY && aRect.fTop <= fStopY && aRect.fLeft <= fStopX && aRect.fRight >= fStopX)
                    {
                        AptNativeString nameWithPath;
                        getName2((AptCIH *)pChild, nameWithPath);
                        //                        pDragCIH->GetSpriteInst()->SetDropTarget(pChild->GetInstanceName());
                        pDragCIH->GetSpriteInst()->SetDropTarget(nameWithPath);
                        bFound = true;
                    }
                }
                pChild = pChild->GetDisplayListNext();
            }
            pCur = pCur->GetDisplayListNext();
        }
    }
#endif

    GetTargetSim()->GetAnimationTarget()->SetDragMC(gpUndefinedValue); // Set pDragMC to undefined
}

void AptActionInterpreter::_FunctionAptActionStringLessThan(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(false); //  Not implemented
}

void AptActionInterpreter::_FunctionAptActionRandom(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pMax = pInterpreter->stackAt(0);
    int nRand      = pMax->isUndefined() ? 0 : pMax->toInteger();

    if (nRand != 0)
    {
        nRand = AptRand() % nRand;
    }

    AptInteger *pRand = AptInteger::Create(nRand);
    pInterpreter->stackPop();
    pInterpreter->stackPush(pRand);
}

void AptActionInterpreter::_FunctionAptActionMBLength(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(false); //  Not implemented
}

void AptActionInterpreter::_FunctionAptActionCharToAscii(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(false); //  Not implemented
}

void AptActionInterpreter::_FunctionAptActionAsciiToChar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pVal = pInterpreter->stackAt(0);
    if (!pVal->isUndefined())
    {
        AptString *pTargetString = AptString::Create();
        pTargetString->str       = AptNativeString(pVal->toInteger(), 1);
        pInterpreter->stackPop();
        pInterpreter->stackPush(pTargetString);
    }
    else
    {
        pInterpreter->stackPop();
        pInterpreter->stackPush(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionGetTimer(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    if (AptGetUserFuncs().pfnGetCurrentTime)
    {
        // pInterpreter->stackPush(AptInteger::Create(AptGetUserFuncs().pfnGetCurrentTime()));
        pInterpreter->stackPush(AptInteger::Create(AptGetUserFuncs().pfnGetCurrentTime() / 1000));
    }
    else
    {
        pInterpreter->stackPush(AptInteger::Create(AptGetLib()->mnCurTick));
    }
}

void AptActionInterpreter::_FunctionAptActionMBSubString(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(false); //  Not implemented
}

void AptActionInterpreter::_FunctionAptActionMBCharToAscii(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(false); //  Not implemented
}

void AptActionInterpreter::_FunctionAptActionMBAsciiToChar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(false); //  Not implemented
}

void AptActionInterpreter::_FunctionAptActionDelete(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pName   = pInterpreter->stackAt(0);
    AptValue *pObject = pInterpreter->stackAt(1);

    if (pObject->ContainsNativeHashVirtual())
    {
        TO_STRING(pName, psBuf);
        // ActionDelete deletes a named property from a ScriptObject.
        // Flags:
        //   bGlobal=1
        //   bLookInFunctionScope=1
        //   bIsMember=1
        //
        // NOTE: The flags should *probably* be bGlobal=0 bLookInFunctionScope=0 bIsMember=1,
        //       but the previous version of Apt (2.09.00) passed bGlobal=1 and bLookInFunctionScope=1
        //       and we want our change to be minimal. Might need to revisit.

#if !defined(APT_ALLOW_GLOBAL_MEMBER_DELETION)
        pInterpreter->setVariable(pObject, pLocalContext->pCurWith, psBuf, NULL, 1, 1, 1);
#else
        // This code is buggy - please see below comments
        AptValue *pContext;
        AptNativeString sVar;
        getContext(pObject, pLocalContext->pCurWith, psBuf, &pContext, sVar);
        pInterpreter->setVariable(pContext, pLocalContext->pCurWith, &sVar, NULL, 1, 1, 1);

#if (APT_CURR_DEBUG_MSG_LVL >= APT_DEBUG_MSG_ASSERT_LVL)
        if (pContext != pObject)
        {
            // TODO: Fix data.  Will M. 5/16/12
            // At this point, we know we are attempting to delete pName, and need to delete the member
            // variable from pObject. The above buggy code calls getContext to look in a
            // variety of places for pName, including the _global namespace, just in case it's not in
            // the current context (which is really where it should be)
            //
            // I've noticed this happening when deleting class definitions in Fifa's data.
            // Usually there will be a line in the following format:
            //
            //      delete _global["game.foo.bar"];
            //
            // Where game.foo.bar is the class name. The setVariable would fail and the class definition
            // would not be deleted, creating a zombie.
            // This line should be replaced with the following:
            //
            //      delete eval("game.foo.bar");
            //
            // This will go through _FunctionAptActionDelete2 where the global search will take place,
            // resulting in the delete being successful. Since Fifa does this in many instances,
            // it's not trivial to replace every one in a single pass, and using the correct implementation
            // results in zombies when advancing through kickoff (and most other flows)
            //
            // Therefore, at the moment we are going to let Fifa use the buggy version.  In order to help
            // us find and fix the data, we compare vs. the not-buggy version and assert if we get
            // different behavior.  If we do, then there's potentially a data problem that should be fixed!

            const int ERROR_BUFFER_SIZE = 1024;
            char errorBuffer[ERROR_BUFFER_SIZE];
            SNPRINTF(errorBuffer, ERROR_BUFFER_SIZE, "Found data behavior inconsistency attempting to delete member \"%s\" - see comments in AptActionInterpreter around line %d.", psBuf->c_str(), __LINE__);
            errorBuffer[ERROR_BUFFER_SIZE - 1] = '\0';
            APT_FAIL(errorBuffer);
        }
#endif
#endif
    }

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(AptInteger::Create(1)); // TODO this is the success of the delete, which we always assume succeeded (it shouldn't for, say, "delete _x;" in a MC)
}

void AptActionInterpreter::_FunctionAptActionDelete2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pName = pInterpreter->stackAt(0);

    TO_STRING(pName, psBuf);

    // ActionDelete2 deletes a named property. Flash Player first looks for the property in the
    // current scope, and if the property cannot be found, continues to search in the encompassing scopes.
    // Flags:
    //   bGlobal=1
    //   bLookInFunctionScope=1
    //   bIsMember=0
    pInterpreter->setVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, psBuf, NULL, 1, 1, 0);

    pInterpreter->stackPop();
    pInterpreter->stackPush(AptInteger::Create(1));
}

void AptActionInterpreter::_FunctionAptActionDefineLocal(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue    = pInterpreter->stackAt(0);
    AptValue *pVariable = pInterpreter->stackAt(1);

    if (pInterpreter->mpCurrentFunction)
    {
        pInterpreter->mpCurrentFunction->SetInLocalScope(pVariable->c_string()->GetInternalString(), pValue);
    }
    else
    {
        pInterpreter->setVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pVariable->c_string()->GetInternalString(), pValue, false);
    }

    pInterpreter->stackPop(2);
}

void AptActionInterpreter::_FunctionAptActionCallFunction(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pName   = pInterpreter->stackAt(0);
    AptValue *pParams = pInterpreter->stackAt(1);

    AptNativeString sVar;               // From pName
    int nParams = pParams->toInteger(); // From pParams

    AptValue *pFunctionValue = 0;
    AptValue *pContext       = 0;

    if (pName->isArray()) // weird Flash syntax: ["my" + var](), which apparently means the same as eval("my"+var)();
    {
        pName = pName->c_array()->get(0);
    }

    if (pName->isString())
    {
        // If we are in a constructor and calling super, we need to use ActionCallMethod instead of ActionCallFunction. ActionCallMethod is
        // designed to run on instanced methods, and also has a lot of special handling for super calls already in place. Usually, the byte
        // code in the data would already be setup to call ActionCallMethod. However, if the swf is published with debugging enabled, we get
        // an ActionCallFunction code here. As far as I can tell, this is a bug in Flash that is hacked in FlashPlayer. So, I am hacking it
        // here also. (xclark)
        if (pName->c_string()->GetInternalString()->Equal(*StringPool::GetString(SC_super)))
        {
            // We need to adjust the stack to be what ActionCallMethod expects it to be.
            pInterpreter->stackPop();                                                                       // Pop the function name
            pInterpreter->stackPush(pInterpreter->thisStack.top()->GetNativeHashVirtual()->Get__Proto__()); // Push the object's prototype
            pInterpreter->stackPush(gpUndefinedValue);                                                      // Push undefined for the function name (this is expected behavior when a constructor calls super)

            _FunctionAptActionCallMethod(pInterpreter, pLocalContext);
            return;
        }

        getContext(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pName->c_string()->GetInternalString(), &pContext, sVar);
        pFunctionValue = pInterpreter->getVariable(pContext, pLocalContext->pCurWith, &sVar, true);
    }
    else
    {
        pFunctionValue = pName;
    }

    APT_INC(pFunctionValue);

    // pop the name and number of parameters off
    pName   = NULL; // After the pop, these may be deleted
    pParams = NULL; // After the pop, these may be deleted
    pInterpreter->stackPop(2);


#ifdef APT_DEBUGGER_ENABLE
    bool isScriptActionFlag = AptDebugger::GetInstance()->PushCallStack(pFunctionValue, sVar.GetBuffer(), pLocalContext->pInstruction, pLocalContext->pCurrentContext);
#endif

    gAptOptCallStack->Push((const char *)sVar.GetBuffer());

#if defined(APT_DEBUG)
    pInterpreter->debugCallStack.Push(new DebugCallStackInfoT(sVar.c_str(), pContext ? pContext : pLocalContext->pCurrentContext, AptActionType_CallFunction));
#endif


    pInterpreter->callFunction(pContext ? pContext : pLocalContext->pCurrentContext, pFunctionValue, nParams, 0, 0, true);

#if defined(APT_DEBUG)
    pInterpreter->debugCallStack.Pop();
#endif


    // this means that current context was removed while executing above actionscript
    // if (pCurrentContext->isUndefined())
    //  APT_ASSERT(!pCurrentContext->isUndefined()) ;

    APT_DEC(pFunctionValue);

    gAptOptCallStack->Pop();

#ifdef APT_DEBUGGER_ENABLE
    if (isScriptActionFlag)
    {
        AptDebugger::GetInstance()->PopCallStack(pFunctionValue);
    }
#endif
}

void AptActionInterpreter::_FunctionAptActionReturn(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pLocalContext->bEncounteredReturn = 1;
}

void AptActionInterpreter::_FunctionAptActionModulo(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    float fFloat1;

    //(1.1:rrv:14/27): AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is undefined, the MOD operation is not defined, so return undefined
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        fFloat1 = pA->toFloat();

        if (fFloat1 == 0.f)
        {
            pResult = gpUndefinedValue;
        }
        else
        {
            pResult = AptFloat::Create(fmodf(pB->toFloat(), fFloat1));
        }
    }
    //(1.1:rrv:14/27): AS2 end

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionNewObject(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    AptValue *pObjectName = pInterpreter->stackAt(0);
    AptValue *pParams     = pInterpreter->stackAt(1);

    int nParams; // From pParams

    TO_STRING(pObjectName, psObject); // From pObjectName
    nParams = pParams->toInteger();

    pObjectName = NULL; // May be deleted after Pop.
    pParams     = NULL; // May be deleted after Pop.
    pInterpreter->stackPop(2);

    AptValue *pObject = pInterpreter->_createObject(pLocalContext->pInstruction, pLocalContext->pCurrentContext, pLocalContext->pCurWith, psObject, nParams);

    if (pObject)
    {
        pInterpreter->stackPush(pObject);
        APT_DEC(pObject); // need to dec because of APT_INC in _createObject.
    }
    else
    {
        pInterpreter->stackPush(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionDefineLocal2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    AptValue *pVariable    = pInterpreter->stackAt(0);
    AptNativeString *pName = pVariable->c_string()->GetInternalString();
    AptValue *pOldValue    = gpUndefinedValue;

    if (pInterpreter->mpCurrentFunction)
    {
        AptScriptFunctionBase *pFunc = pInterpreter->mpCurrentFunction;

        if (!pFunc->ExistsInLocalScope(pName))
        {
            pFunc->SetInLocalScope(pName, gpUndefinedValue); // Variable was being set to variable name; should be set to undefined by default.
        }
    }
    else
    {
        pOldValue = pInterpreter->getVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pName, false);
        if (pOldValue->isUndefined())
        {
            pInterpreter->setVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pName, gpUndefinedValue, false);
        }
    }
    pInterpreter->stackPop();
}

void AptActionInterpreter::_FunctionAptActionInitArray(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pNumElements = pInterpreter->stackAt(0);
    int nNumElements       = pNumElements->toInteger();

    pNumElements = NULL; // May be deleted in pop.

    pInterpreter->stackPop();

    // call _createObject() to create the array but pass it 0 parameters
    // to prevent it from behaving as if a single numerical element is passed to Array constructor
    // (it will interpret 'var a = [ 20 ]' as 'new Array(20)' which is incorrect in the context
    // of 'initArray', but correct in the context of 'new' opcode
    AptValue *pArray = pInterpreter->_createObject(pLocalContext->pInstruction,
                                                   pLocalContext->pCurrentContext,
                                                   pLocalContext->pCurWith,
                                                   StringPool::GetString(SC_Array),
                                                   0);

    if (pArray)
    {
        AptArray *_pArray = static_cast<AptArray *>(pArray);

        for (int i = 0; i < nNumElements; ++i)
        {
            _pArray->set(i, pInterpreter->stackAt(i));
        }

        pInterpreter->stackSafePop(nNumElements);
        pInterpreter->stackPush(pArray);
        APT_DEC(pArray);
    }
    else
    {
        pInterpreter->stackSafePop(nNumElements);
        pInterpreter->stackPush(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionInitObject(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pNumElements = pInterpreter->stackAt(0);
    int nNumElements       = pNumElements->toInteger();

    pNumElements = NULL; // May be deleted after Pop.
    pInterpreter->stackPop();

    AptValue *pObject = pInterpreter->_createObject(pLocalContext->pInstruction, pLocalContext->pCurrentContext, pLocalContext->pCurWith, StringPool::GetString(SC_Object));

    if (pObject)
    {
        // The InitObject action should never be invoked for an
        // AptExtObject.  But just to be safe, we add some checks below.
        // _createObject only returns AptObject* or AptExtObject*,
        // so if pObject is not an AptExtObject*, then it's safe to
        // cast as an AptObject* below.  And if for some strange reason
        // we do have an AptExtObj here, we just basically do nothing.

        bool bIsAptExtObject = (pObject->getVtblIndex() == AptVFT_Extension);
        APT_ASSERT(!bIsAptExtObject);
        if (!bIsAptExtObject)
        {
            for (int i = 0, stkReg = 0; i < nNumElements; i++, stkReg += 2)
            {
                AptValue *pValue = pInterpreter->stackAt(stkReg);
                AptValue *pName  = pInterpreter->stackAt(stkReg + 1);
                TO_STRING(pName, psBuf);
                static_cast<AptObject *>(pObject)->Set(psBuf, pValue);
            }
        }

        pInterpreter->stackPop(2 * nNumElements);
        pInterpreter->stackPush(pObject);
        APT_DEC(pObject); // need to dec because of APT_INC in _createObject.
    }
    else
    {
        pInterpreter->stackSafePop(2 * nNumElements);
        pInterpreter->stackPush(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionTypeOf(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue   = pInterpreter->stackAt(0);
    AptString *pString = AptString::Create();

    if (pValue->getIsDefined())
    {
        if (pValue->isInteger() || pValue->isFloat())
            pString->cpy(*StringPool::GetString(SC_Number));
        else if (pValue->isBoolean())
            pString->cpy(*StringPool::GetString(SC_Boolean));
        else if (pValue->isString())
            pString->cpy(*StringPool::GetString(SC_String));
        else if (pValue->isObject() || pValue->isArray())
            pString->cpy(*StringPool::GetString(SC_Object));
        else if (pValue->isCIH())
        {
            if (pValue->c_cih()->IsSpriteInst() || pValue->c_cih()->IsAnimationInst()) // ### Only SpriteInst are MovieClips
            {
                pString->cpy(*StringPool::GetString(SC_MovieClip));
            }
            else if (pValue->c_cih()->IsLevelInst())
            {
                pString->cpy(*StringPool::GetString(SC_Undefined));
            }
            else
            {
                pString->cpy(*StringPool::GetString(SC_Object));
            }
        }
        else if (pValue->isNone())
            pString->cpy(*StringPool::GetString(SC_null));
        else if (pValue->isUndefined())
            pString->cpy(*StringPool::GetString(SC_Undefined));
        else if (pValue->isFunction())
            pString->cpy(*StringPool::GetString(SC_Function));
    }
    else
    {
        pString->cpy(*StringPool::GetString(SC_Undefined));
    }
    pInterpreter->stackPop();
    pInterpreter->stackPush(pString);
}

void AptActionInterpreter::_FunctionAptActionTargetPath(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptNativeString sBuf;
    AptValue *pObject = pInterpreter->stackAt(0);
    AptValue *pContext;

    valueToObject(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pObject, &pContext);

    if (pContext)
    {
        if (pContext->isCIH())
        {
            getName(pContext->c_cih(), sBuf);
        }
        else
        {
            sBuf = "";
        }

        AptString *pTargetString = AptString::Create();
        pTargetString->str       = sBuf;

        pInterpreter->stackPop();
        pInterpreter->stackPush(pTargetString);
    }
    else
    {
        pInterpreter->stackPop();
        pInterpreter->stackPush(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionEnumerate(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->_doEnumerate(pLocalContext->pCurrentContext, pLocalContext->pCurWith);
}

void AptActionInterpreter::_FunctionAptActionAdd2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    // stupid instruction that's sometimes Add, and sometimes StringAdd.. why they didn't remove some, we're not sure..
    AptValue *pA    = pInterpreter->stackAt(0);
    AptValue *pB    = pInterpreter->stackAt(1);
    int nSwfVersion = AptGetSwfVersion();

    if (pA->isString() || pB->isString())
    {
        AptString *pBA = _concatAsStrings(pA, pB);
        pInterpreter->stackPop(2);
        pInterpreter->stackPush(pBA);
    }
    else if ((pA->isInteger() || pB->isInteger()) && (!pA->isFloat() && !pB->isFloat()))
    {
        //(1.1:rrv:16/27): AS2 begin
        //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
        //: for AS2, if value is undefined, the operation is undefined
        if (nSwfVersion >= 7)
        {
            if (pA->isUndefined() || pB->isUndefined())
            {
                pInterpreter->stackPop(2);
                pInterpreter->stackPush(gpUndefinedValue);
                return;
            }
        }
        //(1.1:rrv:16/27): AS2 end
        int nInt1 = pA->toInteger();
        int nInt2 = pB->toInteger();
        pInterpreter->stackPop(2);
        pInterpreter->stackPush(AptInteger::Create(nInt1 + nInt2));
    }
    else
    {
        if (nSwfVersion >= 7)
        {
            if (pA->isUndefined() || pB->isUndefined())
            {
                pInterpreter->stackPop(2);
                pInterpreter->stackPush(gpUndefinedValue);
                return;
            }
        }
        float fFloat1 = pA->toFloat();
        float fFloat2 = pB->toFloat();
        pInterpreter->stackPop(2);
        pInterpreter->stackPush(AptFloat::Create(fFloat1 + fFloat2));
    }
}

void AptActionInterpreter::_FunctionAptActionLessThan2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA = pInterpreter->stackAt(0);
    AptValue *pB = pInterpreter->stackAt(1);
    int nResult  = 0;
    //(1.1:rrv:17/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is 'undefined', return 'undefined'
        if (pA->isUndefined() || pB->isUndefined())
        {
            pInterpreter->stackPop(2);
            pInterpreter->stackPush(gpUndefinedValue);
            return;
        }
    }
    //(1.1:rrv:17/27): AS2 end

    if (pA->isString() && pB->isString())
    {
        // 1 of 2 - Added in Release 17.00 - Function was doing a case insensitive compare (Bad)
        // Flash is case sensitive in Less then  / greater then / equal to
        nResult = strcmp(pB->c_string()->GetInternalString()->c_str(), pA->c_string()->GetInternalString()->c_str()) < 0;
    }
    else if (_isNaN(pA) || _isNaN(pB))
    {
        pInterpreter->stackPop(2);
        pInterpreter->stackPush(gpUndefinedValue);
        return;
    }
    else if (pA->isFloat() || pB->isFloat())
    {
        nResult = (pB->toFloat() < pA->toFloat());
    }
    else
    {
        nResult = (pB->toInteger() < pA->toInteger());
    }
    AptBoolean *pResult = AptBoolean::Create(nResult != 0);
    pInterpreter->stackPop(2);
    pInterpreter->stackPush(pResult);
}

void AptActionInterpreter::_FunctionAptActionEquals2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA = pInterpreter->stackAt(0);
    AptValue *pB = pInterpreter->stackAt(1);
    int nResult  = 0;

#if defined(APT_EQUALITY_TYPECHECK_ASSERT)
    APT_ASSERT(ComparisonTypeCheck(pA, pB) && "types are not comparable with ==");
#endif

    if (pA->isCIH() && pA->c_cih()->IsLevelInst())
    {
        pA = gpUndefinedValue;
    }
    if (pB->isCIH() && pB->c_cih()->IsLevelInst())
    {
        pB = gpUndefinedValue;
    }

    //(1.1:rrv:18/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is 'undefined'...
        if (pA->isUndefined())
        {
            nResult++;
        }
        if (pB->isUndefined())
        {
            nResult++;
        }
        if (nResult > 0)
        {
            // if nResult == 1: false
            // if nResult == 2: true
            pInterpreter->stackPop(2);
            pInterpreter->stackPush(AptBoolean::Create(nResult == 2));
            return;
        }
    }
    //(1.1:rrv:18/27): AS2 end

    if (((pA->isInteger() || pA->isFloat() || pA->isBoolean() || pA->isString()) &&
         (pB->isInteger() || pB->isFloat() || pB->isBoolean() || pB->isString())) ||
        (pA->getVtblIndex() == pB->getVtblIndex()))
    {
        if (pA->isUndefined()) // for Null and Undefined return true
        {
            nResult = 1;
        }
        else if (pA->isInteger() && pB->isInteger()) // both are ints
        {
            nResult = (pA->toInteger() == pB->toInteger());
        }
        else if (pA->isFloat() && pB->isFloat()) // both are floats
        {
            nResult = FLOAT_EQUALS(pA->toFloat(), pB->toFloat());
        }
        else if (pA->isString() && pB->isString()) // both are strings
        {
            if (*pB->c_string()->GetInternalString() == *pA->c_string()->GetInternalString())
            {
                nResult = 1;
            }
        }
        else if (((pA->isInteger() || pA->isFloat()) && !_isNaN(pB)) || ((pB->isInteger() || pB->isFloat()) && !_isNaN(pA)))
        {
            // if either are Numbers check equality
            bool fStrAIsFloat = false;
            bool fStrBIsFloat = false;
            if (pA->isString() || pA->isFloat()) // check if pA is float
            {
                if (pA->isFloat() || (pA->c_string()->GetInternalString()->Find('.') != -1))
                {
                    fStrAIsFloat = true;
                }
            }
            if (pB->isString() || pB->isFloat()) // check if pB is float
            {
                if (pB->isFloat() || (pB->c_string()->GetInternalString()->Find('.') != -1))
                {
                    fStrBIsFloat = true;
                }
            }
            if (pA->isInteger())
            {
                int nA = pA->toInteger();
                if (fStrBIsFloat)
                {
                    float fB = pB->toFloat();
                    nResult  = fabsf(nA - fB) < 0.001f;
                }
                else
                {
                    int nB  = pB->toInteger();
                    nResult = (nA == nB);
                }
            }
            else if (pB->isInteger())
            {
                int nB = pB->toInteger();
                if (fStrAIsFloat)
                {
                    float fA = pA->toFloat();
                    nResult  = fabsf(fA - nB) < 0.001f;
                }
                else
                {
                    int nA  = pA->toInteger();
                    nResult = (nA == nB);
                }
            }
            else
            {
                float fA = pA->toFloat();
                float fB = pB->toFloat();
                nResult  = fabsf(fA - fB) < 0.001f;
            }
        }
        else // neither are numbers
        {
            if (pA->isString() && !pB->isBoolean())
            {
                TO_STRING(pA, psAString);
                TO_STRING(pB, psBString);

                nResult = *psAString == *psBString ? 1 : 0;
            }
            // added additional clause for testing of boolean against number
            else if ((pA->isBoolean() && !pB->isString()) || (pB->isBoolean() && !pA->isString()))
            {
                nResult = pA->toInteger() == pB->toInteger() ? 1 : 0;
            }
            else
            {
                nResult = (pA == pB); // here we are talking objects of some sort, so check if same object
            }
        }
    }
    else
    {
        if (pA->isUndefined() && pB->isUndefined()) // if both are undef, then they're equal
        {
            nResult = 1;
        }
    }

    pInterpreter->stackPop(2);
    pInterpreter->stackPush(AptBoolean::Create(nResult != 0));
}

void AptActionInterpreter::_FunctionAptActionToNumber(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue = pInterpreter->stackAt(0);
    if (!pValue->isFloat() && !pValue->isInteger())
    {
        AptValue *pNew = gpUndefinedValue;
        if (!_isNaN(pValue)) // Checks for isNaN first, if true, return undef
        {
            // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
            if (AptGetSwfVersion() >= 7)
            {
                if (pValue->isUndefined())
                {
                    pInterpreter->stackPop();
                    pInterpreter->stackPush(pNew);
                    return;
                }
            }

            TO_STRING(pValue, psBuf);

            /// ###
            // If the '.' is the last char, then use an integer.
            int place = psBuf->UTF8_Find(".");
            if (place != -1 && place != psBuf->Size())
            {
                pNew = AptFloat::Create(pValue->toFloat());
            }
            else
            {
                pNew = AptInteger::Create(pValue->toInteger());
            }
        }
        pInterpreter->stackPop();
        pInterpreter->stackPush(pNew);
    }
    else
    {
        // do not push or pop anything since top of the stack already is a number
    }
}

void AptActionInterpreter::_FunctionAptActionToString(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue = pInterpreter->stackAt(0);
    if (!pValue->isString())
    {
        AptString *pStr = AptString::Create();
        pValue->Append_ToString(pStr->str);
        pInterpreter->stackPop();
        pInterpreter->stackPush(pStr);
    }
    else
    {
        // do not push or pop anything since top of stack is already a string
    }
}

void AptActionInterpreter::_FunctionAptActionPushDuplicate(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue = pInterpreter->stackAt(0);
    pInterpreter->stackPush(pValue);
}

void AptActionInterpreter::_FunctionAptActionStackSwap(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue1 = pInterpreter->stackAt(0);
    AptValue *pValue2 = pInterpreter->stackAt(1);

    // $TempSave
    pInterpreter->stackPopNoDec();
    pInterpreter->stackPopNoDec();
    pInterpreter->stackPushNoInc(pValue1);
    pInterpreter->stackPushNoInc(pValue2);
}

void AptActionInterpreter::_FunctionAptActionGetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pName   = pInterpreter->stackAt(0);
    AptValue *pObject = pInterpreter->stackAt(1);

    if (pObject->isUndefined() || pName->isUndefined())
    {
        pInterpreter->stackPop(2);
        pInterpreter->stackPush(gpUndefinedValue);
    }
    else if (pObject->isArray() && (pName->isInteger() || pName->isFloat()))
    {
        AptArray *pArray = pObject->c_array();
        AptValue *pValue = pArray->get(pName->toInteger());

        pInterpreter->stackPopAndPush(2, pValue);
    }
    else if (pObject->isExtern())
    {
        AptValue *pValue = AptGetUserFuncs().pfnGetExternVariable(pName->c_string()->GetInternalString()->c_str());
        pInterpreter->stackPopAndPush(2, pValue);
    }
    else
    {
        TO_STRING(pName, psBuf);

        AptValue *pValue = pInterpreter->getVariable(pObject, NULL, psBuf, true, false, true); // 5/8

        // APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "getMember: %s  --> %p\n", pStr->ConstRawPtr(), pValue);

        pInterpreter->stackPopAndPush(2, pValue);
    }
}

void AptActionInterpreter::_FunctionAptActionSetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pValue  = pInterpreter->stackAt(0);
    AptValue *pName   = pInterpreter->stackAt(1);
    AptValue *pObject = pInterpreter->stackAt(2);

    if (pObject->isArray() && (pName->isInteger() || pName->isFloat()))
    {
        AptArray *pArray = pObject->c_array();
        pArray->set(pName->toInteger(), pValue);
    }
    else if (pObject->ContainsNativeHashVirtual() || pObject->isCIH())
    {
        if (pName->isString())
        {
            AptNativeString *sBuf = pName->c_string()->GetInternalString();
            pInterpreter->setVariable(pObject, pLocalContext->pCurWith, sBuf, pValue, true, false, true); // 7/8

#if APT_GET_SET_MEMBER_CHECK
            // Added check to make sure that what is written can be read back (has already found a couple bugs for me)
            // Watchout this will fire on object members that are not supported. Since there is no
            AptValue *pVerify = pInterpreter->getVariable(pObject, pLocalContext->pCurWith, sBuf, true, false, true); // 8/8
            if (!(pObject->isCIH() && pObject->c_cih()->IsLevelInst()) && pValue != gpUndefinedValue && pVerify == gpUndefinedValue)
            {
                // This just checks that any variable set into an object can be retrieved.
                // If you get this assert then either
                //   a: you are setting a new member to something that does not accept new members in Apt (i.e. assning custom members to a TextField or Shape Object)
                //   b: you are setting a value to an unsupported member of an object.
                //   c: there is an Apt bug preventing the member from being read.
                APT_ASSERT(0 && "Setting Member variable that cannot be retrieved.");
            }
#endif
            // Super Fix:  Since you can change a class hierarchy by simply changing an objects __proto__ property, we need to check here if this is done.
            if ((*sBuf == *StringPool::GetString(SC___proto__)) && (pObject->isObject() || pObject->isCIH()))
            {
                // Added common interface to "hasClass"
                pObject->SetHasClass(1);
            }
        }
        else
        {
            AptNativeString sBuf;
            pName->toString(sBuf);
            pInterpreter->setVariable(pObject, pLocalContext->pCurWith, &sBuf, pValue, true, false, true);

            // APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "setMember: %s  <-- %p\n", sBuf.ConstRawPtr(), pValue );

#if APT_GET_SET_MEMBER_CHECK
            // Added check to make sure that what is written can be read back (has already found a couple bugs for me)
            // Watchout this will fire on object members that are not supported. Since there is no
            AptValue *pVerify = pInterpreter->getVariable(pObject, pLocalContext->pCurWith, &sBuf, true, false, true); // 8/8
            if (!(pObject->isCIH() && pObject->c_cih()->IsLevelInst()) && pValue != gpUndefinedValue && pVerify == gpUndefinedValue)
            {
                // This just checks that any variable set into an object can be retrieved.
                // If you get this assert then either
                //   a: you are setting a new member to something that does not accept new members in Apt (i.e. assning custom members to a TextField or Shape Object)
                //   b: you are setting a value to an unsupported member of an object.
                //   c: there is an Apt bug preventing the member from being read.
                APT_ASSERT(0 && "Setting Member variable that cannot be retrieved.");
            }
#endif
            // Super Fix:  Since you can change a class hierarchy by simply changing an objects __proto__ property, we need to check here if this is done.
            if ((sBuf == *StringPool::GetString(SC___proto__)) && (pObject->isObject() || pObject->isCIH()))
            {
                // Added common interface to "hasClass"
                pObject->SetHasClass(1);
            }
        }
    }
    else if (pObject->isExtern())
    {
        TO_STRING(pValue, psBuf);
        AptGetUserFuncs().pfnSetExternVariable(*pName->c_string()->GetInternalString(), *psBuf);
    }

#if defined(APT_USE_DEBUG_NAMES)
    // We are having a very difficult time debugging "Delegate" functions - that is,
    // anonymous functions that call into member functions with "Function.call." In such
    // cases, the profiler/debug callstack would output "call" instead of a meaningful name.
    // So instead, // we're working around it by setting a debug name on the function.
    if (pValue->isScriptFunction())
    {
        if (pName->isString())
        {
            // We are setting a ScriptFunction2 to be a member of something, and giving it a name.
            // This is actually a frequent occurrence - it happens for any class with a function in it.
            const AptNativeString *nameKeyString = StringPool::GetString(SC__debugName);
            AptNativeHash *pFuncHash             = pValue->GetNativeHashVirtual();
            AptValue *currentName                = pFuncHash->Lookup(nameKeyString);
            if (NULL == currentName)
            {
                pFuncHash->Set(nameKeyString, pName);
            }
        }
    }
#endif

    pInterpreter->stackPop(3);

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing it here
        //  we avoid to call this function if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionIncrement(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pNumber = pInterpreter->stackAt(0);
    AptValue *pNew    = NULL;

    //(1.1:rrv:19/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if value is 'undefined'...
        if (pNumber->isUndefined())
        {
            pNew = gpUndefinedValue;
        }
    }

    if (pNew == NULL)
    {
        if (pNumber->isInteger())
        {
            pNew = AptInteger::Create(pNumber->toInteger() + 1);
        }
        else
        {
            pNew = AptFloat::Create(pNumber->toFloat() + 1.f);
        }
    }
    //(1.1:rrv:19/27): AS2 end

    pInterpreter->stackPop();
    pInterpreter->stackPush(pNew);
}

void AptActionInterpreter::_FunctionAptActionDecrement(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pNumber = pInterpreter->stackAt(0);
    AptValue *pNew    = NULL;

    //(1.1:rrv:20/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if value is 'undefined'...
        if (pNumber->isUndefined())
        {
            pNew = gpUndefinedValue;
        }
    }

    if (pNew == NULL)
    {
        if (pNumber->isInteger())
        {
            pNew = AptInteger::Create(pNumber->toInteger() - 1);
        }
        else
        {
            pNew = AptFloat::Create(pNumber->toFloat() - 1.f);
        }
    }
    //(1.1:rrv:20/27): AS2 end

    pInterpreter->stackPop();
    pInterpreter->stackPush(pNew);
}

#if defined APT_AS_DEBUGGING
static uint32_t bAptPrintASFunctionName = 0;
static uint32_t nAptExtFunctionCalls    = 0;
static uint32_t nAptCallmethodCalls     = 0;
#endif
void AptActionInterpreter::_FunctionAptActionCallMethod(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pFunction        = pInterpreter->stackAt(0);
    AptValue *pObject          = pInterpreter->stackAt(1);
    AptValue *pParams          = pInterpreter->stackAt(2);
    int nParams                = pParams->toInteger();
    AptNativeString *sFunction = NULL;
#if defined APT_AS_DEBUGGING
    nAptCallmethodCalls++;
#endif
    bool bPushed             = false;
    AptValue *pFunctionValue = NULL;
    AptValue *pOverrideThis  = NULL;
    AptValue *pOverrideSuper = NULL;
    AptNativeString sBuf;
    // when definefunction2 is used and super() is called in that case pFunction is passed as
    // undefined value and so ignore it and execute the the pObject function itself so added
    // this extra check.
    if (pFunction == gpUndefinedValue)
    {
        // this assignment is not actually needed as in this case we will not be calling
        // getvariable on sFunction
        // sFunction = "super" ;
        sFunction = (AptNativeString *)StringPool::GetString(SC_super);
        // If we are here the compiler wants to run the super constructor. Setup the function value
        // with what is stored in the prototype.
        if (pObject->isPrototype())
        {
            pFunctionValue = pObject->c_prototype()->GetSuperConstructor();

            // The super constructor must use the actual object being constructed
            // as its context-- not the prototype.
            // So, if a member is set in the super constructor, it will be
            // set for the object instance, not as a "static" class member.

            bool bStackEmpty;
            bStackEmpty = (pInterpreter->createdObjectsStack.size() == 0);
            APT_ASSERT(!bStackEmpty);
            if (!bStackEmpty)
            {
                pOverrideThis = pInterpreter->createdObjectsStack.top();
                if (!pOverrideThis->isObject())
                {
                    pOverrideThis = NULL;
                }
            }
        }
        else
        {
            pFunctionValue = pObject;
        }
    }
    else
    {
        // pFunction->toString(sFunction);
    }

    if (!pObject->isUndefined())
    {
        AptValue *pObjectSave = pObject;
        AptValue *pParamSave  = pParams;

        pInterpreter->thisStack.push(pLocalContext->pCurrentContext);

        if (pFunctionValue == NULL || pFunctionValue == gpUndefinedValue)
        {
            if (pFunction->isString())
            {
                sFunction = pFunction->c_string()->GetInternalString();
                if (pObject->getVtblIndex() == AptVFT_Extension)
                {
                    // we do not have to do all the work getVariable in this case, as there is no AS inheritance in this case
                    // also no need to do findchild, getcontext that is done inside getVariable-> We can directly do a
                    // Hashlookup to see if the function exists inside AptExtObject or not.
                    AptNativeHash *pNativeHash = pObject->GetNativeHashVirtual();
                    APT_ASSERT(pNativeHash != NULL);
                    pFunctionValue = pNativeHash->Lookup(sFunction);
#if defined APT_AS_DEBUGGING
                    nAptExtFunctionCalls++;
#endif
                }
                else
                {
#if !defined(APT_ALLOW_GLOBAL_MEMBER_FUNCTION_LOOKUP)
                    // we are calling a method, so we pass bIsMember=1
                    pFunctionValue = pInterpreter->getVariable(pObject, NULL, sFunction, 1, 1, 1);
#else
                    // This call is buggy - please see below comments
                    pFunctionValue = pInterpreter->getVariable(pObject, NULL, sFunction, 1, 1, 0);

#if (APT_CURR_DEBUG_MSG_LVL >= APT_DEBUG_MSG_ASSERT_LVL)
                    // TODO: Fix data.  Colin C. 12/2/11
                    // At this point, we know we are in a member lookup, and need to get the Function
                    // variable out of pObject. The above buggy code is passing "0" for the "bIsMember" parameter
                    // when calling the getVariable() call (see above).  This means getVariable() will look in a
                    // variety of places for sFunction, including the _global namespace.
                    //
                    // The first place I noticed this being problematic was in a case in Fifa for which the code
                    // looked similar to this:
                    //     _global.MenuTransition = <something>
                    //     trace(_level0.MenuTransition);
                    //     _level0.MenuTransition();
                    //
                    // This code would trace "undefined" but then go ahead and call the MenuTransition() function
                    // anyway.  This is incorrect behavior.
                    //
                    // Unfortunately, Fifa's data *relies* on this incorrect behavior.  It looks like the reliance
                    // is pretty heavy, as there are lots of _global functions in Fifa's data.  With the bugfix, I
                    // cannot get into a screen past the main menu without asserting, and cannot get into a game
                    // without crashing.
                    //
                    // Therefore, at the moment we are going to let Fifa use the buggy version.  In order to help
                    // us find and fix the data, I will also compare vs. the not-buggy version and see if we get
                    // differnet behavior.  If we do, then there's potentially a data problem that should be fixed!
                    AptValue *bugfixedFunction = pInterpreter->getVariable(pObject, NULL, sFunction, 1, 1, 1);
                    if (pFunctionValue != bugfixedFunction)
                    {
                        const int ERROR_BUFFER_SIZE = 1024;
                        char errorBuffer[ERROR_BUFFER_SIZE];
                        SNPRINTF(errorBuffer, ERROR_BUFFER_SIZE, "Found data behavior inconsistency attempting to call function \"%s\" - see comments in AptActionInterpreter around line %d.", sFunction->c_str(), __LINE__);
                        errorBuffer[ERROR_BUFFER_SIZE - 1] = '\0';
                        APT_FAIL(errorBuffer);
                    }
#endif
#endif
                }
#if defined APT_AS_DEBUGGING && defined APT_DECOUPLED_RENDERING
                if (bAptPrintASFunctionName)
                {
                    // using printf as it will be available in release/ship modes also.
                    printf("%s -> %s\n", pLocalContext->pParentCharacter->GetAnimationInst()->GetCharacterConst()->m_pAnimFile->GetName().c_str(), sFunction->c_str());
                }
#endif
            }
            else
            {
                pFunction->toString(sBuf);
                pFunctionValue = pInterpreter->getVariable(pObject, NULL, &sBuf);
                sFunction      = &sBuf;
#if defined APT_AS_DEBUGGING && defined APT_DECOUPLED_RENDERING
                if (bAptPrintASFunctionName)
                {
                    // using printf as it will be available in release/ship modes also.
                    printf("%s -> %s\n", pLocalContext->pParentCharacter->GetAnimationInst()->GetCharacterConst()->m_pAnimFile->GetName().c_str(), sBuf.c_str());
                }
#endif
            }
        }

        bool functionIsNative = pFunction->isNativeFunction();

        // pop everything but parameters
        pFunction = NULL; // May be deleted after Pop.

        // $TempSave
        pInterpreter->stackPop();
        pInterpreter->stackPopNoDec(); // Don't Decrement pObject (Saved to pObjectSave), We will do this at the end.
        pInterpreter->stackPopNoDec(); // Don't Decrement pParams (Saved to pParamSave ), We will do this at the end.


        // This is newly added for [ ] apply and call support in release 0.15.00
        if (!pFunctionValue || pFunctionValue->isUndefined())
        {
            APT_ASSERT(sFunction);
            // function is not found so check if it's name is 'apply' or 'call'
            if ((sFunction->EqualNoCase("apply")) || (sFunction->EqualNoCase("call")))
            {
                // this is a case of apply or call
                pFunctionValue = pObject; // pObject is the actual function we want to execute


                // now check if 2 param on stack is a object or a integer telling number of params.
                if (pParams->isInteger() || pParams->isFloat())
                {
                    // also we need to remove the extra 'this' parameter sent to function
                    if (nParams > 0)
                    {
                        AptValue *pThisObj = pInterpreter->stackAt(0);
                        // no need to push extra 'this' object on thisStack
                        if (!pThisObj || pThisObj->isUndefined())
                        {
                            // pObject = pInterpreter->thisStack.at(0);    // now we have to change pObject to this.
                            pObject = gpUndefinedValue; // user really want a null as 'this' obj
                        }
                        else
                        {
                            pObject = pThisObj;
                        }
                        // pop of that 'thisObj' and also decrement the number of params to function
                        pInterpreter->stackPop();
                        nParams--;
                    }
                    else
                    {
                        pObject = gpUndefinedValue; // user really want a null as 'this' obj
                    }
                    // everything is set to go now
                }
                else
                {
                    // 3rd parameter is not a integer but an object itself so params is next parameter.
                    pObject                 = pParams;
                    AptValue *pActualParams = pInterpreter->stackAt(0);
                    nParams                 = pActualParams->toInteger();
                    if (nParams > 1)
                    {
                        AptValue *pThisObj = pInterpreter->stackAt(1);
                        if (!pThisObj || pThisObj->isUndefined())
                        {
                            // do not change pObject now as the 'thisObj' specified as first param is null
                            // so pObject will be pushed as this later in callFunction.
                            // pObject = pInterpreter->thisStack.at(0);    // now we have to change pObject to this.
                        }
                        else
                        {
                            // here also we do not have to push pThisObj on to thisStack even if this is specifically
                            // specified by user to be used as 'this' inside the object.
                            // just by setting pObject will take care of that later in CallFunction.
                            pObject = pThisObj;
                        }

                        // pop of params and that 'thisObj' and also decrement the number of params to function
                        pInterpreter->stackPop();
                        nParams--;
                    }
                    pInterpreter->stackPop();
                }
                // now if the function is 'apply' then separate out the array into individual parameters
                if (sFunction->EqualNoCase("apply"))
                {
                    AptValue *pValue1 = pInterpreter->stackAt(0);
                    if (pValue1->isArray())
                    {
                        pInterpreter->stackPop();
                        AptArray *pArrayObj = pValue1->c_array();
                        int nLength         = pArrayObj->length();
                        nParams             = (nParams - 1) + nLength;
                        for (int i = nLength - 1; i >= 0; i--)
                        {
                            pInterpreter->stackPush(pArrayObj->GetAt(i));
                        }
                    }
                }
            }
            // otherwise just continue as normal.
        }

        // super fix code block
        // Added common interface to "hasClass"
        if (pObject->GetHasClass())
        {
            // Now, if pObject is either an object or a sprite and has class hierarchy, we do this....
            bool bIsGood            = true;
            AptValue *pTmpPushValue = pObject;

            // If the current object is the super of the current context, then we have to get the "this" context of the super
            if (pInterpreter->withStack.size() == 0)
            {
                if (pTmpPushValue == pLocalContext->pSuper)
                {
                    pTmpPushValue = pInterpreter->getVariable(pLocalContext->pCurrentContext, NULL, StringPool::GetString(SC_this));
                }
            }
            // If the withStack is zero, no need to make these checks, just put it on top already.
            else if (pInterpreter->withStack.size() != 0)
            {
                AptValue *pWithStkTop = pInterpreter->withStack.top();
                if (pTmpPushValue->isCIH() && pTmpPushValue->c_cih()->GetDisplayListParent() == pWithStkTop)
                {
                    // Do nothing
                }
                else if (pTmpPushValue != pWithStkTop)
                {
                    // we need to make sure that the current object is not in the class hierarchy of the guy on top of the withStack,
                    //  if it is, then we want to keep the current object on top of the withStack
                    AptNativeHash *pNativeHash = pWithStkTop->GetNativeHashVirtual();

                    while (pNativeHash)
                    {
                        AptValue *pValue = pNativeHash->Get__Proto__();
                        if (pValue && pValue == pTmpPushValue)
                        {
                            bIsGood = false;
                            break;
                        }
                        pNativeHash = pValue ? pValue->GetNativeHashVirtual() : 0;
                    }
                }
            }

            if (bIsGood == true)
            {
                bPushed = true;
                if (pTmpPushValue->isObject())
                {
                    pTmpPushValue->c_object()->setInMainInst(1);
                }
                pInterpreter->withStack.push(pTmpPushValue);
            }
        }
        // End super code block

        // 02/13/07  EATech 105415:
        //
        // We want to know if sFunction is:
        // 1. Implemented directly by pObject, or
        // 2. A "virtual" method implemented by a superclass of pObject.
        // 3. None of the above.
        //
        // pOverrideSuper is set based on this knowledge:
        //   - In the case of 1 or 3, pOverrideSuper==NULL and we continue on
        //     as usual.

        //   - In the case of 2, it gets interesting.  When we call sFunction,
        //     we'll pass pObject on the stack as the "this" pointer, but we must
        //     also pass a superclass prototype, so that the "super" keyword
        //     works correctly inside the method.  Normally (case 1) we pass in
        //     the superclass prototype of pObject.  However, for case 2, we
        //     need to identify the prototype that actually implements sFunction,
        //     and pass onto the stack the super prototype of that prototype.

        //     The super prototype to be passed onto the stack is returned by
        //     AptScriptFunction2::SetupBeforeExecution(), which is called by
        //     AptActionInterpreter::callFunction().  So for case 2, we
        //     pass a non-NULL pOverrideSuper down to SetupBeforeExecution,
        //     which forces the correct value to be returned.
        //
        //     Addendum from Colin C., 1/4/12 - SetupBeforeExecution is all
        //     well and good for ScriptFunction2s, which use that function to
        //     set things in registers.  But for ScriptFunction1s, (which seem to
        //     only show up for Debug SWFs), we don't have registers and actually
        //     need to properly do a lookup on the "super" keyword.

        APT_ASSERT(sFunction);
        if (!sFunction->Equal(*StringPool::GetString(SC_super)))
        {
            bool bHasMethod = HasMethodImplementation(pObject, sFunction);
            if (!bHasMethod)
            {
                // pObject itself does not implement sFunction.  So we search
                // up the __proto__ chain until we find a prototype that does
                // implement sFunction.
                AptValue *pProto = FindSuperImplementor(pObject, sFunction);
                if (pProto)
                {
                    // pProto implements the given method.  So we use this as the calling
                    // object instead, and pass in an override for the superclass for use
                    // later (it will be set into registers if this function is a ScriptFunction2).
                    // Note that the pObject= part is a sensitive bugfix for a problem
                    // that mostly manifests itself when using fla files with the "Debugging permitted"
                    // box checked, which creates a SWF with slightly different bytecodes
                    // (ScriptFunction rather than ScriptFunction2) than we had tested in Apt previously.
                    // In this case, we end up with functions that push "super" on the stack and
                    // resolve it, but those don't resolve properly because pObject is set to a class
                    // higher in the chain. This would result in calling the same function multiple
                    // times rather than delving into superclasses. Colin C. 1/4/12
                    pOverrideSuper = GetNextProto(pProto);
                    pObject        = pProto;
                }
            }
            else
            {
                // Okay, more patches!  This Object's prototype DOES implement the function.
                // Congratulations!  However, due to some other hacks (search for IsInCtor()),
                // when we execute this function *while in a constructor* we won't actually know
                // the superclass of this function - we will end up determining that it is the same
                // class, and call the function twice.  That's bad.  So instead, in that very specific
                // case, we are going to override the superclass to the next prototype up in the chain.
                // To make this behavior occur, have a class A that extends class B that extends MovieClip,
                // write a function Foo() in class A that calls super.Foo(), and call Foo() in A's
                // constructor. The problem will arise when super.Foo() is called - "super" will end up being
                // set to the object's prototype rather than the Object's prototype's prototype.
                // Colin C. 8/28/12
                if (pObject->isCIH() && pObject->c_cih()->IsInCtor())
                {
                    AptValue *objectClass = pOverrideSuper = GetNextProto(pObject);
                    if (objectClass)
                    {
                        pOverrideSuper = GetNextProto(objectClass);
                    }
                }
            }
        }

        bool bStayAlive = false;
        if (pObject->getRefCount() == 1)
        {
            APT_INC(pObject); // ### Make sure this is not removed before used... for 313
            bStayAlive = true;
        }
#if defined(APT_DEBUG)
        pInterpreter->debugCallStack.Push(new DebugCallStackInfoT(sFunction->c_str(), pObject, AptActionType_CallMethod));
#endif

        gAptOptCallStack->Push((const char *)sFunction->c_str());

#ifdef APT_DEBUGGER_ENABLE
        bool isScriptActionFlag = AptDebugger::GetInstance()->PushCallStack(pFunctionValue, sFunction->c_str(), pLocalContext->pInstruction, pObject);
#endif


        // EATech 109673 and 113825: When super.function appeared in a function called through AptCallFunction,
        // the super in the context was not detected, and function was being called instead of super.function
        // if(pObject == pLocalContext->pSuper)
        //{
        //     AptValue *pBaseClass = FindSuperImplementor(pObject, sFunction);
        //     if ( pBaseClass ) // The condition is also true when sFunction is SC_super. In that case pBaseClass is NULL.
        //     {
        //         pFunctionValue = pInterpreter->getVariable(pBaseClass, 0, sFunction, 1);
        //     }
        // }

        if (pObject->isMCInParentChain())
        {
            // The following super bug code checks if the pObject is the actual MovieClip class prototype,
            //  if so, update pObject to the previous context on thisStack.  This is because we want the context
            //  of the function to run from the actual sprite, not the MovieClip class context

            // use prestored Movieclip.prototype
            if (pObject == gpGlobalMovieclipPrototype)
            {
                pObject = pInterpreter->thisStack.at(1);
            } // ### begin new code for super
            if (pObject == pLocalContext->pSuper)
            {
                // this extra comparison if pFunctionValue is scriptfunction is done as
                // in some weird cases of super along with definefunction2 it is not scriptfunction.
                APT_ASSERT(pFunctionValue);
                if (pFunctionValue && pFunctionValue->isScriptFunction())
                {
                    // Combined Script Functions into one control path.
                    AptScriptFunctionBase *pFuncBase = pFunctionValue->c_scriptfunction();
                    AptCIH *pOldCIH                  = pFuncBase->mpCIH;
                    pFuncBase->mpCIH                 = pLocalContext->pCurrentContext;


                    pInterpreter->callFunction(pObject, pFunctionValue, nParams, NULL, pOverrideSuper, true);


                    pFuncBase->mpCIH = pOldCIH;
                }
                else
                {
                    pInterpreter->callFunction(pObject, pFunctionValue, nParams, NULL, pOverrideSuper, true);
                }
            }
            else
            {
                pInterpreter->callFunction(pObject, pFunctionValue, nParams, NULL, pOverrideSuper, true);
            }
        }
        else
        {
            pInterpreter->callFunction(pObject, pFunctionValue, nParams, pOverrideThis, pOverrideSuper, true);
            if (gAptOptCallStackGetScopeInfo && pOverrideThis)
            {
                AptStackItem &item = gAptOptCallStack->Top();
                if (NULL == item.scope)
                {
                    const char *classname = pOverrideThis->GetClassName();
                    item.scope            = (classname && *classname) ? (const char *)classname : 0;
                }
            }
        }

        // super fix
        if (bPushed == true)
        {
            AptValue *pTmpTopVal = pInterpreter->withStack.top();
            if (pTmpTopVal->isObject())
            {
                pTmpTopVal->c_object()->setInMainInst(0);
            }
            // The following if() block fixes a crash that might be related to class inheritance
            // and/or gotoAndStop. Bug found by NCAA10 where pTmpTopVal was destroyed by withStack.pop(),
            // later causing APT_DEC(pObjectSave) to crash, because pObjectSave == pTmpTopVal. So instead
            // increment the refcount just so the AptValue stays alive long enough to be killed properly.
            // This fix is specialized for when isCIH()==true and GetInRemList()==true because that's the
            // condition under which we saw the bug, and we want to minimize any side effects.
            if (pTmpTopVal->getRefCount() == 1)
            {
                if ((pTmpTopVal == pObjectSave) ||
                    (pTmpTopVal->isCIH() && pTmpTopVal->c_cih()->GetInRemList()))
                {
                    // We don't want withStack.pop() to delete pTmpTopVal because that
                    // would cause APT_DEC(pObjectSave) to double-delete the same object.
                    APT_INC(pTmpTopVal);
                }
            }

            pInterpreter->withStack.pop();
        }

        if (bStayAlive)
        {
            APT_DEC(pObject); // ### Make sure this is removed after used...
        }

#if defined(APT_DEBUG)
        pInterpreter->debugCallStack.Pop();
#endif

        gAptOptCallStack->Pop();

#ifdef APT_DEBUGGER_ENABLE
        if (isScriptActionFlag)
        {
            AptDebugger::GetInstance()->PopCallStack(pFunctionValue);
        }
#endif


        // $hack code start$ Daniel Craig, 2004/04/06.
        // This helps take care of the issue of garbage collecting object after
        // a pop or a shift (before it is pushed onto the stack). I.e. when an
        // object is popped or shifted from an array, the pop / shift code cannot
        // decrement the reference counter until the returned AptValue is placed
        // on the action-script stack. But the action-script stack push occurs
        // outside the function implementations. So to fix this one would
        // probably need to move the stack push of the result to the function
        // implementations themselves instead of doing it in callFunction().
        if (pInterpreter->stack.Top() != gpUndefinedValue && pObject->isArray() && (sFunction->EqualNoCase("pop") || sFunction->EqualNoCase("shift")))
        {
            APT_DEC(pInterpreter->stack.Top());
        }
        // $hack code end$

        pInterpreter->thisStack.pop();

        APT_DEC(pObjectSave); // $TempSave Decrement delayed from before.
        APT_DEC(pParamSave);  // $TempSave Decrement delayed from before.
    }
    else
    {
        // toss all stack + parameters
        pInterpreter->stackPop(nParams + 3);
        // push an undef so there's something for the caller to pop
        pInterpreter->stackPushNoInc(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionNewMethod(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pMethodName       = pInterpreter->stackAt(0);
    AptValue *pScriptObjectName = pInterpreter->stackAt(1);
    AptValue *pParams           = pInterpreter->stackAt(2);
    int nParams;

    TO_STRING(pMethodName, psObject);
    nParams = pParams->toInteger();

    // 1 of 3 - Pop off the 3 items we already got from the stack so it is setup for the params
    pInterpreter->stackPop();      // pop and dec pMethodName - done with it.
    pInterpreter->stackPopNoDec(); // pop pScriptObjectName but don't allow GC to delete it, _createObject uses it.
    pInterpreter->stackPop();      // pop and dec pParams - done with it.

    AptValue *pObject = pInterpreter->_createObject(pLocalContext->pInstruction, pScriptObjectName, pLocalContext->pCurWith, psObject, nParams);

    // 2 of 3 - Decrement pScriptObjectName after using it so GC can delete it.
    APT_DEC(pScriptObjectName);

    // 3 of 3 - push on the object returned or gpUndefinedValue
    if (pObject)
    {
        pInterpreter->stackPush(pObject);
        APT_DEC(pObject); // need to dec because of APT_INC in _createObject.
    }
    else
    {
        pInterpreter->stackPush(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionEnumerate2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->_doEnumerate(pLocalContext->pCurrentContext, pLocalContext->pCurWith);
}

void AptActionInterpreter::_FunctionAptActionBitAnd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:21/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if value is 'undefined'...
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        int nA  = pA->toInteger();
        int nB  = pB->toInteger();
        pResult = AptInteger::Create(nA & nB);
    }
    //(1.1:rrv:21/27): AS2 end

    pInterpreter->stackPopAndPush(2, pResult);
}

void AptActionInterpreter::_FunctionAptActionBitOr(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:22/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if value is 'undefined'...
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        int nA  = pA->toInteger();
        int nB  = pB->toInteger();
        pResult = AptInteger::Create(nA | nB);
    }
    //(1.1:rrv:22/27): AS2 end
    pInterpreter->stackPopAndPush(2, pResult);
}

void AptActionInterpreter::_FunctionAptActionBitXor(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA      = pInterpreter->stackAt(0);
    AptValue *pB      = pInterpreter->stackAt(1);
    AptValue *pResult = NULL;

    //(1.1:rrv:23/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if value is 'undefined'...
        if (pA->isUndefined() || pB->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        int nA  = pA->toInteger();
        int nB  = pB->toInteger();
        pResult = AptInteger::Create(nA ^ nB);
    }
    //(1.1:rrv:23/27): AS2 end

    pInterpreter->stackPopAndPush(2, pResult);
}

void AptActionInterpreter::_FunctionAptActionBitLShift(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pShiftCount = pInterpreter->stackAt(0);
    AptValue *pValue      = pInterpreter->stackAt(1);
    AptValue *pResult     = NULL;

    //(1.1:rrv:24/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is 'undefined'...
        if (pValue->isUndefined() || pShiftCount->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        int nShiftCount = pShiftCount->toInteger();
        int nValue      = pValue->toInteger();
        pResult         = AptInteger::Create(nValue << nShiftCount);
    }
    //(1.1:rrv:24/27): AS2 end

    pInterpreter->stackPopAndPush(2, pResult);
}

void AptActionInterpreter::_FunctionAptActionBitRShift(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pShiftCount = pInterpreter->stackAt(0);
    AptValue *pValue      = pInterpreter->stackAt(1);
    AptValue *pResult     = NULL;

    //(1.1:rrv:25/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if value is 'undefined'...
        if (pValue->isUndefined() || pShiftCount->isUndefined())
        {
            pResult = gpUndefinedValue;
        }
    }

    if (pResult == NULL)
    {
        int nShiftCount = pShiftCount->toInteger();
        int nValue      = pValue->toInteger();
        pResult         = AptInteger::Create(nValue >> nShiftCount);
    }
    //(1.1:rrv:25/27): AS2 end

    pInterpreter->stackPopAndPush(2, pResult);
}

void AptActionInterpreter::_FunctionAptActionBitURShift(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(false); //  Not implemented
}

void AptActionInterpreter::_FunctionAptActionStrictEquals(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA = pInterpreter->stackAt(0);
    AptValue *pB = pInterpreter->stackAt(1);

    int nResult = 0;

    if (pA->isCIH() && pA->c_cih()->IsLevelInst())
    {
        pA = gpUndefinedValue;
    }

    if (pB->isCIH() && pB->c_cih()->IsLevelInst())
    {
        pB = gpUndefinedValue;
    }

    //(1.1:rrv:26/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if value is 'undefined'...
        if (pA->isUndefined())
        {
            nResult++;
        }
        // if value is 'undefined'...
        if (pB->isUndefined())
        {
            nResult++;
        }
        if (nResult > 0)
        {
            // if nResult == 1: false
            // if nResult == 2: true
            pInterpreter->stackPopAndPush(2, AptBoolean::Create(nResult == 2));
            return;
        }
    }
    //(1.1:rrv:26/27): AS2 end

    // original code didn't allow floats and ints to compare which of course Flash allows.. So we need to use this instead.
    if (((pA->isInteger() || pA->isFloat()) && (pB->isInteger() || pB->isFloat())) || pA->getVtblIndex() == pB->getVtblIndex())
    {
        switch (pA->getVtblIndex())
        {
        case AptVFT_StringValue:
        case AptVFT_StringObject:
        {
            if (pB->c_string()->GetInternalString()->Equal(*pA->c_string()->GetInternalString()))
            {
                nResult = 1;
            }
            break;
        }
        case AptVFT_Float:
        {
            float fA = pA->toFloat();
            if (pB->isInteger())
            {
                int nB  = pB->toInteger();
                nResult = fabsf(fA - nB) < 0.001f;
            }
            else
            {
                float fB = pB->toFloat();
                nResult  = fabsf(fA - fB) < 0.001f;
            }
            break;
        }
        case AptVFT_Boolean:
        case AptVFT_Integer:
        {
            int nA = pA->toInteger();
            if (pB->isInteger())
            {
                int nB  = pB->toInteger();
                nResult = (nA == nB);
            }
            else
            {
                float fB = pB->toFloat();
                nResult  = fabsf(nA - fB) < 0.001f;
            }
            break;
        }
        default:
        {
            nResult = (pA == pB);
            break;
        }
        }
    }

    pInterpreter->stackPopAndPush(2, AptBoolean::Create(nResult != 0));
}

void AptActionInterpreter::_FunctionAptActionGreater(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA = pInterpreter->stackAt(0);
    AptValue *pB = pInterpreter->stackAt(1);

    //(1.1:rrv:27/27) AS2 begin
    //: for AS1, value is converted to a number and evaluated therefrom (undef is converted to zero)
    //: for AS2, if value is undefined, the operation is undefined
    // if(7==((AptCharacterAnimationInst*)pLocalContext->pCurrentContext->pData)->getSwfVersion())
    if (AptGetSwfVersion() >= 7)
    {
        // if either value is 'undefined', return 'undefined'
        if (pA->isUndefined() || pB->isUndefined())
        {
            pInterpreter->stackPop(2);
            pInterpreter->stackPush(gpUndefinedValue);
            return;
        }
    }
    //(1.1:rrv:27/27): AS2 end

    int nResult = 0;
    if (pA->isString() && pB->isString())
    {
        // 1 of 2 - Added in Release 17.00 - Function was doing a case insensitive compare (Bad)
        // Flash is case sensitive in Less then  / greater then / equal to
        nResult = strcmp(pA->c_string()->GetInternalString()->c_str(), pB->c_string()->GetInternalString()->c_str()) < 0;
    }
    else if (pA->isFloat() || pB->isFloat()) // added
    {
        nResult = (pB->toFloat() > pA->toFloat());
    }
    else
    {
        nResult = (pB->toInteger() > pA->toInteger());
    }

    pInterpreter->stackPopAndPush(2, AptBoolean::Create(nResult != 0));
}

void AptActionInterpreter::_FunctionAptActionGotoFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptCIH *pCIH = NULL;
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_GotoFrame *pData = (AptAction_GotoFrame *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_GotoFrame);

    // Added support to use the current With (target) if it is out there.
    if (pLocalContext->pCurWith && pLocalContext->pCurWith->isCIH())
    {
        pCIH = pLocalContext->pCurWith->c_cih();
    }
    else if (pLocalContext->pCurrentContext->isCIH())
    {
        pCIH = pLocalContext->pCurrentContext->c_cih();
    }

    if (pCIH && !pCIH->isNone()) // don't do this id gpUndefinedCIH
    {
        pCIH->jumpToFrame(pData->nFrame);
        pCIH->SetIsPlaying(false);
    }

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionGetUrl(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_GetUrl *pData = (AptAction_GetUrl *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_GetUrl);

    if (pInterpreter->isFSCommand(pData->szUrl))
    {
        pInterpreter->doFSCommand(pData->szUrl, pData->szWin);
    }
    else
    {
        GetTargetSim()->GetLinker()->Load(pData->szUrl, AptNativeString(pData->szWin));
    }
}

void AptActionInterpreter::_FunctionAptActionStoreRegister(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);

    const AptAction_StoreRegister *pData = (AptAction_StoreRegister *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_StoreRegister);

    AptScriptFunctionBase::SetRegisterValue(pData->nRegister, pInterpreter->stackAt(0));
}

void AptActionInterpreter::_FunctionAptActionDefineDictionary(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_Push *pData = (AptAction_Push *)pLocalContext->pInstruction; // these actually are _Push; the struct is exactly the same
    pLocalContext->pInstruction += sizeof(AptAction_Push);

    // Overwrite the old pool
    pInterpreter->constantPool = pData->items;
}

void AptActionInterpreter::_FunctionAptActionWaitForFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
}

void AptActionInterpreter::_FunctionAptActionSetTarget(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_SetTarget *pData = (AptAction_SetTarget *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_SetTarget);

    if (pData->szTarget[0] == 0)
    {
        APT_DECSAFE(pLocalContext->pCurWith);
        pLocalContext->pCurWith = NULL;
    }
    else
    {
        APT_ASSERT(!pLocalContext->pCurWith); // Should not have a current Target (With)
        AptNativeString strTarget(pData->szTarget);
        AptValue *pTarget;

        if ((pData->szTarget[0] == '/') || (pData->szTarget[0] == '.'))
        {
            const char *p = pData->szTarget;
            AptCIH *t     = pLocalContext->pCurrentContext;

            while (p[0] == '.' && p[1] == '.' &&
                   t->GetDisplayListParent() != NULL)
            {
                t = t->GetDisplayListParent(); // go up one level for each ".."
                p += 2;                        // Advance passed ".."
            }
            pTarget = t;
        }
        else
        {
            strTarget.TrimRight("/"); // names can have a trailing '/'
            pTarget = getObject(pLocalContext->pCurrentContext, NULL, &strTarget);
        }

        APT_ASSERT(pTarget); // Setting Target to an Object that we can't find!
        pLocalContext->pRemoveWithAt = NULL;
        pLocalContext->pCurWith      = pTarget;
        APT_INC(pLocalContext->pCurWith);
    }
}

void AptActionInterpreter::_FunctionAptActionGotoLabel(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    // APT_ASSERT(false);              //  Not implemented

    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_GotoLabel *pData = (AptAction_GotoLabel *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_GotoLabel);

    AptNativeString pLabelName = pData->szLabel;
    AptCIH *pTarget;

    if (pLocalContext->pCurWith && pLocalContext->pCurWith->isCIH())
    {
        pTarget = pLocalContext->pCurWith->c_cih();
    }
    else
    {
        pTarget = pLocalContext->pCurrentContext;
    }

    int nFrame = pTarget->GetSpriteInstBase()->GetCharacterConst()->sprite.movie.labelToFrame(&pLabelName) + 1;

    if (nFrame - 1 >= 0)
    {
        pTarget->jumpToFrame(nFrame - 1);
        pTarget->SetIsPlaying(false);
    }
}

void AptActionInterpreter::_FunctionAptActionWith(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_With *pData = (AptAction_With *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_With);

    AptValue *pWithValue = pInterpreter->stackAt(0);
    if (pWithValue->isUndefined())
    {
        pLocalContext->pCurWith     = NULL;
        pLocalContext->pInstruction = pData->pEnd;
    }
    else
    {
        AptValue *pWith;
        valueToObject(pLocalContext->pCurrentContext, NULL, pWithValue, &pWith);
        APT_ASSERT(pWith);
        APT_ASSERT(!pLocalContext->pCurWith);
        {
            pLocalContext->pRemoveWithAt = pData->pEnd;
            pLocalContext->pCurWith      = pWith;
            APT_INC(pLocalContext->pCurWith);
        }
    }
    pInterpreter->stackPop();
}

void AptActionInterpreter::_FunctionAptActionPush(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_Push *pData = (AptAction_Push *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_Push);

    for (int i = 0; i < pData->items.nItems; i++)
    {
        pInterpreter->stackPushIndirect(pData->items.apItems[i]);
    }
}

void AptActionInterpreter::_FunctionAptActionGetUrl2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pA = pInterpreter->stackAt(0);
    AptValue *pB = pInterpreter->stackAt(1);
    AptNativeString sAString;
    AptNativeString sBString;

    pB->toString(sBString);

    if (pInterpreter->isFSCommand(sBString.ConstRawPtr()))
    {
        pA->toString(sAString);
        pInterpreter->doFSCommand(sBString.ConstRawPtr(), sAString.ConstRawPtr());
    }
    else
    {
        int nLength          = sBString.Size();
        AptNativeString sBuf = sBString;
        AptValue *pContext   = NULL;

        // if it's an unload (empty) or the extension is .swf then load as an animation
        // TODO: Not sure how this is supposed to interact with loading Textures
        if (nLength == 0 || sBuf.EndWithIgnoreCase(".swf"))
        {
            pA->toString(sAString);

            // try to convert to a full path if possible
            AptValue *pVal = pInterpreter->getVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, &sAString);
            if (pVal->isCIH())
            {
                getName(pVal->c_cih(), sAString);
            }

            // these are poped before because linker->load calls clearCIH that cleans the gpValueToRelease.
            pInterpreter->stackPop(2);
            GetTargetSim()->GetLinker()->Load(sBuf, sAString);
            return;
        }
        else
        {
            // is the 2nd parameter a string of a class or an actual class?
            if (pA->isString())
            {
                pContext = pInterpreter->getVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pA->c_string()->GetInternalString(), true);
            }
            else
            {
                pContext = pA;
            }

            pInterpreter->loadVariables(pContext, pLocalContext->pCurWith, &sBString);
        }
    }

    pInterpreter->stackPop(2);
}

void AptActionInterpreter::_FunctionAptActionDefineFunction(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_DefineFunction *pData = (AptAction_DefineFunction *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_DefineFunction);
    pLocalContext->pInstruction += pData->nCodeSize;
    pData->constantPool = pInterpreter->constantPool;

    // AptValue *pCheckFunc = gpUndefinedValue;
    AptScriptFunction1 *pFuncDef;
    AptCIH *pCIH = pLocalContext->pCurrentContext;

    pFuncDef = new AptScriptFunction1(pInterpreter->mpCurrentFunction,
                                      pData,
                                      // pParentAnimation,
                                      pCIH);

    if (pData->szName[0] == '\0')
    {
        pInterpreter->stackPush(pFuncDef);
    }
    else
    {
        AptNativeString strName(pData->szName);
        pInterpreter->setVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, &strName, pFuncDef, true);
    }
}

void AptActionInterpreter::_FunctionAptActionDefineFunction2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_DefineFunction2 *pData = (AptAction_DefineFunction2 *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_DefineFunction2);
    pLocalContext->pInstruction += pData->nCodeSize;
    pData->constantPool = pInterpreter->constantPool;

    AptCIH *pCIH = pLocalContext->pCurrentContext;
    // AptCharacterInst  * pCharInst        = pCIH->GetCharacterInst();
    // AptCharacter *      pParentAnimation;

    // if( pCharInst )
    //{
    //     pParentAnimation = pCharInst->GetCharacter(AptGetLib()->mnCurrUpdateTick)->pParentAnim;
    // }
    // else
    {
        //    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Error! Defining Function from Unloaded CIH Context!!!");
        // AptCIH * pTemp   = _AptGetAnimationAtLevel(0);
        // pParentAnimation = pTemp->GetCharacterInst()->GetCharacter(AptGetLib()->mnCurrUpdateTick);
    }

    AptScriptFunction2 *pFuncDef = new AptScriptFunction2(pInterpreter->mpCurrentFunction,
                                                          pData,
                                                          // pParentAnimation,
                                                          pCIH);

    if (pData->szName[0] == '\0')
    {
        pInterpreter->stackPush(pFuncDef);
    }
    else
    {
        AptNativeString strName(pData->szName);
        pInterpreter->setVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, &strName, pFuncDef, true);
    }
}

void AptActionInterpreter::_FunctionAptActionBranchIfTrue(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_BranchAddress *pData = (AptAction_BranchAddress *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_BranchAddress);

    AptValue *pCondition = pInterpreter->stackAt(0);

    if (pCondition->toBool() == true)
    {
        pLocalContext->pInstruction += pData->nTargetDelta;
    }

    pInterpreter->stackPop();

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionCallFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pFrame = pInterpreter->stackAt(0);

    int nFrame = -1;
    if (pFrame->isString())
    {
        AptValue *pLabelContext;
        AptNativeString sName;

        getContext(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pFrame->c_string()->GetInternalString(), &pLabelContext, sName);
        nFrame = pLabelContext->c_cih()->GetCharacterInst()->GetCharacterConst()->sprite.movie.labelToFrame(&sName);
    }
    else if (pFrame->isInteger())
    {
        nFrame = pFrame->toInteger() - 1; // Frame number is 0-based but the parameter is 1-based
    }
    else
    {
        APT_ASSERT(NOT_REACHED);
    }
    pInterpreter->stackPop();

    // run frames actions
    if (nFrame != -1)
    {
        pLocalContext->pCurrentContext->GetCharacterInst()->GetCharacterConst()->sprite.movie.runFrameActions(pLocalContext->pCurrentContext, nFrame);
    }
}

void AptActionInterpreter::_FunctionAptActionGotoFrame2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_GotoFrame2 *pData = (AptAction_GotoFrame2 *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_GotoFrame2);
    AptValue *pFrame = pInterpreter->stackAt(0);
    AptCIH *pCIH     = NULL;

    // Added support to use the current With (target) if it is out there.
    if (pLocalContext->pCurWith && pLocalContext->pCurWith->isCIH())
    {
        pCIH = pLocalContext->pCurWith->c_cih();
    }
    else if (pLocalContext->pCurrentContext->isCIH())
    {
        pCIH = pLocalContext->pCurrentContext->c_cih();
    }

    int nFrame = -1;
    if (pFrame->isString())
    {
        AptValue *pLabelContext;
        AptNativeString sName;

        getContext(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pFrame->c_string()->GetInternalString(), &pLabelContext, sName);
        if (pLabelContext->isCIH() && pLabelContext->c_cih()->IsSpriteInstBase())
        {
            nFrame = pLabelContext->c_cih()->GetSpriteInstBase()->GetCharacterConst()->sprite.movie.labelToFrame(&sName);
        }
    }
    else if (pFrame->isInteger())
    {
        nFrame = pFrame->toInteger() - 1; // Frame number is 0 based but the parameter is 1 based
    }

    // Ignore action if invalid frame
    if (nFrame != -1 && pCIH)
    {
        pCIH->jumpToFrame(nFrame);
        pCIH->SetIsPlaying(pData->bPlay != 0);
    }
    pInterpreter->stackPop();
}

void AptActionInterpreter::_FunctionAptActionBranchAlways(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_BranchAddress *pData = (AptAction_BranchAddress *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_BranchAddress);

    pLocalContext->pInstruction += pData->nTargetDelta;

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionPushThis(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptString *pString = AptString::Create();
    pString->str       = *StringPool::GetString(SC_this);
    pInterpreter->stackPush(pString);
}

void AptActionInterpreter::_FunctionAptActionPushGlobal(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptString *pString = AptString::Create();
    pString->str       = *StringPool::GetString(SC__global);
    pInterpreter->stackPush(pString);
}

void AptActionInterpreter::_FunctionAptActionPush0(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->stackPush(AptInteger::Create(0));
}

void AptActionInterpreter::_FunctionAptActionPush1(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->stackPush(AptInteger::Create(1));
}

void AptActionInterpreter::_FunctionAptActionPushTrue(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->stackPush(AptBoolean::Create(true));
}

void AptActionInterpreter::_FunctionAptActionPushFalse(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->stackPush(AptBoolean::Create(false));
}

void AptActionInterpreter::_FunctionAptActionPushNULL(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->stackPush(gpUndefinedValue);
}

void AptActionInterpreter::_FunctionAptActionPushUndefined(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->stackPush(gpUndefinedValue);
}

void AptActionInterpreter::_FunctionAptActionTraceStart(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    const uint32_t *pI  = (const uint32_t *)pLocalContext->pInstruction;
    uint32_t jumpOffset = *pI;

    AptValue *pParams = pInterpreter->stackAt(0);
    pInterpreter->stackPop(1);
    if (pParams == AptGetLib()->mpGlobalGlobalObject)
    {
        // that means that the call was actually made _global.AptTrace(), in that case we first pop of _global
        // and then look for actual number of params for AptTrace(...)
        pParams = pInterpreter->stackAt(0);
        pInterpreter->stackPop(1);
    }
    if (pParams->isInteger())
    {
        int nParams = pParams->c_integer()->GetInt();
        pInterpreter->stackPop(nParams);
    }

    if (pInterpreter->mbSkipTraceBytecodes)
    {
        pLocalContext->pInstruction += jumpOffset; // add size of jump offset which goes one bytecode beyond trace
    }
    else
    {
        pLocalContext->pInstruction += 4; // add just size of short, and move to next bytecode
    }
}

void AptActionInterpreter::_FunctionAptActionCallFuncAndPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _FunctionAptActionCallFunction(pInterpreter, pLocalContext);
    pInterpreter->stackPop();

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionCallFuncSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _FunctionAptActionCallFunction(pInterpreter, pLocalContext);
    _FunctionAptActionSetVariable(pInterpreter, pLocalContext);

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionCallMethodPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _FunctionAptActionCallMethod(pInterpreter, pLocalContext);
    pInterpreter->stackPop();

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionCallMethodSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _FunctionAptActionCallMethod(pInterpreter, pLocalContext);
    _FunctionAptActionSetVariable(pInterpreter, pLocalContext);

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionPushThisVariable(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    AptValue *pValue = pInterpreter->getVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, StringPool::GetString(SC_this), true);
    pInterpreter->stackPush(pValue);

    /*
        AptValue *  pCurrentContext = pLocalContext->pCurrentContext;

        if  (pInterpreter->thisStack.top() == pCurrentContext)
        {
            pInterpreter->stackPush(pCurrentContext);
            return;
        }

        AptNativeHash * pNativeHash = pCurrentContext->GetNativeHashVirtual();
        while   (pNativeHash != NULL)
        {
            AptValue *  pProto = pNativeHash->Get__Proto__();
            if (pProto == NULL)
            {
                break;
            }
            if (pProto == pCurrentContext)
            {
                pInterpreter->stackPush(pCurrentContext);
                return;
            }
            pNativeHash = pProto->GetNativeHashVirtual();
        }

        pInterpreter->stackPush(pInterpreter->thisStack.top());
    */
}

void AptActionInterpreter::_FunctionAptActionPushGlobalVariable(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    pInterpreter->stackPush(AptGetLib()->mpGlobalGlobalObject);
}

void AptActionInterpreter::_FunctionAptActionPushZeroSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    AptInteger *pInteger = AptInteger::Create(0);
    pInterpreter->stackPush(pInteger);

    _FunctionAptActionSetVariable(pInterpreter, pLocalContext);
}

void AptActionInterpreter::_FunctionAptActionPushString(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    // AptString * pString = new AptString;
    // pString->str = (char *)pLocalContext->pInstruction;     //  The string is '\0' terminated
    // pLocalContext->pInstruction += pString->str.GetLength() + 1;    //  Skip the size with the trailing '\0'
    // pInterpreter->stackPush(pString);

    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_PushString *pData = (AptAction_PushString *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_PushString);

    // if (pData->szStringToBePushed[0] != 0)
    //{
    AptString *pString = AptString::Create();
    pString->str       = pData->szStringToBePushed;
    pInterpreter->stackPush(pString);
    // }
}

void AptActionInterpreter::_FunctionAptActionPushStringDictByte(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    unsigned char uDictionary = *pLocalContext->pInstruction++;
    AptValue *pValue          = pInterpreter->constantPool.apItems[uDictionary];
    pInterpreter->stackPush(pValue);
}

void AptActionInterpreter::_FunctionAptActionPushStringDictWord(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    const uint8_t *pI = pLocalContext->pInstruction;

    union
    {
        uint16_t uDictionary;
        struct
        {
            char c0;
            char c1;
        } S;
    } Var;

    Var.S.c0                    = *pI++;
    Var.S.c1                    = *pI++;
    pLocalContext->pInstruction = pI;
    AptValue *pValue            = pInterpreter->constantPool.apItems[Var.uDictionary];
    pInterpreter->stackPush(pValue);
}

void AptActionInterpreter::_FunctionAptActionPushStringGetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    // AptGetLib()->mstrTempString = (char *)pLocalContext->pInstruction;     //  The string is '\0' terminated
    // pLocalContext->pInstruction += AptGetLib()->mstrTempString.GetLength() + 1;    //  Skip the size with the trailing '\0'

    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_PushString *pData = (AptAction_PushString *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_PushString);

    // AptString * pString = AptString::Create();
    AptGetLib()->mstrTempString = pData->szStringToBePushed;

    AptValue *pValue = pInterpreter->getVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, &AptGetLib()->mstrTempString, true);
    pInterpreter->stackPush(pValue);
}

void AptActionInterpreter::_FunctionAptActionPushStringGetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    // AptGetLib()->mstrTempString = (char *)pLocalContext->pInstruction;     //  The string is '\0' terminated
    // pLocalContext->pInstruction += AptGetLib()->mstrTempString.GetLength() + 1;    //  Skip the size with the trailing '\0'

    // AptString * pString = new AptString;
    // pString->str = AptGetLib()->mstrTempString;
    // pInterpreter->stackPush(pString);

    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_PushString *pData = (AptAction_PushString *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_PushString);

    AptString *pString = AptString::Create();
    pString->str       = pData->szStringToBePushed;
    pInterpreter->stackPush(pString);

    _FunctionAptActionGetMember(pInterpreter, pLocalContext);

    // pString->str.Clear();   //  We delete here the content of the string
    //   So there is now only one reference of the string in memory (in AptGetLib()->mstrTempString)
    //   So the allocated buffer stays always in memory
}

void AptActionInterpreter::_FunctionAptActionPushStringSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    // AptGetLib()->mstrTempString = (char *)pLocalContext->pInstruction;     //  The string is '\0' terminated
    // pLocalContext->pInstruction += AptGetLib()->mstrTempString.GetLength() + 1;    //  Skip the size with the trailing '\0'

    // AptString * pString = new AptString;
    // pString->str = AptGetLib()->mstrTempString;
    // pInterpreter->stackPush(pString);

    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_PushString *pData = (AptAction_PushString *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_PushString);

    AptString *pString = AptString::Create();
    pString->str       = pData->szStringToBePushed;
    pInterpreter->stackPush(pString);

    _FunctionAptActionSetVariable(pInterpreter, pLocalContext);

    // pString->str.Clear();   //  We delete here the content of the string
    //   So there is now only one reference of the string in memory (in AptGetLib()->mstrTempString)
    //   So the allocated buffer stays always in memory
}

void AptActionInterpreter::_FunctionAptActionPushStringSetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    // AptGetLib()->mstrTempString = (char *)pLocalContext->pInstruction;     //  The string is '\0' terminated
    // pLocalContext->pInstruction += AptGetLib()->mstrTempString.GetLength() + 1;    //  Skip the size with the trailing '\0'

    // AptString * pString = new AptString;
    // pString->str = AptGetLib()->mstrTempString;
    // pInterpreter->stackPush(pString);

    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_PushString *pData = (AptAction_PushString *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_PushString);

    AptString *pString = AptString::Create();
    pString->str       = pData->szStringToBePushed;
    pInterpreter->stackPush(pString);

    _FunctionAptActionSetMember(pInterpreter, pLocalContext);

    // pString->str.Clear();   //  We delete here the content of the string
    //   So there is now only one reference of the string in memory (in AptGetLib()->mstrTempString)
    //   So the allocated buffer stays always in memory
}

void AptActionInterpreter::_FunctionAptActionStringDictByteGetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    unsigned char uDictionary = *pLocalContext->pInstruction++;
    AptValue *pString         = pInterpreter->constantPool.apItems[uDictionary];

    AptValue *pValue = pInterpreter->getVariable(pLocalContext->pCurrentContext, pLocalContext->pCurWith, pString->c_string()->GetInternalString(), true);
    pInterpreter->stackPush(pValue);
}

void AptActionInterpreter::_FunctionAptActionStringDictByteGetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    unsigned char uDictionary = *pLocalContext->pInstruction++;
    AptValue *pValue          = pInterpreter->constantPool.apItems[uDictionary];
    pInterpreter->stackPush(pValue);
    _FunctionAptActionGetMember(pInterpreter, pLocalContext);
}

void AptActionInterpreter::_FunctionAptActionDictCallFuncPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    unsigned char uDictionary = *pLocalContext->pInstruction;
    AptValue *pValue          = pInterpreter->constantPool.apItems[uDictionary];

    pInterpreter->stackPush(pValue);
    _FunctionAptActionCallFunction(pInterpreter, pLocalContext);
    pLocalContext->pInstruction++;
    pInterpreter->stackPop();

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionDictCallFuncSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    unsigned char uDictionary = *pLocalContext->pInstruction;
    AptValue *pValue          = pInterpreter->constantPool.apItems[uDictionary];
    pInterpreter->stackPush(pValue);
    _FunctionAptActionCallFunction(pInterpreter, pLocalContext);
    pLocalContext->pInstruction++;
    _FunctionAptActionSetVariable(pInterpreter, pLocalContext);

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionDictCallMethodPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    unsigned char uDictionary = *pLocalContext->pInstruction;
    AptValue *pValue          = pInterpreter->constantPool.apItems[uDictionary];
    pInterpreter->stackPush(pValue);
    _FunctionAptActionCallMethod(pInterpreter, pLocalContext);
    pLocalContext->pInstruction++;
    pInterpreter->stackPop();

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionDictCallMethodSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    unsigned char uDictionary = *pLocalContext->pInstruction;
    AptValue *pValue          = pInterpreter->constantPool.apItems[uDictionary];
    pInterpreter->stackPush(pValue);
    _FunctionAptActionCallMethod(pInterpreter, pLocalContext);
    pLocalContext->pInstruction++;
    _FunctionAptActionSetVariable(pInterpreter, pLocalContext);

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionPushFloat(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    const uint8_t *pI = pLocalContext->pInstruction;

    union
    {
        float fValue;
        struct
        {
            char c0;
            char c1;
            char c2;
            char c3;
        } S;
    } Var;

    Var.S.c0                    = *pI++;
    Var.S.c1                    = *pI++;
    Var.S.c2                    = *pI++;
    Var.S.c3                    = *pI++;
    pLocalContext->pInstruction = pI;

    pInterpreter->stackPush(AptFloat::Create(Var.fValue));
}

void AptActionInterpreter::_FunctionAptActionPushByte(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    signed char c = *pLocalContext->pInstruction++;
    pInterpreter->stackPush(AptInteger::Create(c));
}

void AptActionInterpreter::_FunctionAptActionPushWord(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    const uint8_t *pI = pLocalContext->pInstruction;

    union
    {
        int16_t nValue;
        struct
        {
            unsigned char c0;
            unsigned char c1;
        } S;
    } Var;

    Var.S.c0                    = *pI++;
    Var.S.c1                    = *pI++;
    pLocalContext->pInstruction = pI;

    pInterpreter->stackPush(AptInteger::Create(Var.nValue));
}

void AptActionInterpreter::_FunctionAptActionPushDWord(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    const uint8_t *pI = pLocalContext->pInstruction;

    volatile union
    {
        int32_t nValue;
        struct
        {
            unsigned char c0;
            unsigned char c1;
            unsigned char c2;
            unsigned char c3;
        } S;
    } Var;

    // Warning I have had nightmares at previous jobs doing this with unions on GCC's optimizer.
    // I made the union volatile to prevent this from ruining other peoples weekends/holidays.
    // For those who want to the know what the problem I saw was: GCC optimizer does dependency checking
    // based on the pointer value, so Var.S.c0 modifications effect Var.S.nValue but Var.S.c1, c2,c3
    // are all at different memory locations, and thus can be after modified after any assignments to
    // Var.nValue... makeing this volatile should not pose any major performance problem but will prevent
    // any of those *crafty* optimizers from shifting the accesses around.
    Var.S.c0                    = *pI++;
    Var.S.c1                    = *pI++;
    Var.S.c2                    = *pI++;
    Var.S.c3                    = *pI++;
    pLocalContext->pInstruction = pI;

    pInterpreter->stackPush(AptInteger::Create(Var.nValue));
}

void AptActionInterpreter::_FunctionAptActionPushRegister(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    unsigned char cRegNum = *pLocalContext->pInstruction++;
    // APT_ASSERT(cRegNum >= 0);     // some how gives warning on ps2.
    AptValue *pPushValue = AptScriptFunctionBase::GetRegisterValue((int)cRegNum);
    pInterpreter->stack.Push(pPushValue);
}

void AptActionInterpreter::_FunctionAptActionBranchIfFalse(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    _TONEXTALIGNED(pLocalContext->pInstruction);
    AptAction_BranchAddress *pData = (AptAction_BranchAddress *)pLocalContext->pInstruction;
    pLocalContext->pInstruction += sizeof(AptAction_BranchAddress);

    AptValue *pCondition = pInterpreter->stackAt(0);

    if (pCondition->toBool() == false)
    {
        pLocalContext->pInstruction += pData->nTargetDelta;
    }

    pInterpreter->stackPop();

    if ((AptGetLib()->mpValuesToRelease->GetNumValues() != 0) && (pInterpreter->stack.GetSize() == 0))
    {
        //  There are some values to collect, we do it
        //  The same test is done internally but by doing that we avoid to call this function
        //  if not needed
        AptGetLib()->mpValuesToRelease->ReleaseValues();
    }
}

void AptActionInterpreter::_FunctionAptActionExtends(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    APT_ASSERT(pInterpreter->stack.GetSize() >= 2);
    AptValue *pSuperClass = pInterpreter->stackAt(0); // Step 1.
    AptValue *pSubClass   = pInterpreter->stackAt(1); // Step 2. This is the Class that extends the Super it should exist already.

    // Nothing to do if these are not Objects.
    if (pSuperClass->ContainsNativeHashVirtual() && pSubClass->isScriptFunction())
    {
        AptNativeHash *pSuperHash = pSuperClass->GetNativeHashVirtual();
        AptNativeHash *pSubHash   = pSubClass->GetNativeHashVirtual();
        AptValue *pSuperPrototype = pSuperHash->GetPrototype();
        AptValue *pSubPrototype   = pSubHash->GetPrototype();

        if (!pSuperPrototype)
        {
            pSuperPrototype = new AptPrototype();
            pSuperHash->SetPrototype(pSuperPrototype);
        }
        if (!pSubPrototype) // Step 4. Set Sub.protoype to a new Prototype if it does not have one already.
        {
            pSubPrototype = new AptPrototype();
            pSubHash->SetPrototype(pSubPrototype);
        }

        pSuperPrototype->SetClassName(pSuperClass->GetClassName());
        pSubPrototype->SetClassName(pSubClass->GetClassName());

#if defined(APT_USE_DEBUG_NAMES)
        // When debugging we often want to know the name of the class we're looking at.
        // This logic grabs the debug name we set during setMember and puts it in the
        // hashtables for both the subclass and the superclass.  This will not
        // catch classes that don't extend anything.
        const AptNativeString *nameKeyString = StringPool::GetString(SC__debugName);
        if (pSuperClass->isScriptFunction())
        {
            AptNativeHash *superProtoHash = pSuperPrototype->GetNativeHashVirtual();
            AptValue *currentSuperName    = superProtoHash->Lookup(nameKeyString);
            if (NULL == currentSuperName)
            {
                AptValue *superConstructorName = pSuperHash->Lookup(nameKeyString);
                if (NULL != superConstructorName)
                {
                    superProtoHash->Set(nameKeyString, superConstructorName);
                }
            }
        }
        if (pSubClass->isScriptFunction())
        {
            AptNativeHash *subProtoHash = pSubPrototype->GetNativeHashVirtual();
            AptValue *currentSubName    = subProtoHash->Lookup(nameKeyString);
            if (NULL == currentSubName)
            {
                AptValue *subConstructorName = pSubHash->Lookup(nameKeyString);
                if (NULL != subConstructorName)
                {
                    subProtoHash->Set(nameKeyString, subConstructorName);
                }
            }
        }
#endif

        APT_ASSERT(pSubPrototype->isPrototype() && "Object Extending has invalid prototype object!");
        pSubPrototype->c_prototype()->SetSuperConstructor(pSuperClass);

        pSuperClass->SetHasClass(1);
        pSubClass->SetHasClass(1);

        pSubPrototype->GetNativeHashVirtual()->Set__Proto__(pSuperPrototype); // Step 5. Set Sub.prototype.__proto__ to the super class prototype
    }
    else
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "--AptWarning-- Actionscript is attempting to use invalid objects in Extends Opcode.");
    }

    pInterpreter->stackPop(2);
}

bool AptActionInterpreter::isObjectOfType(AptValue *pObject, AptValue *pInterface)
{
    bool bIsOfType = false;

    // If we have an actionscript object, check it's prototype.
    if (pObject->ContainsNativeHashVirtual() && pInterface->ContainsNativeHashVirtual())
    {
        AptValue *pFuncPrototype = pInterface->GetNativeHashVirtual()->GetPrototype();

        APT_ASSERT(pFuncPrototype->isPrototype());

        if (pObject->isCIH())
        {
            // Look up the prototype Chain.
            AptValue *pProto = pObject->GetNativeHashVirtual()->Get__Proto__();
            while (pProto)
            {
                if (pProto == pFuncPrototype)
                {
                    bIsOfType = true;
                }

                pProto = pProto->GetNativeHashVirtual()->Get__Proto__();
            }
        }
        else
        {
            if (pObject->isObject())
            {
                if (((AptObject *)pObject)->DoesImplementObject(pFuncPrototype))
                {
                    bIsOfType = true;
                }
            }
            else
            {
                AptValue *pProto = pObject->GetNativeHashVirtual()->Get__Proto__();

                while (pProto)
                {
                    if (pProto == pFuncPrototype)
                    {
                        bIsOfType = true;
                        return bIsOfType;
                    }

                    if (pProto->ContainsNativeHashVirtual())
                    {
                        pProto = pProto->GetNativeHashVirtual()->Get__Proto__();
                    }
                    else
                    {
                        break;
                    }
                }
                bIsOfType = false;
            }
        }
    }
    // Else if it isn't an actionscript object or script function it is probably a native object,
    // just compare data types.
    else if (!pObject->isScriptFunction() && !pObject->isObject() &&
             pObject->getVtblIndex() == pInterface->getVtblIndex())
    {
        bIsOfType = true;
    }

    return bIsOfType;
}

void AptActionInterpreter::_FunctionAptActionInstanceOf(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pObject;
    AptValue *pInterface;
    bool retVal;
    APT_ASSERT(pInterpreter->stack.GetSize() >= 2);

    if (pInterpreter->stack.GetSize() < 2)
    {
        APT_ASSERT(false && "[APT] Actionscript InstanceOf Op did not find enough parameters. Check Script code.");
        pInterpreter->stackPopAndPush(pInterpreter->stack.GetSize(), gpUndefinedValue);
        return;
    }

    // Fixed Issue where InstanceOf / CastOp would fail for MovieClips / some Inheritence Chains
    pObject    = pInterpreter->stackAt(1);
    pInterface = pInterpreter->stackAt(0);

    retVal = isObjectOfType(pObject, pInterface);

    pInterpreter->stackPopAndPush(2, AptBoolean::Create(retVal));
}

void AptActionInterpreter::_FunctionAptActionCastOp(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    AptValue *pObject;
    AptValue *pInterface;

    if (pInterpreter->stack.GetSize() < 2)
    {
        APT_ASSERT(false && "[APT] Actionscript Cast Op did not find enough parameters. Check Script code.");
        pInterpreter->stackPopAndPush(pInterpreter->stack.GetSize(), gpUndefinedValue);
        return;
    }

    pObject    = pInterpreter->stackAt(0);
    pInterface = pInterpreter->stackAt(1);

    // Fixed Issue where InstanceOf / CastOp would fail for MovieClips / some Inheritence Chains
    if (isObjectOfType(pObject, pInterface))
    {
        pInterpreter->stackPopAndPush(2, pObject);
    }
    else
    {
        pInterpreter->stackPop(2);
        pInterpreter->stackPushNoInc(gpUndefinedValue);
    }
}

void AptActionInterpreter::_FunctionAptActionImplementsOp(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
#if APT_TRACK_IMPLEMENTED_INTERFACES
    AptValue *pObject = NULL;
#endif
    int nInterfaces;
    AptValue *pTemp;
    AptNativeString sTemp;

    APT_ASSERT(pInterpreter->stack.GetSize() > 2);

#if APT_TRACK_IMPLEMENTED_INTERFACES
    {
        pObject = pInterpreter->stackAt(0);
#endif
        pTemp       = pInterpreter->stackAt(1);
        nInterfaces = pTemp->toInteger();
#if APT_TRACK_IMPLEMENTED_INTERFACES
    }

    if (!pObject->isFunction())
    {
        APT_FAIL("Implements Opcode attempting to mess with invalid object.");
        pInterpreter->stackPop(2 + nInterfaces);
        return;
    }
#endif

    APT_ASSERT(nInterfaces > 0);

#if APT_TRACK_IMPLEMENTED_INTERFACES
    AptArray *pA         = new AptArray();
    int nInterfacesFound = 0;
    for (int i = 0; i < nInterfaces; i++)
    {
        pTemp = pInterpreter->stackAt(i + 2);
        // if undefined value if pushed on stack then basically SWF compiler is not able to find interface definition at time of publishing
        // the class. But in Flash it just continues to execute fine.
#if !defined(DO_COVERAGE)
        APT_ASSERTM(pTemp != gpUndefinedValue, "One of the interface definitions was not available at time of publishing class definition");
        APT_ASSERT(pTemp->ContainsNativeHashVirtual());
#endif

        if (pTemp != gpUndefinedValue && pTemp->ContainsNativeHashVirtual())
        {
            AptNativeHash *pNativeHash = pTemp->GetNativeHashVirtual();
            AptValue *pFuncPrototype   = pNativeHash->GetPrototype();

            if (!pFuncPrototype)
            {
                pFuncPrototype = new AptPrototype();
                pNativeHash->SetPrototype(pFuncPrototype);
            }

            pA->set(nInterfacesFound, pFuncPrototype);
            nInterfacesFound++;
        }
    }
    if (pObject->GetNativeHashVirtual() != NULL)
    {
        AptObject *pObjectTemp = (AptObject *)pObject;
        pObjectTemp->SetImplementedObjects(pA, nInterfacesFound); // EATech#105757, 105235
    }
    AptObject *pObjectTemp = (AptObject *)pObject;
    pObjectTemp->SetImplementedObjects(pA, nInterfacesFound); // EATech#105757, 105235
#endif

    // The fact that this is based off of AptObject is Asserted earlier.
    pInterpreter->stackPop(2 + nInterfaces);
}

void AptActionInterpreter::_FunctionAptActionTry(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
    uint32_t nPreStackSize = pInterpreter->stack.GetSize();
    _TONEXTALIGNED(pLocalContext->pInstruction);
    const AptAction_TryCatchFinallyBlock *pTry = (AptAction_TryCatchFinallyBlock *)pLocalContext->pInstruction;

    pLocalContext->pInstruction += sizeof(AptAction_TryCatchFinallyBlock);

    pLocalContext->pInstruction += pTry->uTryCodeSize;
    pLocalContext->pInstruction += pTry->uCatchCodeSize;
    pLocalContext->pInstruction += pTry->uFinallyCodeSize;

    {

        pInterpreter->runStream(pTry->getTryBlockBase(), pLocalContext->pCurrentContext, pTry->uTryCodeSize, pLocalContext->pParentCharacter);
    }
    if (pInterpreter->hasThrownValue() && pTry->hasCatchBlock())
    {
        AptValue *pThrown = pInterpreter->getThrownValue();

        // Note that the objects do not need to be cleaned up. I have demonstrated that objects declared in try / catch / finally
        // blocks are all accessable outside the blocks... including the thrown object itself... crazy, but hey its Macromedia?
        if (pTry->putCaughtObjectInRegister())
        {
            AptScriptFunctionBase::SetRegisterValue(pTry->uCaughtRegister, pThrown); // This internally handles Reference counts.
        }
        else
        {
            AptNativeString strParam(pTry->szCaughtVarName);

            if (pInterpreter->mpCurrentFunction)
            {
                pInterpreter->mpCurrentFunction->SetInLocalScope(&strParam, pThrown);
            }
            else
            {
                pInterpreter->setVariable(pLocalContext->pCurrentContext, 0, &strParam, pThrown);
            }
        }

        pInterpreter->clearThrownValue();

        pInterpreter->runStream(pTry->getCatchBlockBase(), pLocalContext->pCurrentContext, pTry->uCatchCodeSize, pLocalContext->pParentCharacter);
    }
    if (pTry->hasFinallyBlock())
    {
        // What if catch thew a new object, or the same object to another handler... Pass it on.
        AptValue *pThrown = pInterpreter->getThrownValue();
        if (pThrown)
        {
            APT_INC(pThrown);
            pInterpreter->clearThrownValue();
        }

        pInterpreter->runStream(pTry->getFinallyBlockBase(), pLocalContext->pCurrentContext, pTry->uFinallyCodeSize, pLocalContext->pParentCharacter);

        if (pThrown != NULL && pInterpreter->hasThrownValue() == false)
        {
            pInterpreter->throwValue(pThrown);
            APT_DEC(pThrown);
        }
    }

    uint32_t nPostStackSize = pInterpreter->stack.GetSize();
    if (nPostStackSize > nPreStackSize)
    {
        // We need clean up the stack if anything was left after the stack unwind.
        pInterpreter->stack.Pop(nPostStackSize - nPreStackSize);
    }

    APT_ASSERT((uint32_t)pInterpreter->stack.GetSize() == nPreStackSize);
}

void AptActionInterpreter::_FunctionAptActionThrow(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{

    APT_ASSERT(pInterpreter->stack.GetSize() >= 1);

    pInterpreter->throwValue(pInterpreter->stackAt(0));

    pInterpreter->stackPop();
}

AptValue *AptActionInterpreter::GetNextProto(AptValue *pObject)
{
    AptNativeHash *pNativeHash = pObject->GetNativeHashVirtual();
    if (pNativeHash)
    {
        AptValue *pProto = pNativeHash->Get__Proto__();
        return pProto;
    }
    return NULL;
}

bool AptActionInterpreter::HasMethodImplementation(AptValue *pObject, const AptNativeString *psFunction)
{
    AptValue *pProto  = NULL;
    AptValue *pMember = NULL;
    if (pObject->isPrototype())
    {
        pProto = pObject;
    }
    else if (pObject->isCIH() || pObject->isObject())
    {
        pProto = GetNextProto(pObject);
    }
    if (pProto)
    {
        AptNativeHash *pNativeHash = pProto->GetNativeHashVirtual();
        if (pNativeHash)
        {
            pMember = pNativeHash->Lookup(psFunction);
        }
    }
    return (pMember != NULL);
}

AptValue *AptActionInterpreter::FindSuperImplementor(AptValue *pObject, const AptNativeString *psFunction)
{
    AptValue *pProto = GetNextProto(pObject);
    while (pProto)
    {
        if (HasMethodImplementation(pProto, psFunction))
        {
            return pProto;
        }
        pProto = GetNextProto(pProto);
    }
    return NULL;
}

static int WriteAptCallStack(char *buffer, int size)
{
    int wrote = 0;
    if (NULL != gAptOptCallStack)
    {
        for (int i = 0, count = gAptOptCallStack->GetItemCount(); i < count && wrote < size; i++)
        {
            const AptStackItem &item = gAptOptCallStack->GetItem(i);
            const char *file         = item.file ? (const char *)item.file : "?";
            wrote += item.scope ? SNPRINTF(buffer + wrote, size - wrote, "%s.%s ('%s', %p)\n", item.scope, item.function, file, item.context) : SNPRINTF(buffer + wrote, size - wrote, "%s ('%s', %p)\n", item.function, file, item.context);
        }
    }
    return wrote;
}

/**
 * Constructs a callstack string and fires an assert using pfnAssertFail*().
 * @param statement the string buffer
 * @param message the (optional) message
 * @param file __FILE__
 * @param line __LINE__
 */
void AptAssert(const char *statement, const char *message, const char *file, int line)
{
    enum
    {
        MAX_ASSERT_BUFFER_SIZE = 16384
    };

    char buffer[MAX_ASSERT_BUFFER_SIZE];

    int wrote = 0, size = MAX_ASSERT_BUFFER_SIZE;

    wrote += SNPRINTF(buffer, size, "%s\n\n", statement ? statement : "");
    wrote += WriteAptCallStack(buffer + wrote, size - wrote);

    if (!message)
    {
        if (NULL != AptGetUserFuncs().pfnAssertFail)
        {
            AptGetUserFuncs().pfnAssertFail(buffer, file, line);
        }
    }
    else
    {
        if (NULL != AptGetUserFuncs().pfnAssertFailMsg)
        {
            AptGetUserFuncs().pfnAssertFailMsg(buffer, message, file, line);
        }
    }
}

/******************************************************************************/
/**
    AptActionInterpreter::_FunctionAptActionBreakpoint
    @brief  Process AptActionBreakpoint instruction and execute the original
            instruction which is replaced by breakpoint.

    @param  pInterpreter    instance of action interpreter class (this is a static function)
    @param  pLocalContext   current execution context
*/
/******************************************************************************/
void AptActionInterpreter::_FunctionAptActionBreakpoint(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext)
{
#ifdef APT_DEBUGGER_ENABLE

    APT_ASSERT(NULL != AptDebugger::GetInstance());

    unsigned char action = AptDebugger::GetInstance()->RaiseBreakpoint(
        const_cast<unsigned char *>(pLocalContext->pInstruction),
        pLocalContext->pCurrentContext);

    APT_ASSERT(action < AptActionBreakpoint);

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    APT_ASSERT(sGlobalTable[action].mCheckAlignment == action);
#endif

    sGlobalTable[action].mFunctionPointer(pInterpreter, pLocalContext);

#endif // APT_DEBUGGER_ENABLE
}
