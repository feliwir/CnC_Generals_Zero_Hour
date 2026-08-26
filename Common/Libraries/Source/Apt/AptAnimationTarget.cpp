/*** Include files ********************************************************************************/
#include "AptAnimationTarget.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "Display/AptRenderingContext.h"
#include "AptAnimationTargetButtonFunction.h"
#include "MainInline.h"

#include "AptBCRenderTree.h"
#include "AptCallStack.h"
#if defined(APT_DEBUGGER_ENABLE)
#include "AptDebugger/AptDebugger.h"
#endif


/*** Defines **************************************************************************************/
#define APT_SAVEINPUTS_RECORD_SIZE 8

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/
struct ListenerEventType
{
    int32_t nFlag;
    StringCode eName;
};

static ListenerEventType _aListenerEvents[] =
    {
        {AptEventActionFlag_KeyUp, SC_onKeyUp},
        {AptEventActionFlag_KeyDown, SC_onKeyDown},
        {AptEventActionFlag_MouseDown, SC_onMouseDown},
        {AptEventActionFlag_MouseUp, SC_onMouseUp},
        {AptEventActionFlag_MouseMove, SC_onMouseMove},
        {AptEventActionFlag_Wheel, SC_onMouseWheel}};

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/
#if defined(APT_ALTERNATE_INPUT)
#endif
// Private variables

AptAnalogStickInfo AptAnimationTarget::sgAStickLeft[AptInputController_NumControllers];
AptAnalogStickInfo AptAnimationTarget::sgAStickRight[AptInputController_NumControllers];
AptAnalogStickInfo AptAnimationTarget::sgATriggers[AptInputController_NumControllers]; // Analog triggers data

int32_t AptAnimationTarget::snXMousePos = 0;
int32_t AptAnimationTarget::snYMousePos = 0;

#if defined APT_USE_BUTTONS
int32_t AptAnimationTarget::snBILCount                        = 0;    // count of the used elements in buttonInstanceList
ButtonHitTestRecord *AptAnimationTarget::saButtonInstanceList = NULL; // content of the array will be clean and refilled every frame, these are now allocated at runtime in constructor
int32_t AptAnimationTarget::siButtonInstanceListSize          = 0;
#endif

int32_t AptAnimationTarget::siMaxNewMovieClips      = 0; // Although the vectors will grow to any size, the max count will still assert to keep our memory in track.
int32_t AptAnimationTarget::siNewMovieClipsGrowSize = 1024;
AptAnimationCihList AptAnimationTarget::sapNewInsts; // Vector to track newly constructed CIH objects.
int32_t AptAnimationTarget::snNewInsts = 0;
AptAnimationCihList AptAnimationTarget::sapDelayedReleaseList; // Vector to track CIH objects to be deleted on the next frame update.
int32_t AptAnimationTarget::snDelayedReleaseCount   = 0;
int32_t AptAnimationTarget::snDelayedReleaseCountHW = 0;

// Public variables

extern AptCallStack *gAptOptCallStack;

/*** Private Functions ****************************************************************************/

namespace
{

#if defined APT_USE_MOUSE

static bool IsPointInRect(AptRect &rect, const AptMatrix &mat, int32_t x, int32_t y)
{
    float l  = rect.fLeft;
    float r  = rect.fRight;
    float t  = rect.fTop;
    float b  = rect.fBottom;
    float x0 = mat.a * l + mat.b * t + mat.tx;
    float y0 = mat.c * l + mat.d * t + mat.ty;
    float x1 = mat.a * r + mat.b * b + mat.tx;
    float y1 = mat.c * r + mat.d * b + mat.ty;
    return (x > x0 && x < x1 && y > y0 && y < y1);
}

static bool IsPointInCIH(AptCIH *cih, int32_t x, int32_t y)
{
    AptRect rect;
    cih->GetGlobalBoundingRect(&rect);
    return (x > rect.fLeft && x < rect.fRight && y > rect.fTop && y < rect.fBottom);
}

static bool PointHits(AptCIH *cih, int32_t x, int32_t y)
{
    bool hits = IsPointInCIH(cih, x, y);
    if (hits)
    {
        AptCIH *mask = cih->GetMask();
        if (mask)
        {
            // hit-test against mask
            hits = IsPointInCIH(mask, x, y);
        }

        // hit-test any other masks in mask layer
        for (AptCIH *prev = cih->GetDisplayListPrevious(); hits && prev; prev = prev->GetDisplayListPrevious())
        {
            if (prev->GetCharacterInst()->GetRenderItem()->GetClipDepth() >= 0) // It's a clipper
            {
                hits = IsPointInCIH(prev, x, y);
            }
        }
    }
    return hits;
}
#endif // APT_USE_MOUSE

inline static const char *_GetMovieClipEventFunctionDescription(const AptActionQueueC::AptActionPool *cur)
{
    static const char DEFAULT_DESC[] = "<movieclipevent>";
#if defined(APT_DEBUG)
    static const char LOAD_DESC[]           = "<onLoad>";
    static const char ENTERFRAME_DESC[]     = "<onEnterFrame>";
    static const char UNLOAD_DESC[]         = "<onUnload>";
    static const char MOUSEMOVE_DESC[]      = "<onMouseMove>";
    static const char MOUSEDOWN_DESC[]      = "<onMouseDown>";
    static const char MOUSEUP_DESC[]        = "<onMouseUp>";
    static const char KEYDOWN_DESC[]        = "<onKeyDown>";
    static const char KEYUP_DESC[]          = "<onKeyUp>";
    static const char DATA_DESC[]           = "<onData>";
    static const char INITIALIZE_DESC[]     = "<onInitialize>";
    static const char PRESS_DESC[]          = "<onPress>";
    static const char RELEASE_DESC[]        = "<onRelease>";
    static const char RELEASEOUTSIDE_DESC[] = "<onReleaseOutside>";
    static const char ROLLOVER_DESC[]       = "<onRollOver>";
    static const char ROLLOUT_DESC[]        = "<onRollOut>";
    static const char DRAGOVER_DESC[]       = "<onDragOver>";
    static const char DRAGOUT_DESC[]        = "<onDragOut>";
    static const char KEYPRESS_DESC[]       = "<onButtonKeyPress>";
    static const char MOUSEWHEEL_DESC[]     = "<onMouseWheel>";

    switch (cur->eAptActionType)
    {
    case AptActionType_OnLoad:
        return LOAD_DESC;
    case AptActionType_EnterFrame:
        return ENTERFRAME_DESC;
    case AptActionType_Unload:
        return UNLOAD_DESC;
    case AptActionType_MouseMove:
        return MOUSEMOVE_DESC;
    case AptActionType_MouseDown:
        return MOUSEDOWN_DESC;
    case AptActionType_MouseUp:
        return MOUSEUP_DESC;
    case AptActionType_KeyDown:
        return KEYDOWN_DESC;
    case AptActionType_KeyUp:
        return KEYUP_DESC;
    case AptActionType_Data:
        return DATA_DESC;
    case AptActionType_Initialize:
        return INITIALIZE_DESC;
    case AptActionType_Press:
        return PRESS_DESC;
    case AptActionType_Release:
        return RELEASE_DESC;
    case AptActionType_ReleaseOutside:
        return RELEASEOUTSIDE_DESC;
    case AptActionType_RollOver:
        return ROLLOVER_DESC;
    case AptActionType_RollOut:
        return ROLLOUT_DESC;
    case AptActionType_DragOver:
        return DRAGOVER_DESC;
    case AptActionType_DragOut:
        return DRAGOUT_DESC;
    case AptActionType_KeyPress:
        return KEYPRESS_DESC;
    case AptActionType_Wheel:
        return MOUSEWHEEL_DESC;
    default:
        break;
    }
#endif // defined(APT_DEBUG)
    return DEFAULT_DESC;
}
} // namespace

/*** Private Methods ******************************************************************************/

void AptAnimationTarget::AddListenerToQueue(AptValue *pValue, int32_t nEventFlags, AptInput input)
{
    // jump out early if the sprite doesn't handle the event.
    APT_ASSERT(pValue->ContainsNativeHashVirtual());
    if (pValue->isCIH() && !pValue->c_cih()->HasEvent(nEventFlags))
    {
        return;
    }
    // Added extra check for mp__proto__
    AptNativeHash *pHash = pValue->GetNativeHashVirtual();
    if (!pValue->isCIH() && !pHash->HasEventHandler(nEventFlags) &&
        (pHash->Get__Proto__() && !pHash->Get__Proto__()->GetNativeHashVirtual()->HasEventHandler(nEventFlags))) // Check __proto__ for event flag as well
    {
        return;
    }

    for (int32_t i = 0; i < APT_ARRAYSIZE(_aListenerEvents); i++)
    {
        if (_aListenerEvents[i].nFlag & nEventFlags)
        {
            AptValue *pValue1 = pValue->findChild(StringPool::GetString(_aListenerEvents[i].eName), NULL);
            if (pValue1 && pValue1->getIsDefined() && pValue1->isScriptFunction())
            {
                // Changed to use AptScriptFunctionBase (Reduces duplication of code).
                // This also is much more correct from a scoping perspective because the function
                // prototype and such is copied as well (it wasn't before).
                AptScriptFunctionBase *pFuncBase = pValue1->c_scriptfunction();
                if (pFuncBase->mpCIH != pValue)
                {
                    // set mpParentAnim to the CIH that created the script function, so the correct hash table is used
                    //                to find the script function.
                    // TODO: find another way to fix this...
                    AptCIH *pTempCIH = pFuncBase->mpCIH;
                    // pFuncBase = pFuncBase->Duplicate( pFuncBase->mpFuncAnim, (AptCIH*) pValue);
                    pFuncBase = pFuncBase->Duplicate((AptCIH *)pValue);
                    pFuncBase->mpParentAnim->DecZombieCount(); // We also need to dec the Zombie counter!
                    APT_DEC(pFuncBase->mpParentAnim);
                    pFuncBase->mpParentAnim = (AptCIH *)pTempCIH; // DOING THIS IS REALLY REALLY BAD, WE SHOULD NEVER CHAGNE THE mpParentAnim POINTER SINCE IT IS USED TO MAINTAIN THE ZOMBIE REFERENCES!!!!
                    pFuncBase->mpParentAnim->IncZombieCount();    // Now we need to inc the new parents zombie counter
                    APT_INC(pFuncBase->mpParentAnim);

                    pFuncBase->setGCRoot(1);
                }

                AddFunctionBack((AptCIH *)pValue,
                                pFuncBase,
                                pFuncBase->GetNumArguments(),
                                    ACTION_TYPE_CALL_PARAM(_aClipEvents[i].nFlag)
                                        input);
            }
        }
    }
}

void AptAnimationTarget::ProcessListenerEvents(AptInputType eType, AptInputState eState, AptInput input, AptInputController eController)
{
    APT_ASSERT(eState == AptInputState_Pressed || eState == AptInputState_Released || eState == AptInputState_WheelUp || eState == AptInputState_WheelDown || eState == AptInputState_MouseMove);
    AptValue *pValue = NULL;
    int32_t nCount   = 0;

#if defined(APT_USE_MOUSE)
    if (eController == AptInputController_Mouse && ((eType == AptInputType_MouseButton0 && (eState == AptInputState_Pressed || eState == AptInputState_Released)) || eState == AptInputState_WheelDown || eState == AptInputState_WheelUp))
    {
        nCount = 0;
        for (int32_t i = 0; i < mMouseListenerSet.capacity(); i++)
        {
            if (nCount == mMouseListenerSet.mnElements)
            {
                break;
            }
            if (mMouseListenerSet.maElements[i] == NULL)
            {
                continue;
            }
            pValue = mMouseListenerSet.maElements[i];

            switch (eState)
            {
            case AptInputState_Pressed:
                APT_ASSERT(eType == AptInputType_MouseButton0)
                AddListenerToQueue(pValue, AptEventActionFlag_MouseDown, input);
                break;
            case AptInputState_Released:
                APT_ASSERT(eType == AptInputType_MouseButton0)
                AddListenerToQueue(pValue, AptEventActionFlag_MouseUp, input);
                break;
            case AptInputState_WheelUp: // F#563    8
            case AptInputState_WheelDown:
                if (pValue->isCIH() && pValue->c_cih()->IsDynamicTextInst()) // added support for TextMouseWheel   4/
                {
                    if (mpTopMostSprite == pValue)
                    {
                        int32_t nTmp = eType;
                        if (eState == AptInputState_WheelDown)
                        {
                            nTmp *= -1;
                        }
                        AptValue *nPos          = AptInteger::Create(nTmp);
                        AptNativeString sScroll = "scroll";
                        pValue->c_cih()->objectMemberSet(pValue, &sScroll, nPos);
                    }
                }
                else
                {
                    AddListenerToQueue(pValue, AptEventActionFlag_Wheel, input);
                }
                break;
            default:
                APT_ASSERT(NOT_REACHED);
                break;
            }
            nCount++;
        }
    }
    else
#endif
    {
        if (eState == AptInputState_Pressed || eState == AptInputState_Released)
        {
            // ok now queue up the clipevent actions that need keydown/up events
            for (int32_t i = 0; i < mListenerSet.capacity(); i++)
            {
                if (nCount == mListenerSet.mnElements)
                {
                    break;
                }
                if (mListenerSet.maElements[i] == NULL)
                {
                    continue;
                }
                pValue = mListenerSet.maElements[i];

                switch (eState)
                {
                case AptInputState_Pressed:
                    AddListenerToQueue(pValue, AptEventActionFlag_KeyDown, input);
                    break;
                case AptInputState_Released:
                    AddListenerToQueue(pValue, AptEventActionFlag_KeyUp, input);
                    break;
                default:
                    break;
                }
                nCount++;
            }
        }
    }
}

void AptAnimationTarget::ProcessInputSet(AptInputType eType, AptInputState eState, AptInput input, AptInputController eController, bool bCheckTop)
{

    AptCIH *pSprInst     = NULL;
    bool bQueuedKeyPress = false;
#if defined(APT_USE_MOUSE)
    AptValue *pCurrentTopSprite = gpUndefinedValue;
    bool bPressRelease          = false;
    bool bPress                 = false;
#endif
    int32_t nCount = 0;

    for (int32_t i = 0; i < mInputSet.capacity(); i++)
    {
        if (nCount == mInputSet.mnElements)
        {
            break;
        }
        if (mInputSet.maElements[i] == NULL)
        {
            continue;
        }
        pSprInst = mInputSet.maElements[i];

        if (IsInputMasked(pSprInst))
        {
            continue;
        }
        APT_ASSERT(mInputSet.maElements[i]->isCIH());

#if defined(APT_USE_MOUSE)
        if (bCheckTop)
        {
            // Here we get the top most sprite over the current mouse position, we only do this once, bCehckTop should be false everytime except the first time.
            if (pSprInst->IsVisible() && pSprInst->HasMouseEvent() && PointHits(pSprInst, snXMousePos, snYMousePos)) // added support for TextField MouseWheel  2/
            {
                if (pCurrentTopSprite->isUndefined() || (!pSprInst->IsParent(pCurrentTopSprite->c_cih()) && pSprInst->CheckIfHigher(pCurrentTopSprite->c_cih())))
                {
                    pCurrentTopSprite = pSprInst;
                }
            }
        }

        if (eController == AptInputController_Mouse)
        {
            // Now we handle the input event
            switch (eState)
            {
            case AptInputState_Pressed:
                APT_ASSERT(eType == AptInputType_MouseButton0);
                {
                    pSprInst->queueClipEvents(AptEventActionFlag_MouseDown, input);
                    bPress = true;
                }
                break;
            case AptInputState_Released:
                APT_ASSERT(eType == AptInputType_MouseButton0);
                {
                    pSprInst->queueClipEvents(AptEventActionFlag_MouseUp, input);
                    bPressRelease = true;
                }
                break;
            case AptInputState_WheelUp: // 07/21/04 these two can pass here since _ProcessMouseListenerEvents will take care of these events
            case AptInputState_WheelDown:
                break;
            case AptInputState_MouseMove:
                if (!pSprInst->IsDynamicTextInst()) // skip this if the pSprInst is a textfield   1/
                {
                    pSprInst->queueClipEvents(AptEventActionFlag_MouseMove, input);
                }
                break;
            default:
                APT_ASSERT(NOT_REACHED);
                break;
            }
        }
        else
#endif
        {
            // Now we handle the input event
            switch (eState)
            {
            case AptInputState_Pressed:
                if (INPUT_IS_ANALOG(&input))
                {
                    APT_ASSERT((eType == AptInputType_LeftAnalogStick) || (eType == AptInputType_RightAnalogStick));
                    pSprInst->queueClipEvents(AptEventActionFlag_KeyDown, input, false);
                }
                else if (INPUT_IS_GESTURE(&input))
                {
                    pSprInst->queueClipEvents(AptEventActionFlag_KeyDown, input, false);
                }
                else if (INPUT_IS_KEY(&input))
                {
                    pSprInst->queueClipEvents(AptEventActionFlag_KeyDown, input, false);
                    if (!bQueuedKeyPress)
                    {
                        bQueuedKeyPress = pSprInst->queueClipEvents(AptEventActionFlag_KeyPress, input);
                    }
                }
                break;
            case AptInputState_Released:
                if (INPUT_IS_KEY(&input))
                {
                    {
                        pSprInst->queueClipEvents(AptEventActionFlag_KeyUp, input, false);
                    }
                }
                break;
            default:
                APT_ASSERT(NOT_REACHED);
                break;
            }
        }
        nCount++;
    }

#if defined APT_USE_BUTTONS
    // button keypresses
    if (eState == AptInputState_Pressed && mButtonSet.mnElements != 0 && bQueuedKeyPress == false) // NO idea what this does, come back and comment later
    {
        nCount = 0;
        for (int32_t i = 0; i < mButtonSet.capacity(); i++)
        {
            if (nCount == mButtonSet.mnElements)
            {
                break;
            }
            if (mButtonSet.maElements[i] == NULL)
            {
                continue;
            }
            AptCIH *pInst = mButtonSet.maElements[i];

            if (IsInputMasked(pInst))
            {
                continue;
            }

            APT_ASSERT(pInst->IsButtonInst());

            const AptCharacter *pChar = pInst->GetCharacterInst()->GetCharacterConst();
            for (int32_t j = 0; j < pChar->button.nActionConditions; j++)
            {
                if (pChar->button.aActionConditions[j].nConditions & AptActionConditionFlag_KeyPress)
                {
                    int32_t nKey = (pChar->button.aActionConditions[j].nConditions & AptActionConditionFlag_KeyPress) >> 9;
                    if (eType == nKey)
                    {
                        AddActionBack(&pChar->button.aActionConditions[j].actions, pInst->GetDisplayListParent(),
                                          ACTION_TYPE_CALL_PARAM(AptActionType_ButtonActions)
                                              gNullInput);
                        return;
                    }
                }
            }
            nCount++;
        }
    }
#endif

#if defined(APT_USE_MOUSE)
    if (bCheckTop)
    {
        mpTopMostSprite = pCurrentTopSprite; // keep track of the highest sprite       F#563   7
    }
    ProcessMouseState(bPress, bPressRelease, input);
#if defined APT_USE_BUTTONS
    if (eController == AptInputController_Mouse)
    {
        ProcessButtonState();
    }
#endif
#endif
}

void AptAnimationTarget::ProcessAptInput(AptInput input, bool bCheck)
{
    APT_ASSERT(!INPUT_IS_KEY(&input) || !INPUT_IS_MOUSE(&input) || !INPUT_IS_ANALOG(&input) || !INPUT_IS_GESTURE(&input));

    AptInputType eType;
    AptInputState eState;
    AptInputController eController;
#if defined APT_USE_BUTTONS
    AptCIH *pNewButton = NULL;
#endif

#if defined(APT_USE_MOUSE)
    if (INPUT_IS_MOUSE(&input))
    {
        eType       = AptInputType_MouseMovement;
        eState      = AptInputState_MouseMove;
        eController = AptInputController_Mouse;
        GET_MOUSE_INPUT(input, snXMousePos, snYMousePos);
    }
    else
#endif
    {
        GET_INPUT(input, eType, eState, eController);
    }

    ProcessInputSet(eType, eState, input, eController, bCheck);
    ProcessListenerEvents(eType, eState, input, eController);
#if defined APT_USE_BUTTONS
    if (!HandleFocusButton(eType, eState, &pNewButton))
    {
        HandleAutoNav(pNewButton, eType, eState);
    }
#endif
}

#if defined APT_USE_BUTTONS
bool AptAnimationTarget::ValidateBIL()
{
#if defined(APT_DEBUG)
    for (int32_t i = snBILCount - 1; i >= 0; i--)
    {
        APT_ASSERT(saButtonInstanceList[i].pCIH != NULL);

        if (saButtonInstanceList[i].pCIH->isUndefined())
        {
            continue;
            // this situation will happen when the animation is still being initializing
            // and a mouse event come in
        }
        APT_ASSERT(saButtonInstanceList[i].pCIH->IsButtonInst());
    }
#endif
    return true;
}
#endif

void AptAnimationTarget::ProcessInputs()
{

    int32_t i;

    //// 06.03.04 FR217.  This is the StartDrag/StopDrag feature.  If the mpDragMC is set, then move it based on the mouse postion.
#if defined(APT_USE_MOUSE)
    if (mpDragMC->isUndefined() == false)
    {
        DragMovieClip();
    }
#endif
    for (i = 0; i < mnQueuedInputs; i++)
    {
        ProcessAptInput(maQueuedInputs[i], i == 0 ? true : false);
    }

    mnQueuedInputs = 0;

#if defined(APT_ALTERNATE_INPUT)
    ProcessAlternateEvents();
    mnQueuedAltInputs = 0;
#endif

}

// Begin Private Button Functions
#if defined APT_USE_BUTTONS
int32_t AptAnimationTarget::ActionConditionFlagToActionEventFlags(int32_t condition)
{
    int flags = 0;
    if (condition & AptActionConditionFlag_OverDownToOverUp)
    {
        flags |= AptEventActionFlag_Release;
    }
    if (condition & AptActionConditionFlag_OverUpToOverDown)
    {
        flags |= AptEventActionFlag_Press;
    }
    if (condition & AptActionConditionFlag_OutDownToIdle)
    {
        flags |= AptEventActionFlag_ReleaseOutside;
    }
    if (condition & AptActionConditionFlag_IdleToOverUp)
    {
        flags |= AptEventActionFlag_RollOver;
    }
    if (condition & AptActionConditionFlag_OverUpToIdle)
    {
        flags |= AptEventActionFlag_RollOut;
    }
    if (condition & AptActionConditionFlag_OutDownToOverDown)
    {
        flags |= AptEventActionFlag_DragOver;
    }
    if (condition & AptActionConditionFlag_OverDownToOutDown)
    {
        flags |= AptEventActionFlag_DragOut;
    }
    return flags;
}

void AptAnimationTarget::ProcessButtonState()
{
    AptCIH *pNewButton = GetButton(snXMousePos, snYMousePos);
    if (IsInputMasked(pNewButton))
    {
        pNewButton = 0;
    }

    APT_INCSAFE(pNewButton);

    // Move the focus to the new button
    if (mbButtonDown)
    {
        if (mpFocusButton && !mpFocusButton->isUndefined())
        {
            if (pNewButton != mpFocusButton)
            {
                if (mpFocusButton->GetButtonInst()->mnState == AptCharacterButtonRecordState_Down)
                {
                    mpFocusButton->gotoState(AptCharacterButtonRecordState_Over);
                    DoButtonActions(mpFocusButton, AptActionConditionFlag_OverDownToOutDown);
                }
            }
            else
            {
                if (mpFocusButton->GetButtonInst()->mnState == AptCharacterButtonRecordState_Over)
                {
                    mpFocusButton->gotoState(AptCharacterButtonRecordState_Down);
                    DoButtonActions(mpFocusButton, AptActionConditionFlag_OutDownToOverDown);
                }
            }
        }
        else
        {
            if (pNewButton && !pNewButton->isUndefined())
            {
                //              mpFocusButton->gotoState(AptCharacterButtonRecordState_Over);
                DoButtonActions(pNewButton, AptActionConditionFlag_OutDownToOverDown);
            }
        }
    }
    else if (pNewButton != mpFocusButton)
    {
        if (mpFocusButton && !mpFocusButton->isUndefined())
        {
            if (mpFocusButton->GetButtonInst()->mnState != AptCharacterButtonRecordState_Up)
            {
                // Don�t set the state unless it�s different.
                // Prevents extra onRollOver / onRollOut events from being fired.
                mpFocusButton->gotoState(AptCharacterButtonRecordState_Up);
                DoButtonActions(mpFocusButton, AptActionConditionFlag_OverUpToIdle);
            }
        }

        APT_DECSAFE(mpFocusButton);
        mpFocusButton = pNewButton;
        APT_INCSAFE(mpFocusButton);

        if (pNewButton && !pNewButton->isUndefined())
        {
            mpFocusButton->gotoState(AptCharacterButtonRecordState_Over);
            DoButtonActions(pNewButton, AptActionConditionFlag_IdleToOverUp);
        }
    }

    APT_DECSAFE(pNewButton)
}

bool AptAnimationTarget::HandleFocusButton(AptInputType eType, AptInputState eState, AptCIH **ppNewButton)
{
    APT_ASSERT(ppNewButton);
    *ppNewButton = NULL;

    // now try and handle aptmovie overrides
    if (mpFocusButton == NULL)
    {
        return false;
    }
    if (eState != AptInputState_Pressed)
    {
        return false;
    }

    const AptNativeString *pStrText;

    switch (eType)
    {
    case AptInputType_Up:
        pStrText = StringPool::GetString(SC__up);
        break;
    case AptInputType_Down:
        pStrText = StringPool::GetString(SC__down);
        break;
    case AptInputType_Left:
        pStrText = StringPool::GetString(SC__left);
        break;
    case AptInputType_Right:
        pStrText = StringPool::GetString(SC__right);
        break;
    default:
        //  Key not handled
        return false;
    }

    AptCIH *pParent  = mpFocusButton->GetDisplayListParent();
    AptValue *pValue = pParent->GetCharacterInst()->GetNativeHash()->Lookup(pStrText);
    if (pValue == NULL)
    {
        return false;
    }
    if (pValue->isString())
    {
        AptNativeString sVar;
        AptValue *pObject;

        gAptActionInterpreter.getContext(pParent, NULL, pValue->c_string()->GetInternalString(), &pObject, sVar);
        if (pObject)
        {
            AptCIH *pInst = pObject->c_cih();
            if (sVar.Size() == 0 && (pInst->IsButtonInst() || pInst->IsSpriteInstBase()))
            {
                *ppNewButton = pInst;
            }
            else if (sVar.Size() > 0 && pInst->IsSpriteInstBase())
            {
                AptValue *pValue2 = pInst->GetCharacterInst()->GetNativeHash()->Lookup(&sVar);

                // use function as atom
                pValue = pValue2 ? pValue2 : pValue;
            }
        }
    }
    if (pValue->isScriptFunction())
    {
        // Simplified Logic significantly.
        // void * pSavedValue   = gAptActionInterpreter.PrepareForExecution("_handleFocusButton");
        APT_DEFINE_ACTION_SETUP(pParent, pValue, pStrText->GetBuffer(), AptActionType_ButtonActions);
        void *pSavedValue = gAptActionInterpreter.PrepareForExecution(&oActionSetup);

        gAptActionInterpreter.callFunction(pParent, pValue, 0);
        // gAptActionInterpreter.CleanupAfterExecution("_handleFocusButton", pSavedValue);
        gAptActionInterpreter.CleanupAfterExecution(pSavedValue, &oActionSetup); // oActionSetup is defined by the macro

        AptValue *pReturnedValue = gAptActionInterpreter.stackAt(0);
        // if the return value is a pcharacter, try and use it as new focus
        if (pReturnedValue->isCIH())
        {
            APT_INC(pReturnedValue);
            gAptActionInterpreter.stackPop();
            pValue = pReturnedValue;
        }
        else
        {
            gAptActionInterpreter.stackPop();
            return true;
        }
    }
    else
    {
        APT_INC(pValue);
    }

    if (pValue->c_cih()->IsButtonInst() || pValue->c_cih()->IsSpriteInstBase())
    {
        if (!IsInputMasked(pValue->c_cih()))
            *ppNewButton = pValue->c_cih();
    }
    APT_DEC(pValue);
    return false;
}

void AptAnimationTarget::HandleAutoNav(AptCIH *pNewButton, AptInputType eType, AptInputState eState)
{
    // This is a set, so we need to iterate through the whole thing
    switch (eType)
    {
    case AptInputType_Up:
    case AptInputType_Down:
    case AptInputType_Left:
    case AptInputType_Right:
        // Only move if the padbutton isn't down
        // (note that normally, this isn't an issue in videogames because an option is selected on press, not release like a mouse)
        if (!mbButtonDown && eState == AptInputState_Pressed)
        {
            // make sure we have a valid focus button
            SetValidFocusButton();

            if (mpFocusButton == NULL)
                break;

            // if we have already found a new button just use that one
            if (pNewButton == NULL)
            {
                pNewButton = GetBestButton(eType, mpFocusButton->GetDisplayListParent(), mpFocusButton); // GetBestButton defined in AptAnimationTargetButtonFunction.h
            }
            // if the new button we found is actually a sprite, find an appropriate button within the sprite
            else if (pNewButton->IsSpriteInstBase())
            {
                pNewButton = GetBestButton(eType, pNewButton, NULL); // GetBestButton defined in AptAnimationTargetButtonFunction.h
            }

            // Move the focus to the new button
            if (pNewButton)
            {
                mpFocusButton->gotoState(AptCharacterButtonRecordState_Up);
                pNewButton->gotoState(AptCharacterButtonRecordState_Over);
                // Run event action code
                DoButtonActions(mpFocusButton, AptActionConditionFlag_OverUpToIdle);
                DoButtonActions(pNewButton, AptActionConditionFlag_IdleToOverUp);

                APT_DECSAFE(mpFocusButton);
                mpFocusButton = pNewButton;
                APT_INCSAFE(mpFocusButton);
            }
        }
        break;
#if defined(APT_USE_MOUSE)
        // Press/release - change state, do actions and stuff
    case AptInputType_MouseButton0:
        if (mpFocusButton == NULL)
        {
            if (eState == AptInputState_Released)
            {
                mbButtonDown = false;
                if (mpFocusButton)
                {
                    mpFocusButton->gotoState(AptCharacterButtonRecordState_Over);
                    DoButtonActions(mpFocusButton, AptActionConditionFlag_IdleToOverUp);
                }
            }
            else
            {
                mbButtonDown = true;
            }
            break;
        }
        if (!mbButtonDown && eState == AptInputState_Pressed)
        {
            mbButtonDown = true;
            mpFocusButton->gotoState(AptCharacterButtonRecordState_Down);
            DoButtonActions(mpFocusButton, AptActionConditionFlag_OverUpToOverDown);
        }
        if (mbButtonDown && eState == AptInputState_Released)
        {
            mbButtonDown = false;
            if (mpFocusButton->GetButtonInst()->mnState == AptCharacterButtonRecordState_Over)
            {
                mpFocusButton->gotoState(AptCharacterButtonRecordState_Up);
                DoButtonActions(mpFocusButton, AptActionConditionFlag_OutDownToIdle);

                if (mpDragMC == mpFocusButton->GetDisplayListParent())
                {
                    mpFocusButton->gotoState(AptCharacterButtonRecordState_Over);
                    DoButtonActions(mpFocusButton, AptActionConditionFlag_OverDownToOverUp);
                }
            }
            else
            {
                mpFocusButton->gotoState(AptCharacterButtonRecordState_Over);
                DoButtonActions(mpFocusButton, AptActionConditionFlag_OverDownToOverUp);
            }
        }
        break;
#endif
    default:
        break;
    }
}

void AptAnimationTarget::DoButtonActions(AptCIH *pInst, int nTransition)
{
    APT_ASSERT(pInst);
    APT_ASSERT(pInst->IsButtonInst());

    const AptCharacter *pChar = pInst->GetCharacterInst()->GetCharacterConst();

    int32_t i;
    for (i = 0; i < pChar->button.nActionConditions; i++)
    {
        if (pChar->button.aActionConditions[i].nConditions & nTransition)
        {
            APT_ASSERT(pInst->GetDisplayListParent());
            AddActionBack(&pChar->button.aActionConditions[i].actions, pInst->GetDisplayListParent(),
                              ACTION_TYPE_CALL_PARAM(AptActionType_ButtonActions)
                                  gNullInput);
        }
    }
    int32_t nEventFlag = ActionConditionFlagToActionEventFlags(nTransition);
    if (pInst->GetNativeHashVirtual()->HasEventHandler(nEventFlag))
    {
        for (i = 0; i < APT_ARRAYSIZE(_aInputFlags); i++)
        {
            if (nEventFlag & _aInputFlags[i].nFlag)
            {
                // buttons now have a hash
                AptValue *pValue = pInst->GetNativeHash()->Lookup(StringPool::GetString(_aInputFlags[i].eName));
                if (pValue)
                {
                    AddFunctionBack(pInst, pValue, 0,
                                        ACTION_TYPE_CALL_PARAM(_aClipEvents[i].nFlag)
                                            SET_KEYBOARD_INPUT(AptInputType_MouseButton0, _aInputFlags[i].nInputState, AptInputController_Mouse));
                }
            }
        }
    }

#if defined(APT_USE_SOUND_OBJECT)
    if (pChar->button.pButtonSound)
    {
        switch (nTransition)
        {
        case AptActionConditionFlag_OverUpToIdle:
            if (pChar->button.pButtonSound->pOverUpToIdle)
            {
                AptGetUserFuncs().pfnStartSound(pChar->button.pButtonSound->pOverUpToIdle->sound.zID, NULL);
                // AptGetUserFuncs().pfnStartSound(pChar->button.pButtonSound->pOverUpToIdle->sound.zID);
            }
            break;
        case AptActionConditionFlag_IdleToOverUp:
            if (pChar->button.pButtonSound->pIdleToOverUp)
            {
                AptGetUserFuncs().pfnStartSound(pChar->button.pButtonSound->pIdleToOverUp->sound.zID, NULL);
                // AptGetUserFuncs().pfnStartSound(pChar->button.pButtonSound->pIdleToOverUp->sound.zID);
            }
            break;
        case AptActionConditionFlag_OverUpToOverDown:
            if (pChar->button.pButtonSound->pOverUpToOverDown)
            {
                AptGetUserFuncs().pfnStartSound(pChar->button.pButtonSound->pOverUpToOverDown->sound.zID, NULL);
                // AptGetUserFuncs().pfnStartSound(pChar->button.pButtonSound->pOverUpToOverDown->sound.zID);
            }
            break;
        case AptActionConditionFlag_OverDownToOverUp:
            if (pChar->button.pButtonSound->pOverDownToOverUp)
            {
                AptGetUserFuncs().pfnStartSound(pChar->button.pButtonSound->pOverDownToOverUp->sound.zID, NULL);
                // AptGetUserFuncs().pfnStartSound(pChar->button.pButtonSound->pOverDownToOverUp->sound.zID);
            }
            break;
        }
    }
#endif
    // removing this runactions from here - 1.01.00 ASK
    // because of new CleanUpRemList it might create problem if the run actions deletes
    // any movieclip which is accessed from any of these functions.
    //    RunActions();
}

bool AptAnimationTarget::IsPointInButtonHitTestRegion(const AptCharacterButton *button, AptMatrix *pMatrix, int32_t nX, int32_t nY)
{
    int32_t nTriangle;
    float fX = (float)nX;
    float fY = (float)nY;

    // reject by bounding rect first
    float fRect[4];
    float fRectTransformed[4];
    fRect[0] = button->mHitTestBoundingRect.fLeft;
    fRect[1] = button->mHitTestBoundingRect.fTop;
    fRect[2] = button->mHitTestBoundingRect.fRight;
    fRect[3] = button->mHitTestBoundingRect.fBottom;

    // TODO
    // ideally we should build an inverse matrix of pMatrix and transform the x/y into local space
    // instead of transform boundbox and all the vertex into world space
    // Or take Paul's suggestion, avoid triangle totally

    // transform the bounding rect into worldspace
    MatrixVecMult(&fRect[0], pMatrix, &fRectTransformed[0]);
    MatrixVecMult(&fRect[2], pMatrix, &fRectTransformed[2]);

    if (fRectTransformed[0] > fRectTransformed[2])
    {
        float t             = fRectTransformed[2];
        fRectTransformed[2] = fRectTransformed[0];
        fRectTransformed[0] = t;
    }
    if (fRectTransformed[1] > fRectTransformed[3])
    {
        float t             = fRectTransformed[3];
        fRectTransformed[3] = fRectTransformed[1];
        fRectTransformed[1] = t;
    }

    // do a fast reject
    if (fX < fRectTransformed[0] ||
        fX > fRectTransformed[2] ||
        fY < fRectTransformed[1] ||
        fY > fRectTransformed[3])
    {
        // return false;    //### the fast reject will break on certain cases, do nothing for now...fixed in 0.7.11
    }

    // for each triangle
    for (nTriangle = 0; nTriangle < button->mHitTestTriangles; nTriangle++)
    {
        float *p   = button->mHitTestVertexTable;
        short *idx = button->mHitTestIndexTable + nTriangle * 3;

        // transform teh 3 vertex into world space
        float v[6];
        MatrixVecMult(&p[*idx * 2], pMatrix, &v[0]);
        MatrixVecMult(&p[*(idx + 1) * 2], pMatrix, &v[2]);
        MatrixVecMult(&p[*(idx + 2) * 2], pMatrix, &v[4]);

        // test against x/y
        if (PointInTri(v, fX, fY))
        {
            return true;
        }
    }
    return false;
}

AptCIH *AptAnimationTarget::GetButton(int nX, int nY)
{
#if defined APT_USE_MOUSE
    // should test in the reverse order to the drawing order
    for (int32_t i = snBILCount - 1; i >= 0; i--)
    {
        AptCIH *cih = saButtonInstanceList[i].pCIH;
        APT_ASSERT(cih != NULL);
        AptMatrix *instanceMatrix = &saButtonInstanceList[i].matrix;

        if (cih->isUndefined())
        {
            continue;
            // this situation will happen when the animation is still being initializing
            // and a mouse event come in
        }
        APT_ASSERT(cih->IsButtonInst());

        const AptCharacterButton *pButtonChar = &cih->GetCharacterInst()->GetCharacterConst()->button;
        APT_ASSERT(cih->GetCharacterInst()->GetCharacterConst()->eType == AptCharacterType_Button);

        bool hitsButton = false;

        for (int32_t nRecord = 0; nRecord < pButtonChar->nButtonRecords && !hitsButton; nRecord++)
        {
            AptCharacterButtonRecord &record = pButtonChar->aButtonRecords[nRecord];
            if (!(record.eStates & AptCharacterButtonRecordState_HitTest))
            {
                continue;
            }
            AptMatrix mat = record.matrix;
            AptRenderingContext::multMatrix(instanceMatrix, &mat, &mat);

            AptCharacter *pChar = record.pCharacter;
            switch (pChar->eType)
            {
            case AptCharacterType_Shape:
                hitsButton = IsPointInButtonHitTestRegion(pButtonChar, &mat, nX, nY);
                break;
            case AptCharacterType_Text:
                hitsButton = IsPointInRect(pChar->text.rBounds, mat, nX, nY);
                break;
            default:
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "not hit testing: unhandled type\n");
                break;
            }
        }
        // We know that the mouse cursor hits the button. Now we need to know if it hits also the mask if any
        if (hitsButton)
        {
            // Get top-level assigned mask, if any
            AptCIH *parent = cih;
            AptCIH *mask   = NULL;
            while (parent && mask == NULL)
            {
                mask   = parent->GetMask();
                parent = (AptCIH *)parent->GetDisplayListParent();
            }

            if (mask)
            {
                // hit-test against mask
                hitsButton = IsPointInCIH(mask, nX, nY);
            }

            // hit-test any other masks in mask layer
            for (AptCIH *prev = cih->GetDisplayListPrevious(); hitsButton && prev; prev = prev->GetDisplayListPrevious())
            {
                if (prev->GetCharacterInst()->GetRenderItem()->GetClipDepth() >= 0) // It's a clipper
                {
                    hitsButton = IsPointInCIH(prev, nX, nY);
                }
            }
            if (hitsButton)
            {
                return cih;
            }
        }
    } // for i
#endif
    return NULL;
}
#endif
// End Private Button Functions

// Begin Private Mouse Functions
#if defined APT_USE_MOUSE
void AptAnimationTarget::ProcessMouseState(bool bPress, bool bPressRelease, AptInput input)
{

    if (bPressRelease)
    {
        // We have a onRelease event, so we need to see if the it's a release outside, or simply just a release event.
        // This sprite was previously pressed, set the release event to either release, or releaseOutside
        if (!mpOnPress->isUndefined())
        {
            if (!mpDragMC->isUndefined())
            {
                if (mpDragMC == mpOnPress)
                    mpOnPress->c_cih()->queueClipEvents(AptEventActionFlag_Release, input);
                else
                    mpOnPress->c_cih()->queueClipEvents(AptEventActionFlag_ReleaseOutside, input);
            }
            else if (PointHits(mpOnPress->c_cih(), snXMousePos, snYMousePos) && mpTopMostSprite == mpOnPress)
            {
                mpOnPress->c_cih()->queueClipEvents(AptEventActionFlag_Release, input);
            }
            else
            {
                mpOnPress->c_cih()->queueClipEvents(AptEventActionFlag_ReleaseOutside, input);
            }
            mpOnPress = gpUndefinedValue;
        }
    }
    else if (bPress)
    {
        // We have a onPress event, send the event to the top most sprite
        if (mpTopMostSprite->isUndefined() == false)
        {
            // We now have to send a onPress event for this sprite
            mpTopMostSprite->c_cih()->queueClipEvents(AptEventActionFlag_Press, input);
            mpOnPress = mpTopMostSprite;
        }
    }
    else if (!mpOnPress->isUndefined())
    {
        // If the current mpOnPress sprite is defined, check it for a drag over or out event    F#563   7
        bool bHitTest = PointHits(mpOnPress->c_cih(), snXMousePos, snYMousePos);
        if (!mpOnRollOver->isUndefined() && !bHitTest)
        {
            mpOnPress->c_cih()->queueClipEvents(AptEventActionFlag_DragOut, input);
            mpOnRollOver = gpUndefinedValue;
        }
        else if (mpOnRollOver->isUndefined() && bHitTest)
        {
            mpOnPress->c_cih()->queueClipEvents(AptEventActionFlag_DragOver, input);
            mpOnRollOver = mpOnPress;
        }
    }
    else
    {
        if (!mpTopMostSprite->isUndefined() && mpTopMostSprite != mpOnRollOver)
        {
            if (!mpOnRollOver->isUndefined() && mpOnRollOver != mpOnPress)
            {
                // The last rolledOn sprite has a sprite on top; send it the rollOut event before sending the new rolledOn sprite its event
                mpOnRollOver->c_cih()->queueClipEvents(AptEventActionFlag_RollOut, input);
            }
            mpOnRollOver = mpTopMostSprite;
            mpOnRollOver->c_cih()->queueClipEvents(AptEventActionFlag_RollOver, input);
        }
        else if (!mpOnRollOver->isUndefined() && mpTopMostSprite != mpOnRollOver && mpOnPress->isUndefined() &&
                 !PointHits(mpOnRollOver->c_cih(), snXMousePos, snYMousePos))
        {
            mpOnRollOver->c_cih()->queueClipEvents(AptEventActionFlag_RollOut, input);
            mpOnRollOver = gpUndefinedValue;
        }
    }
}
#endif
// End Private Mouse Functions

/*** Public Functions *****************************************************************************/
AptAnimationTarget::AptAnimationTarget(const AptTargetInitParams *aptInitParms)
    : miMaxIntervalTimers(aptInitParms->iMaxIntervalFunctions),
      miMaxQueuedInputs(aptInitParms->iMaxQueuedInputs),
      mListenerSet(aptInitParms->iListenerSetSize),
      mInputSet(aptInitParms->iInputSetSize),
      mDisplayList()
#if defined APT_USE_BUTTONS
      ,
      mButtonSet(aptInitParms->iButtonSetSize)
#endif
#if defined(APT_USE_MOUSE)
      ,
      mMouseListenerSet(aptInitParms->iListenerSetSize)
#endif
#if defined(APT_ALTERNATE_INPUT)
      ,
      mAltListenerSet(aptInitParms->iListenerSetSize)
#endif
{
    APT_ASSERT(AptGetUserFuncs().pfnMemAlloc);

    mDragPos.a = mDragPos.b = mDragPos.c = mDragPos.d = mDragPos.tx = mDragPos.ty = 0;

    mpAptActionPool = new AptActionQueueC(aptInitParms->iActionPoolSize);

    maIntervalTimers = new AptIntervalTimer[miMaxIntervalTimers];
    APT_ASSERT(maIntervalTimers != 0);

    APT_ASSERT(miMaxQueuedInputs != 0);
    maQueuedInputs = APT_MALLOC_ARRAY(AptInput, miMaxQueuedInputs);
    APT_ASSERT(maQueuedInputs != NULL);

    mnQueuedInputs = 0;

    mpInputMask     = NULL;
    mpDragMC        = gpUndefinedValue;
    mpOnPress       = gpUndefinedValue;
    mpOnRollOver    = gpUndefinedValue;
    mpTopMostSprite = gpUndefinedValue;

#if defined APT_USE_BUTTONS
    mpFocusButton = NULL;
    mbButtonDown  = false;
    snBILCount    = 0;
#endif

#if defined(APT_ALTERNATE_INPUT)
    maQueuedAltInputs = new AptAltInput[miMaxQueuedInputs];
    mnQueuedAltInputs = 0;
#endif
}

void AptAnimationTarget::SetupStaticData(const AptInitParams &aptInitParms)
{
#if defined APT_USE_BUTTONS
    siButtonInstanceListSize = aptInitParms.iMaxButtonInstances;
    if (siButtonInstanceListSize == 0)
        saButtonInstanceList = NULL;
    else
        saButtonInstanceList = new ButtonHitTestRecord[siButtonInstanceListSize];
#endif

    siMaxNewMovieClips = aptInitParms.iMaxNewMovieClipsPerFrame;
    APT_ASSERT(siMaxNewMovieClips != 0);

    // Set initial vector sizes to the assumed max capacity.
    sapNewInsts.resize(aptInitParms.iMaxNewMovieClipsPerFrame);
    sapDelayedReleaseList.resize(aptInitParms.iMaxNewMovieClipsPerFrame);

    snNewInsts            = 0;
    snDelayedReleaseCount = 0;
}

void AptAnimationTarget::CleanupStaticData()
{
    AptAnimationCihList().swap(sapNewInsts);
    AptAnimationCihList().swap(sapDelayedReleaseList);

#if defined APT_USE_BUTTONS
    if (saButtonInstanceList != NULL)
        delete[] saButtonInstanceList;
#endif
}
void AptAnimationTarget::AddToRemList(AptCIH *pItem)
{
    if (pItem->GetInRemList()) // if it is already in the rem list, return.
        return;

    // Grow the delayed release list if we're at capacity
    if (sapDelayedReleaseList.capacity() == (size_t)snDelayedReleaseCount)
    {
        sapDelayedReleaseList.resize(sapDelayedReleaseList.capacity() + siNewMovieClipsGrowSize);
    }

    int32_t i = snDelayedReleaseCount;
    APT_ASSERT(sapDelayedReleaseList[i] == NULL);

    APT_INC(pItem);
    pItem->SetInRemList(true);
    sapDelayedReleaseList[i] = pItem;

    if (i == snDelayedReleaseCount)
        snDelayedReleaseCount++;

    if (snDelayedReleaseCount > snDelayedReleaseCountHW)
    {
        snDelayedReleaseCountHW = snDelayedReleaseCount;
    }
}

// #include <xtl.h>
// #include <ppcintrinsics.h>

// Added reference function RemoveReferences that gets called from AptAnimationTarget::CleanRemList
extern void RemoveReferences(AptValue **pAllocatedValues, int32_t nItemsInArray);

void AptAnimationTarget::CleanRemList()
{
    // uint64_t startTime3 = 0;
    // uint64_t startTime4 = 0;
    int32_t nActualItemsToBeReplaced = 0;
    if (snDelayedReleaseCount > 0)
    {
        // QueryPerformanceCounter((union _LARGE_INTEGER*)&startTime3);
    }

    for (int32_t i = 0; i < snDelayedReleaseCount; i++)
    {
        AptCIH *pObj = sapDelayedReleaseList[i];
        APT_ASSERT(pObj->GetInRemList());
        sapDelayedReleaseList[i] = NULL;

        if (pObj != NULL)
        {
            // we will use the GCMark to mark that an item needs to be deleted
            APT_ASSERT(pObj->getGCMark() == false);

            if (!pObj->isCIH(true))
                continue;
            if (pObj->c_cih(true)->GetCIHState() == AptCIH::AptCIHState_Zombie || pObj->isNone())
            {
                continue;
            }
            if (pObj->getRefCount() > 1)
            {
                pObj->PreDestroy();
                pObj->DestroyGCPointers();

                if (!pObj->isCIH(true))
                {
                    continue;
                }
                if (pObj->getRefCount() == 1)
                {
                    APT_DEC(pObj); // All done, it can go away now.
                }
                else
                {
                    sapDelayedReleaseList[nActualItemsToBeReplaced++] = pObj;
                    pObj->setGCMark(true); // mark object so that we can delete all of these with one pass through the AptValues
                }
            }
            else if (pObj->getRefCount() == 1)
            {
                APT_DEC(pObj);
            }
            else
            {
                APT_ASSERT(0 && "Reference counting issue encountered");
            }
        }
    }

    // update size of the sapDelayedReleaseList
    snDelayedReleaseCount = nActualItemsToBeReplaced;

    AptValue **pAllocatedValues = NULL;
    int32_t nPrintCycles        = 0;
    if (nActualItemsToBeReplaced > 0)
    {
        pAllocatedValues      = GetGCPoolManager()->GetAllAllocatedAptValues();
        int32_t nItemsInArray = (int32_t)GetGCPoolManager()->mnItemsAllocated;

        // delete references
        RemoveReferences(pAllocatedValues, nItemsInArray);

        // clean up
        for (int32_t j = 0; j < nActualItemsToBeReplaced; j++)
        {
            AptCIH *pObj             = sapDelayedReleaseList[j];
            sapDelayedReleaseList[j] = NULL;

            if (pObj)
            {
                pObj->setGCMark(false); // unmark the object

                // Note, we're not calling pObj->ForceDelete() when the refcount != 1, because the
                // object might be referenced by something in AptGetLib()->mpValuesToRelease. In that case (which
                // should be very rare in practice) it would eventually be cleaned up during Apt shutdown.
                APT_DEC(pObj);

                nPrintCycles++;
            }
        }

        GetGCPoolManager()->ReleaseAllocatedAptValuesArray(pAllocatedValues, nItemsInArray);
    }

    if (snDelayedReleaseCount > 0 && nPrintCycles > 0)
    {
        // QueryPerformanceCounter((union _LARGE_INTEGER*)&startTime4);
        // printf("CleanRemList took - %I64u, %d \n", startTime4 - startTime3, nPrintCycles);
    }

    snDelayedReleaseCount = 0;
}

AptAnimationTarget::~AptAnimationTarget()
{
    // free the new inst memory
    APT_FREE_ARRAY(maQueuedInputs, AptInput, miMaxQueuedInputs);
    delete[] maIntervalTimers;
    delete mpAptActionPool;

#if defined(APT_ALTERNATE_INPUT)
    delete[] maQueuedAltInputs;
    maQueuedAltInputs = NULL;
#endif

    mpInputMask     = NULL;
    maQueuedInputs  = NULL;
    mpOnPress       = NULL;
    mpOnRollOver    = NULL;
    mpTopMostSprite = NULL;
    mpDragMC        = NULL;
}

void AptAnimationTarget::PreDestroy(void)
{
#if defined APT_USE_BUTTONS
    int32_t nCount = mButtonSet.mnElements;

    // XXX TODO this loop can be removed because it should be done in the dtor?
    for (int32_t i = 0; i < mButtonSet.capacity(); i++)
    {
        if (mButtonSet.maElements[i])
        {
            mButtonSet.remove(mButtonSet.maElements[i]);
            nCount--;
            if (nCount == 0)
                break;
        }
    }
#endif

    // clear the interval timers
    for (int32_t i = 0; i < GetMaxIntervalTimers(); i++)
    {
        if (maIntervalTimers[i].bValid == false)
        {
            continue;
        }
        APT_DEC(maIntervalTimers[i].pCBFunction);
        APT_DECSAFE(maIntervalTimers[i].pContext);
        int32_t j;
        for (j = 0; j < maIntervalTimers[i].pParams.size(); ++j)
        {
            maIntervalTimers[i].CleanParams();
        }
        maIntervalTimers[i].bValid = false;
    }

    // clear  listener events.
    // instead of calling remove on those elements just doing same functionality here.
    for (int32_t i = 0; i < mListenerSet.capacity(); i++)
    {
        if (mListenerSet.maElements[i] == NULL)
            continue;
        APT_DECSAFE(mListenerSet.maElements[i]);
        mListenerSet.maElements[i] = 0;
    }

#if defined(APT_USE_MOUSE)
    // do not Skip this code if the mouse flag is false
    // as we are adding mouselisteners to this set irresepective of bUseMouse.
    for (int32_t i = 0; i < mMouseListenerSet.capacity(); i++)
    {
        if (mMouseListenerSet.maElements[i] == NULL)
            continue;
        APT_DECSAFE(mMouseListenerSet.maElements[i]);
        mMouseListenerSet.maElements[i] = 0;
    }
#endif

#if defined(APT_ALTERNATE_INPUT)
    for (int32_t i = 0; i < mAltListenerSet.capacity(); i++)
    {
        if (mAltListenerSet.maElements[i] == NULL)
            continue;
        APT_DECSAFE(mAltListenerSet.maElements[i]);
        mAltListenerSet.maElements[i] = 0;
    }
#endif

#if defined APT_USE_BUTTONS
    ClearBIL();
#endif
    mDisplayList.PreDestroy();
}

void AptAnimationTarget::Reset()
{
    mnQueuedInputs = 0;
#if defined APT_USE_BUTTONS
    mpFocusButton = NULL;
    mbButtonDown  = false;
    snBILCount    = 0;
#endif
    snXMousePos = 0;
    snYMousePos = 0;

    mpInputMask     = NULL;
    mpDragMC        = gpUndefinedValue;
    mpOnPress       = gpUndefinedValue;
    mpOnRollOver    = gpUndefinedValue;
    mpTopMostSprite = gpUndefinedValue;

#if defined(APT_ALTERNATE_INPUT)
    mnQueuedAltInputs = 0;
#endif
}

void AptAnimationTarget::TickIntervalTimers(int32_t nMilliseconds)
{

    for (int32_t i = 0; i < GetMaxIntervalTimers(); ++i)
    {
        if (maIntervalTimers[i].bValid == false)
        {
            continue;
        }

        maIntervalTimers[i].fCurTime -= nMilliseconds;

        if (maIntervalTimers[i].fCurTime < 0)
        {
            AptValue *pFuncValue = maIntervalTimers[i].pCBFunction;
            AptCIH *pScriptCIH   = (pFuncValue && pFuncValue->isScriptFunction()) ? pFuncValue->c_scriptfunction()->mpCIH : NULL;

            if (pFuncValue && (pFuncValue->isNativeFunction() || pFuncValue->isExternalFunction() || (pScriptCIH && !pScriptCIH->isUndefined() && !pScriptCIH->IsLevelInst())))
            {
                // Changed to use AptScriptFunctionBase (Reduces duplication of code).
                AptValue *pContext = maIntervalTimers[i].pContext != gpUndefinedValue ? maIntervalTimers[i].pContext : pScriptCIH;
                int32_t nParams    = maIntervalTimers[i].pParams.size();

                APT_INCSAFE(pContext);

                // void * pSavedValue   = gAptActionInterpreter.PrepareForExecution("tickIntervalTimers");
                APT_DEFINE_ACTION_SETUP(pContext, pFuncValue, "AptTimerFunc", AptActionType_Timer);

#if defined(APT_DEBUG)
#else
#endif

                void *pSavedValue = gAptActionInterpreter.PrepareForExecution(&oActionSetup);


                // HERE WE NEED TO PUSH ALL maIntervalTimers[i].pParams onto stack and replace the -1 with the num of params
                for (int32_t j = 0; j < nParams; j++)
                {
                    gAptActionInterpreter.stackPush(maIntervalTimers[i].pParams.at(j));
                }

                // moved following line above the function call
                maIntervalTimers[i].fCurTime += maIntervalTimers[i].fInterval;


#ifdef APT_DEBUGGER_ENABLE
                if (pScriptCIH)
                {
                    AptDebugger::GetInstance()->PushCallStack(pFuncValue, "AptTimerFunc", reinterpret_cast<unsigned char *const>(AptDebugger::INVALID_OFFSET), pScriptCIH);
                }
#endif

                gAptActionInterpreter.callFunction(pContext, pFuncValue, nParams);

#ifdef APT_DEBUGGER_ENABLE
                if (pScriptCIH)
                {
                    AptDebugger::GetInstance()->PopCallStack(pFuncValue);
                }
#endif



                // gAptActionInterpreter.CleanupAfterExecution("tickIntervalTimers", pSavedValue);
                gAptActionInterpreter.CleanupAfterExecution(pSavedValue, &oActionSetup);


                // Should only have one Item left on the stack.
                APT_ASSERT(gAptActionInterpreter.stack.GetSize() == 1);

                // Either way we Get clear out the stack elements.
                gAptActionInterpreter.stack.Pop(gAptActionInterpreter.stack.GetSize());

                // moved following line above the function call
                // maIntervalTimers[i].fCurTime += maIntervalTimers[i].fInterval;
                APT_DECSAFE(pContext);
            }
            else
            {
                // The function can't be handled by the tickIntervalTimers
                //  We need to remove it from the list
                APT_DEC(maIntervalTimers[i].pCBFunction);
                APT_DECSAFE(maIntervalTimers[i].pContext);
                maIntervalTimers[i].CleanParams();
                maIntervalTimers[i].bValid = false;
            }
        }
    }

}

void AptAnimationTarget::TickNewInsts(void)
{
    for (int32_t i = 0; i < snNewInsts; i++)
    {
        if (sapNewInsts[i] == NULL)
        {
            continue;
        }
#if defined APT_USE_BUTTONS
        if (sapNewInsts[i]->IsButtonInst())
        {
            AptCharacterButtonInst *pButtonInst = sapNewInsts[i]->GetButtonInst();
            if (pButtonInst->mnState == AptCharacterButtonRecordState_None)
            {
                sapNewInsts[i]->gotoState(AptCharacterButtonRecordState_Up);
            }
        }
        else
#endif
            if (sapNewInsts[i]->IsSpriteInst())
        {
            AptCharacterSpriteInst *pSprInst = sapNewInsts[i]->GetSpriteInst();
            if (pSprInst->mnFrame == -1)
            {
                sapNewInsts[i]->tick();
            }
        }
        APT_DECSAFE(sapNewInsts[i]);
        sapNewInsts[i] = NULL;
    }
    snNewInsts = 0;
}

#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
// when this #define is turned on, it tries to spread the actions executed over multiple frames to improve
// on spike frames. We tried testing it in Game NCAA 08 but it was not that helpful.
// But still keeping that code in, so we can try it out later if needed.
int32_t s_nMaxActionsToExecute = 10;
#endif
void AptAnimationTarget::RunActions()
{

    CleanRemList();

    AptActionQueueC *pCurrentPool        = GetActionPool();
    AptActionQueueC::AptActionPool *pCur = pCurrentPool->GetFirstItem();

#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
    int32_t nActionsExecuted = 0;
    int32_t nQueueSize       = pCurrentPool->GetDequeSize();
#endif

    // Runs all the actions in the display list action pool.
    {
        while (!pCurrentPool->IsLastItem(pCur))
        {
            // AptActionQueueC::AptActionPool *pOrigEnd = pCurrentPool->GetLastItem();
            pCurrentPool->SetCurItem(pCur); // Save off current pointer so we don't remove it from the action pool underneath ourselves.
            if (pCur->eActionType == AptActionQueueC::AAT_ACTION)
            {
                // the classic action
                gAptActionInterpreter.input = pCur->input;

                // Here we check if the nFrame is negative.  If so, we need to check if this action is running in the correct frame num for this sprite inst.
                if (pCur->action.pCIH && !pCur->action.pCIH->isUndefined() &&
                    !pCur->action.pCIH->IsLevelInst() &&
                    pCur->action.pCIH->GetCIHState() != AptCIH::AptCIHState_Unloaded &&
                    pCur->action.pCIH->IsSkipEval() == false &&
                    !(pCur->action.nFrame < 0 && (pCur->action.pCIH->GetCharacterInst() != NULL && (pCur->action.nFrame * -1) != pCur->action.pCIH->GetSpriteInstBase()->mnFrame))) // Null check required in AptAnimationTarget::runActions.
                {
                    APT_DEFINE_ACTION_SETUP(pCur->action.pCIH, NULL, "AptRun-Actions", pCur->eAptActionType);

#if defined(APT_DEBUG)
#else
#endif

                    void *pSavedValue = gAptActionInterpreter.PrepareForExecution(&oActionSetup);


                    AptCharacterInst *pCharacterInst = (pCur->action.pCIH && !pCur->action.pCIH->isNone()) ? pCur->action.pCIH->GetRootAnimation()->GetAnimationInst() : NULL;

#ifdef APT_DEBUGGER_ENABLE
                    AptDebugger::GetInstance()->PushCallStack(pCur->action.pCIH, "Frame time", pCur->action.pBlock->aActionStream, pCur->action.pCIH);
#endif

                    gAptActionInterpreter.runStream(pCur->action.pBlock->aActionStream, pCur->action.pCIH, -1, pCharacterInst);

                    gAptActionInterpreter.CleanupAfterExecution(pSavedValue, &oActionSetup);

#ifdef APT_DEBUGGER_ENABLE
                    AptDebugger::GetInstance()->PopCallStack(pCur->action.pCIH);
#endif

                    APT_ASSERT(_AptValidate());
                    TickNewInsts();
                }
#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
                nActionsExecuted++;
#endif
            }
            else if (pCur->eActionType == AptActionQueueC::AAT_FUNCTION)
            {
                // queued function
                gAptActionInterpreter.input = pCur->input;
                gAptActionInterpreter.thisStack.push(pCur->function.pContext);

                APT_DEFINE_ACTION_SETUP(pCur->function.pContext, pCur->function.pFuncDef, "AptRun-Functions", pCur->eAptActionType);

#if defined(APT_DEBUG)
#else
#endif

                void *pSavedValue = gAptActionInterpreter.PrepareForExecution(&oActionSetup);


#if defined(APT_USE_MOUSE) // All mouse events are queued as keyboard events, except mouse movements
                if (pCur->input && INPUT_IS_KEY(&pCur->input))
                {
                    AptInputController eController;
                    GET_ACTION_CONTROLLER(pCur->input, eController);
                    if (eController == AptInputController_Mouse) // F#563    5
                    {
                        // only do this if we have input and the input is from the mouse wheel
                        int32_t eType;
                        AptInputState eState;
                        GET_INPUT(pCur->input, eType, eState, eController);
                        if (pCur->function.nParams > 0 && (eState & (AptInputState_WheelDown | AptInputState_WheelUp)))
                        {
                            if (pCur->function.nParams > 1)
                            {
                                gAptActionInterpreter.stackPush(mpTopMostSprite); // If we have more than one param, push the top most sprite on top
                            }

                            if (eState == AptInputState_WheelDown) // No push the mouse wheel data, up or down
                            {
                                gAptActionInterpreter.stackPush(AptInteger::Create(-eType));
                            }
                            else
                            {
                                gAptActionInterpreter.stackPush(AptInteger::Create(eType));
                            }
                        }
                    }
                }
#endif


                {
                    // The function description is more accurate when APT_DEBUG
                    // is defined. Otherwise the description is simply "<movieclipevent>".
                    const char *functionDescription = _GetMovieClipEventFunctionDescription(pCur);

#ifdef APT_DEBUGGER_ENABLE
                    bool isScriptActionFlag = AptDebugger::GetInstance()->PushCallStack(pCur->function.pFuncDef, functionDescription, reinterpret_cast<unsigned char *const>(AptDebugger::INVALID_OFFSET), pCur->function.pContext);
#endif

                    gAptOptCallStack->Push(functionDescription, NULL, NULL, NULL);

                    // WithStack needs a reference to the object so if we call super.onLoad (or any super stuff, really), we get the
                    // right prototype context.  Otherwise we're going to call the same onLoad function twice. Colin C. 8/28/12
                    gAptActionInterpreter.withStack.push(pCur->function.pContext);

                    gAptActionInterpreter.callFunction(
                        pCur->function.pContext,
                        pCur->function.pFuncDef,
                        pCur->function.nParams,
                        NULL,
                        NULL,
                        true);

                    gAptActionInterpreter.withStack.pop();

                    gAptOptCallStack->Pop();

#ifdef APT_DEBUGGER_ENABLE
                    if (isScriptActionFlag)
                    {
                        AptDebugger::GetInstance()->PopCallStack(pCur->function.pFuncDef);
                    }
#endif
                }


                gAptActionInterpreter.CleanupAfterExecution(pSavedValue, &oActionSetup);

                gAptActionInterpreter.thisStack.pop();
                gAptActionInterpreter.stack.Pop();
                TickNewInsts();
#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
                nActionsExecuted++;
#endif
            }
#if defined(APT_ALTERNATE_INPUT)
            else if (pCur->eActionType == AptActionQueueC::AAT_ALT_INP_FUNCTION)
            {
                // queued function
                // gAptActionInterpreter.input = pCur->input;
                gAptActionInterpreter.thisStack.push(pCur->function.pContext);

                APT_DEFINE_ACTION_SETUP(pCur->function.pContext, pCur->function.pFuncDef, "AptRun-Functions-AltInput", pCur->eAptActionType);
                void *pSavedValue = gAptActionInterpreter.PrepareForExecution(&oActionSetup);
                // push the passed in AptValue
                gAptActionInterpreter.stackPush(pCur->mAltInputValue ? pCur->mAltInputValue : gpUndefinedValue);
                gAptActionInterpreter.callFunction(pCur->function.pContext, pCur->function.pFuncDef, 1);
                gAptActionInterpreter.CleanupAfterExecution(pSavedValue, &oActionSetup);

                gAptActionInterpreter.thisStack.pop();
                gAptActionInterpreter.stack.Pop();
                TickNewInsts();
            }
#endif
            else
            {
#if !defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
                APT_ASSERT(0); // unknown action/function type
#endif
            }
            if (gAptActionInterpreter.stack.GetSize() > 0)
            {
                /// In the case of a pre-mature return we need to pop off the item here
                gAptActionInterpreter.stackPop();
                APT_ASSERT(gAptActionInterpreter.stack.GetSize() == 0);
            }
#if defined(APT_DEBUG)
            APT_ASSERT(gAptActionInterpreter.debugCallStack.GetSize() == 0);
#endif

            if (pCurrentPool->IsLastItemOrBeyond(pCur))
            {
                // Check pCur is within the action array boundaries, if not break.
                break;
            }

            // if the end of the queue is modified while we iterating through it, correct here
            // Crash in RunActions - commenting following code.
            // if (pOrigEnd > pCurrentPool->GetLastItem())
            //{
            // pCur -= pOrigEnd - pCurrentPool->GetLastItem();
            //}
#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
            pCurrentPool->ClearAction(pCur);
#endif
            pCur = pCurrentPool->GetNextItem(pCur);
#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
            if (nActionsExecuted >= s_nMaxActionsToExecute)
            {
                // just to print how many items we delayed this frame.
                int32_t nItemsExecuted = (pCur - pCurrentPool->GetFirstItem());
                printf("========Executed only %d action items out of %d\n", nItemsExecuted, nQueueSize);
                pCurrentPool->SetAsStart(pCur);
                break;
            }
#endif
        }
    }
#if defined APT_USE_BUTTONS
    APT_ASSERT(ValidateBIL());
#endif
    {
        TickNewInsts();
#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
        if (nTemp1 < s_nMaxActionsToExecute)
        {
            pCurrentPool->ResetPool();
        }
#else
        pCurrentPool->ClearActions();
#endif
        CleanRemList();
    }

}

#if defined(APT_ALTERNATE_INPUT)
void AptAnimationTarget::AddAlternateInput(AptAltInput *pInput)
{
    if (mnQueuedAltInputs >= miMaxQueuedInputs)
    {
        APT_ASSERTM((mnQueuedAltInputs < miMaxQueuedInputs), "Apt fixed-size buffer overflow; increase the corresponding AptInitParams size");

        return;
    }

    maQueuedAltInputs[mnQueuedAltInputs].sEvent = pInput->sEvent;
    maQueuedAltInputs[mnQueuedAltInputs].pValue = pInput->pValue;
    APT_INCSAFE(maQueuedAltInputs[mnQueuedAltInputs].pValue);
    mnQueuedAltInputs++;
}

void AptAnimationTarget::ProcessAlternateEvents()
{
    AptValue *pValue = NULL;
    int32_t nCount   = 0;

    for (int32_t i = 0; i < mnQueuedAltInputs; i++)
    {
        nCount = 0;
        for (int32_t j = 0; j < mAltListenerSet.capacity(); j++)
        {
            if (nCount == mAltListenerSet.mnElements)
            {
                break;
            }
            if (mAltListenerSet.maElements[j] == NULL)
            {
                continue;
            }
            pValue = mAltListenerSet.maElements[j];

            AptNativeHash *pHash = pValue->GetNativeHashVirtual();
            if (pHash != NULL)
            {
                // AptValue *pFunc = pHash->Lookup(&(maQueuedAltInputs[i].sEvent));
                AptValue *pFunc = pValue->findChild(&maQueuedAltInputs[i].sEvent, NULL);
                if (pFunc != NULL && pFunc != gpUndefinedValue)
                {
                    GetTargetSim()->GetAnimationTarget()->AddAltInputFunctionBack((AptCIH *)pValue, pFunc, 0,
                                                                                      ACTION_TYPE_CALL_PARAM(0x0)
                                                                                          maQueuedAltInputs[i]
                                                                                              .pValue);
                }
            }

            nCount++;
        }

        APT_DECSAFE(maQueuedAltInputs[i].pValue);
    }
}

void AptActionQueueC::AddAltInputFunctionBack(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptValue *pAltInputValue)
{
    APT_ASSERT(pContext->getIsDefined());

    AptActionPool *pTempItem = IncrementDequeLocation(mpEndDeque);
    if (pTempItem != mpStartDeque)
    {
        mpEndDeque->eActionType = AAT_ALT_INP_FUNCTION;
        // mpEndDeque->input = input;
        mpEndDeque->mAltInputValue = pAltInputValue;
        APT_INCSAFE(mpEndDeque->mAltInputValue);
        mpEndDeque->function.pContext = pContext;
        APT_INC(mpEndDeque->function.pContext);
        mpEndDeque->function.pFuncDef = pFuncDef;
        APT_INC(mpEndDeque->function.pFuncDef);
        mpEndDeque->function.nParams = nParams;
#if defined(APT_DEBUG)
        mpEndDeque->eAptActionType = (AptActionType)eAptActionType;
#endif
        mpEndDeque = pTempItem;
    }
    else
    {
        // the action pool is full; drop the action (increase AptInitParams::iActionPoolSize to avoid this)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "!!!!!!!!!!!!! AptAnimationPoolData:  Dequeue is full !!!!!!!!");
    }
}

#endif // #if defined(APT_ALTERNATE_INPUT)

void AptAnimationTarget::AddAnalogInput(AptAnalogStickInfo analogInput)
{
    bool bAdd = false;

    switch (analogInput.nSide)
    {
    case AptInputType_LeftAnalogStick:
        bAdd                                  = true;
        sgAStickLeft[analogInput.nController] = analogInput;
        break;
    case AptInputType_RightAnalogStick:
        bAdd                                   = true;
        sgAStickRight[analogInput.nController] = analogInput;
        break;
    case AptInputType_PadL:
        // After a single keydown message from a trigger, getAnalogTriggerInfo can be called to get the "intensity" data
        // Keeping bAdd false here makes this special case no invasive, avoiding additional messages in the queue.
        if (analogInput.fXAxisValue != 0.0f)
        {
            sgATriggers[analogInput.nController].fXAxisValue = analogInput.fXAxisValue;
            sgATriggers[analogInput.nController].fYAxisValue = 0.0f;
            if (AptGetLib()->mbSavedInputsEnabled)
            {
                // added this extra type of SaveInputRecord for trigger analog data. This is a special case because,
                // for trigger buttons we just have normal key up/key down messages, but now we also support analog values
                // for triggers on xenon. These analog values are
                // stored in static array called AptAnimationTarget::sgATriggers[AptInputController_NumControllers]
                // We save those values to saved input file and replay them. There will be a corresponding AddInput for keyup/keydown
                // along with these AddAnalogInput call. But it might be before this record or after this record, depending on how
                // user has passed to Apt originally.
                // The analog data is more related to keydown message and not to key up message.
                // If you call Key.getAnalogTriggerInfo() on keyup/keydown event, you will get same values.
                AptSavedInputRecordTriggers triggerInfo;
                triggerInfo.nTick  = AptGetLib()->mnCurTick;
                triggerInfo.nInput = SET_TRIGGER_INPUT();
#if defined(APT_SYSTEM_BIG_ENDIAN)
                triggerInfo.nInput = ((triggerInfo.nInput << 24) & 0xff000000);
#endif
                triggerInfo.mAnalogTriggerInfo = analogInput;
                AptGetUserFuncs().pfnDebugAddSavedInput(&triggerInfo, sizeof(AptSavedInputRecordTriggers));
            }
        }
        break;
    case AptInputType_PadR:
        sgATriggers[analogInput.nController].fYAxisValue = analogInput.fYAxisValue;
        sgATriggers[analogInput.nController].fXAxisValue = 0.0f;
        if (analogInput.fYAxisValue != 0.0f)
        {
            if (AptGetLib()->mbSavedInputsEnabled)
            {
                AptSavedInputRecordTriggers triggerInfo;
                triggerInfo.nTick  = AptGetLib()->mnCurTick;
                triggerInfo.nInput = SET_TRIGGER_INPUT();
#if defined(APT_SYSTEM_BIG_ENDIAN)
                triggerInfo.nInput = ((triggerInfo.nInput << 24) & 0xff000000);
#endif
                triggerInfo.mAnalogTriggerInfo = analogInput;
                AptGetUserFuncs().pfnDebugAddSavedInput(&triggerInfo, sizeof(AptSavedInputRecordTriggers));
            }
        }
        break;
    default:
        break;
    }

    if (bAdd)
    {
        uint32_t nInput = SET_ANALOG_INPUT(analogInput.nSide, AptInputState_Pressed, analogInput.nController);
        bool bAdded     = AddInput(nInput);

        if (AptGetLib()->mbSavedInputsEnabled && bAdded)
        {
            // we have already added nInput to SavedInputs and now we only want to add the AnalogData to savedinputs.
            AptGetUserFuncs().pfnDebugAddSavedInput((AptSavedInputRecord *)&analogInput, sizeof(AptAnalogStickInfo));
        }
    }
}

void AptAnimationTarget::AddGestureInput(AptGestureInfo gestureInput)
{
    maQueuedGestures[gestureInput.nController][gestureInput.nGesture - AptInputType_GestureStart] = gestureInput;

    uint32_t nInput = SET_GESTURE_INPUT(gestureInput.nGesture, AptInputState_Pressed, gestureInput.nController);

    AddInput(nInput);
}

bool AptAnimationTarget::AddInput(uint32_t nInput)
{

#if defined(APT_PRINT_INPUT)
    int32_t eType;
    AptInputController eController;
    AptInputState eState;

    if (INPUT_IS_KEY(&nInput))
    {
        GET_INPUT(nInput, eType, eState, eController);
        if (eController == AptInputController_Mouse)
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "%i Adding Mouse Input: eType[%i] eState[%i] eController[%i]\n", nInput, eType, eState, eController);
        else
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "%i Adding Keyboard Input: eType[%i] eState[%i] eController[%i]\n", nInput, eType, eState, eController);
    }
    else if (INPUT_IS_ANALOG(&nInput))
    {
        GET_INPUT(nInput, eType, eState, eController);
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "%i Adding Analog Input: eType[%i] eState[%i] eController[%i]\n", nInput, eType, eState, eController);
    }
    else if (INPUT_IS_GESTURE(&nInput))
    {
        GET_INPUT(nInput, eType, eState, eController);
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "%i Adding Gesture Input: eType[%i] eState[%i] eController[%i]\n", nInput, eType, eState, eController);
    }
    else if (INPUT_IS_MOUSE(&nInput))
    {
        float fX = 0.f, fY = 0.f;
        GET_MOUSE_INPUT(nInput, fX, fY);
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "%i Adding Mouse Input: mouse_x[%f] mouse_f[%f]\n", nInput, fX, fY);
    }
#endif
    if (mnQueuedInputs >= miMaxQueuedInputs)
    {
        APT_ASSERTM((mnQueuedInputs < miMaxQueuedInputs), "Apt fixed-size buffer overflow; increase the corresponding AptInitParams size");

        return false;
    }

#if defined(APT_USE_REPEATED_INPUT_OPTI)
    // Don't repeat the message, we end up doing thousands of repeated messages. (Release 17.00)
    // Doh! my bad. Was removing too much. The intent is basically to
    // prevent us from getting 3 billion of the same message, (cause then we will run the event handlers
    // 3 billion times). This should take care of the issue without cutting too much off.
    if (mnQueuedInputs > 0)
    {
        if (maQueuedInputs[mnQueuedInputs - 1] == nInput)
        {
            return false;
        }
    }
#endif

    maQueuedInputs[mnQueuedInputs] = nInput;
    mnQueuedInputs++;

    if (AptGetLib()->mbSavedInputsEnabled)
    {
        AptSavedInputRecordInput inputRecord;
        inputRecord.nTick = AptGetLib()->mnCurTick;
#if defined(APT_SYSTEM_BIG_ENDIAN)
        // Byte swap nInput
        volatile uint32_t temp = nInput;

        char *buf = (char *)&temp;

        for (int32_t i = 0; i < 4; i += 2)
        {
            buf[i] ^= buf[i + 1];
            buf[i + 1] ^= buf[i];
            buf[i] ^= buf[i + 1];
        }

        short *buf2 = (short *)buf;

        for (int32_t i = 0; i < 2; i += 2)
        {
            buf2[i] ^= buf2[i + 1];
            buf2[i + 1] ^= buf2[i];
            buf2[i] ^= buf2[i + 1];
        }
        nInput = temp;
#endif
        inputRecord.nInput = nInput;

        AptGetUserFuncs().pfnDebugAddSavedInput(&inputRecord, APT_SAVEINPUTS_RECORD_SIZE);
    }
    return true;
}

void AptAnimationTarget::AddInput(AptInputType eType, AptInputState eState, AptInputController eController)
{
    uint32_t nInput = 0;
#if defined(APT_USE_MOUSE)
    if (eController == AptInputController_Mouse)
    {
        nInput = SET_KEYBOARD_INPUT(eType, eState, eController);
    }
    else
#endif
    {
        nInput = SET_KEYBOARD_INPUT(eType, eState, eController);
    }
    AddInput(nInput);
}

#if defined(APT_USE_MOUSE)
void AptAnimationTarget::AddInput(int32_t x, int32_t y)
{
    // Skip this code if the mouse flag is false
    uint32_t nInput = SET_MOUSE_INPUT(x, y);
    AddInput(nInput);
}
#endif

AptGestureInfo *AptAnimationTarget::GetGestureInfo(int32_t Controller, AptInputType gestureID)
{
    if (gestureID >= AptInputType_GestureStart && gestureID <= AptInputType_GestureEnd)
    {
        return &maQueuedGestures[Controller][gestureID - AptInputType_GestureStart];
    }
    else
    {
        return NULL;
    }
}

void AptAnimationTarget::RemoveTimerFunctions(AptCIH *pAnimCIH)
{
    // go through the list and remove timers belonging to the movie clip instance
    for (int32_t i = 0; i < GetMaxIntervalTimers(); i++)
    {
        if (maIntervalTimers[i].bValid)
        {
            // if we're a valid script function belonging to the movie clip instance, remove us
            if ((pAnimCIH->GetCharacterInst() != NULL) &&
                ((maIntervalTimers[i].pCBFunction->isScriptFunction() &&
                  (maIntervalTimers[i].pCBFunction->c_scriptfunction()->mpCIH->GetCharacterInst() == NULL ||
                   (maIntervalTimers[i].pCBFunction->c_scriptfunction()->mpCIH->GetCharacterInst() == pAnimCIH->GetCharacterInst()))))) // removed mpFuncAnim
            {
                APT_DEC(maIntervalTimers[i].pCBFunction);
                APT_DECSAFE(maIntervalTimers[i].pContext);
                maIntervalTimers[i].CleanParams(); // set interval fix for supporting params.
                maIntervalTimers[i].bValid = false;
            }
        }
    }
}

#if defined APT_USE_BUTTONS
void AptAnimationTarget::ClearBIL()
{
    for (int32_t i = 0; i < snBILCount; i++)
    {
        APT_DEC(saButtonInstanceList[i].pCIH);
    }
    snBILCount = 0;
}

void AptAnimationTarget::AppendButtonToBIL(AptCIH *pBI, AptMatrix *pMatrix)
{
    if (snBILCount >= siButtonInstanceListSize)
    {
        // the button instance list is full; drop it (increase AptInitParams::iMaxButtonInstances to avoid this)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "AppendButtonToBIL: button instance list is full\n");
        return;
    }

    saButtonInstanceList[snBILCount].pCIH = pBI;
    APT_INC(pBI);
    saButtonInstanceList[snBILCount].matrix = *pMatrix;
    snBILCount++;
}

void AptAnimationTarget::RemoveFromBIL(AptCIH *pBI)
{
    for (int32_t i = 0; i <= snBILCount - 1; i++) // bug fixed in 0.7.11
    {
        if (pBI == saButtonInstanceList[i].pCIH)
        {
            APT_ASSERT(pBI->IsButtonInst());
            APT_DEC(pBI);
            memmove(&saButtonInstanceList[i], &saButtonInstanceList[i + 1], sizeof(saButtonInstanceList[i]) * (snBILCount - i));
            snBILCount--;
        }
    }
}
#endif

void AptAnimationTarget::SetInputMask(AptCIH *pMask)
{
    // TODO BUGBUG; this needs to be INCd and DECd appropriately
    mpInputMask = pMask;
}

bool AptAnimationTarget::IsInputMasked(AptCIH *pObj)
{
    if (mpInputMask == NULL)
    {
        return false;
    }

    AptCIH *pTemp = pObj;

    while (pTemp != NULL)
    {
        if (pTemp == mpInputMask)
        {
            return false;
        }
        else
        {
            pTemp = pTemp->GetDisplayListParent();
        }
    }
    return true;
}

void AptAnimationTarget::RegisterReferences()
{
#if defined APT_USE_BUTTONS
    APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mpFocusButton, "AptAnimationTarget::mpFocusButton", APT_REFREG_IS_APTCIH);
#endif
    APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mpInputMask, "AptAnimationTarget::mpInputMask", APT_REFREG_IS_APTCIH);

    for (int32_t i = 0; i < snNewInsts; i++)
    {
        if (sapNewInsts[i])
        {
            // Memory stomp due incorrect code in RegisterReferences(). - removed * in front of mapNewInsts[i]
            APT_REGISTER_REFERENCE_ANONYMOUS(sapNewInsts[i], "AptAnimationTarget::mapNewInsts", APT_REFREG_IS_APTCIH);
        }
    }

    int32_t nLoopCount = 0;
#if defined APT_USE_BUTTONS
    for (int32_t i = 0; i < snBILCount; i++)
    {
        APT_REGISTER_REFERENCE_ANONYMOUS(saButtonInstanceList[i].pCIH, "AptAnimationTarget::saButtonInstanceList[i].pCIH", APT_REFREG_IS_APTCIH);
    }

    // Warning, these loops go through a number of items that are null,
    // is there a more efficient way to do this loop?
    nLoopCount = mButtonSet.capacity();
    for (int32_t i = 0; i < nLoopCount; i++)
    {
        APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mButtonSet.maElements[i], "AptAnimationTarget::mButtonSet.aElements", APT_REFREG_IS_APTVALUE);
    }
#endif

    // Warning, these loops go through a number of items that are null,
    // is there a more efficient way to do this loop?
    nLoopCount = mListenerSet.capacity();
    for (int32_t i = 0; i < nLoopCount; i++)
    {
        APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mListenerSet.maElements[i], "AptAnimationTarget::mListenerSet.aElements", APT_REFREG_IS_APTVALUE);
    }

    // Warning, these loops go through a number of items that are null,
    // is there a more efficient way to do this loop?
#if defined(APT_USE_MOUSE)
    nLoopCount = mMouseListenerSet.capacity();
    for (int32_t i = 0; i < nLoopCount; i++)
    {
        APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mMouseListenerSet.maElements[i], "AptAnimationTarget::mMouseListenerSet.aElements", APT_REFREG_IS_APTVALUE);
    }
#endif

#if defined(APT_ALTERNATE_INPUT)
    nLoopCount = mAltListenerSet.capacity();
    for (int i = 0; i < nLoopCount; i++)
    {
        APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mAltListenerSet.maElements[i], "AptAnimationPoolData::mouseListenerSet.aElements", APT_REFREG_IS_APTVALUE);
    }
#endif

    // Warning, these loops go through a number of items that are null,
    // is there a more efficient way to do this loop?
    nLoopCount = mInputSet.capacity();
    for (int32_t i = 0; i < nLoopCount; i++)
    {
        APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mInputSet.maElements[i], "AptAnimationTarget::inputSet.aElements", APT_REFREG_IS_APTVALUE);
    }

    //  mDisplayList
    if (mDisplayList.pState)
    {
        mDisplayList.pState->RegisterReferences(NULL);
    }

    //  aActionPool
    mpAptActionPool->RegisterReferences();

    //  maIntervalTimers
    for (int32_t i = 0; i < GetMaxIntervalTimers(); ++i)
    {
        if (maIntervalTimers[i].bValid == false)
        {
            continue;
        }

        APT_REGISTER_REFERENCE_ANONYMOUS(maIntervalTimers[i].pCBFunction, "AptAnimationTarget::maIntervalTimers[i].pCBFunction", APT_REFREG_IS_APTVALUE);
        APT_REGISTER_REFERENCE_ANONYMOUS(maIntervalTimers[i].pContext, "AptAnimationTarget::maIntervalTimers[i].pContext", APT_REFREG_IS_APTVALUE);

        nLoopCount = maIntervalTimers[i].pParams.size();
        for (int32_t j = 0; j < nLoopCount; j++)
        {
            AptValue *pValue = maIntervalTimers[i].pParams.at(j);
            APT_REGISTER_REFERENCE_ANONYMOUS(pValue, "AptAnimationTarget::maIntervalTimers[i].pParams", APT_REFREG_IS_APTVALUE);
        }
    }
    for (int32_t i = 0; i < snDelayedReleaseCount; i++)
    {
        AptValue *pValue = sapDelayedReleaseList[i];
        if (pValue != NULL)
        {
            APT_REGISTER_REFERENCE_ANONYMOUS(pValue, "apDelayedReleaseList[i]", APT_REFREG_IS_APTVALUE);
        }
    }

    return;
}

void AptAnimationTarget::RemoveCIHReferences()
{
#if defined APT_USE_BUTTONS
    APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mpFocusButton, "AptAnimationTarget::mpFocusButton", APT_REFREG_IS_APTCIH);
#endif
    APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mpInputMask, "AptAnimationTarget::mpInputMask", APT_REFREG_IS_APTCIH);

    for (int32_t i = 0; i < snNewInsts; i++)
    {
        if (sapNewInsts[i] != NULL)
            APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(sapNewInsts[i], "mapNewInsts Element", APT_REFREG_IS_APTVALUE);
    }

    int32_t nLoopCount = 0;
#if defined APT_USE_BUTTONS
    for (int32_t i = 0; i < snBILCount; i++)
    {
        APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(saButtonInstanceList[i].pCIH, "saButtonInstanceList Element", APT_REFREG_IS_APTCIH);
    }

    // Warning, these loops go through a number of items that are null,
    // is there a more efficient way to do this loop?
    nLoopCount = mButtonSet.capacity();
    for (int32_t i = 0; i < nLoopCount; i++)
    {
        if (mButtonSet.maElements[i])
            APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mButtonSet.maElements[i], "mButtonSet Element", APT_REFREG_IS_APTVALUE);
    }
#endif

    // Warning, these loops go through a number of items that are null,
    // is there a more efficient way to do this loop?
    nLoopCount = mListenerSet.capacity();
    for (int32_t i = 0; i < nLoopCount; i++)
    {
        if (mListenerSet.maElements[i])
            APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mListenerSet.maElements[i], "mListenerSet Element", APT_REFREG_IS_APTVALUE);
    }

    // Warning, these loops go through a number of items that are null,
    // is there a more efficient way to do this loop?
#if defined(APT_USE_MOUSE)
    nLoopCount = mMouseListenerSet.capacity();
    for (int32_t i = 0; i < nLoopCount; i++)
    {
        if (mMouseListenerSet.maElements[i])
            APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mMouseListenerSet.maElements[i], "mMouseListenerSet Element", APT_REFREG_IS_APTVALUE);
    }
#endif

#if defined(APT_ALTERNATE_INPUT)
    nLoopCount = mAltListenerSet.capacity();
    for (int i = 0; i < nLoopCount; i++)
    {
        if (mAltListenerSet.maElements[i])
            APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mAltListenerSet.maElements[i], "mouseListenerSet Element", APT_REFREG_IS_APTVALUE);
    }
#endif

    // Warning, these loops go through a number of items that are null,
    // is there a more efficient way to do this loop?
    nLoopCount = mInputSet.capacity();
    for (int32_t i = 0; i < nLoopCount; i++)
    {
        if (mInputSet.maElements[i])
            APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(mInputSet.maElements[i], "inputSet Element", APT_REFREG_IS_APTVALUE);
    }

    //  maIntervalTimers
    for (int32_t i = 0; i < GetMaxIntervalTimers(); ++i)
    {
        if (maIntervalTimers[i].bValid == false)
        {
            continue;
        }

        maIntervalTimers[i].pCBFunction->RegisterReferences();
        maIntervalTimers[i].pContext->RegisterReferences();

        nLoopCount = maIntervalTimers[i].pParams.size();
        for (int32_t j = 0; j < nLoopCount; j++)
        {
            AptValue *pValue = maIntervalTimers[i].pParams.at(j);
            APT_REGISTER_REFERENCE_ANONYMOUS_SAFE(pValue, "IntervalTimerParam", APT_REFREG_IS_APTVALUE);
            if (pValue != maIntervalTimers[i].pParams.at(j))
            {
                APT_ASSERT(pValue == gpUndefinedValue);
                maIntervalTimers[i].pParams.SetAt(j, gpUndefinedValue);
            }
        }
    }
    return;
}

#if defined(APT_USE_BUTTONS)
void AptAnimationTarget::SetValidFocusButton()
{
    int32_t nX = 0, nY = 0;

    if (mpFocusButton == NULL || GetCharacterGridPosition(mpFocusButton->GetInstanceName(), &nX, &nY) == false)
    {
        // roll out current bad focus
        if (mpFocusButton)
        {
            mpFocusButton->gotoState(AptCharacterButtonRecordState_Up);
            DoButtonActions(mpFocusButton, AptActionConditionFlag_OverUpToIdle);
        }

        APT_DECSAFE(mpFocusButton);
        mpFocusButton = 0;

        // find a good focus button
        int32_t nCount = 0;
        for (int32_t i = 0; i < mButtonSet.capacity(); i++)
        {
            if (nCount == mButtonSet.mnElements)
                break;
            if (mButtonSet.maElements[i] == NULL)
                continue;

            if (mButtonSet.maElements[i]->GetInstanceName().IsEmpty() == false)
            {
                if (GetCharacterGridPosition(mButtonSet.maElements[i]->GetInstanceName(), &nX, &nY))
                {
                    mpFocusButton = mButtonSet.maElements[i];
                    APT_INC(mpFocusButton);
                    break;
                }
            }
            nCount++;
        }

        // roll into new focus
        if (mpFocusButton)
        {
            mpFocusButton->gotoState(AptCharacterButtonRecordState_Over);
            DoButtonActions(mpFocusButton, AptActionConditionFlag_IdleToOverUp);
        }
    }
}
#endif

#if defined(APT_USE_MOUSE)
void AptAnimationTarget::DragMovieClip()
{
    float fTmpX = snXMousePos - mDragPos.tx; // Update postion based on initial mouse click position
    float fTmpY = snYMousePos - mDragPos.ty; // Update postion based on initial mouse click position

    // Update MC position only if the new postion is within the boundary box given by the user.
    if (mDragPos.a != -9999.f && fTmpX < mDragPos.a)
    {
        fTmpX = mDragPos.a;
    }
    if (mDragPos.c != -9999.f && fTmpX > mDragPos.c)
    {
        fTmpX = mDragPos.c;
    }
    if (mDragPos.b != -9999.f && fTmpY < mDragPos.b)
    {
        fTmpY = mDragPos.b;
    }
    if (mDragPos.d != -9999.f && fTmpY > mDragPos.d)
    {
        fTmpY = mDragPos.d;
    }
    mpDragMC->c_cih()->SetProceduralProperty(AptProceduralProperty_X, fTmpX, true);
    mpDragMC->c_cih()->SetProceduralProperty(AptProceduralProperty_Y, fTmpY, true);
}
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/// AptIntervalTimer
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void AptIntervalTimer::CleanParams()
{
    int i;
    int nP = pParams.size();
    for (i = 0; i < nP; i++)
    {
        pParams.pop();
    }
}

int32_t AptIntervalTimer::GenerateId()
{
    static int32_t snId = 0;
    snId++;
    return snId;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/// AptActionQueueC
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void AptActionQueueC::ClearActions()
{
    AptActionPool *pCur = GetFirstItem();
    while (!IsLastItem(pCur))
    {
        if (pCur->eActionType == AAT_ACTION)
        {
            APT_DEC(pCur->action.pCIH);
        }
        else if (pCur->eActionType == AAT_FUNCTION)
        {
            APT_DEC(pCur->function.pContext);
            APT_DEC(pCur->function.pFuncDef);
        }
#if defined(APT_ALTERNATE_INPUT)
        else if (pCur->eActionType == AAT_ALT_INP_FUNCTION)
        {
            APT_DEC(pCur->function.pContext);
            APT_DEC(pCur->function.pFuncDef);
            APT_DECSAFE(pCur->mAltInputValue);
        }
#endif
        else
        {
            APT_ASSERT(0);
        }
        pCur->eActionType = AAT_NONE;
        pCur              = GetNextItem(pCur);
    }
    mpStartDeque = mpEndDeque = &maActionPool[0];
}

#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
void AptActionQueueC::ClearAction(AptActionPool *pCur)
{
    if (pCur->eActionType == AAT_ACTION)
    {
        APT_DEC(pCur->action.pCIH);
    }
    else if (pCur->eActionType == AAT_FUNCTION)
    {
        APT_DEC(pCur->function.pContext);
        APT_DEC(pCur->function.pFuncDef);
    }
#if defined(APT_ALTERNATE_INPUT)
    else if (pCur->eActionType == AAT_ALT_INP_FUNCTION)
    {
        APT_DEC(pCur->function.pContext);
        APT_DEC(pCur->function.pFuncDef);
        APT_DECSAFE(pCur->mAltInputValue);
    }
#endif
    else
    {
        // APT_ASSERT(0);
    }
    pCur->eActionType = AAT_NONE;
}

void AptActionQueueC::SetAsStart(AptActionPool *pCur)
{
    mpStartDeque = pCur;
}
#endif
void AptActionQueueC::AddActionBack(AptActionBlock *pActionBlock, AptCIH *pCIH, ACTION_TYPE_DECL_PARAM AptInput input)
{
    APT_ASSERT(pCIH->getIsDefined());

    AptActionPool *pTempItem = IncrementDequeLocation(mpEndDeque);
    if (pTempItem != mpStartDeque)
    {
        mpEndDeque->eActionType   = AAT_ACTION;
        mpEndDeque->action.nFrame = pCIH->GetSpriteInstBase()->mnGotoAnded; // Set the frame this action was execute on to the queued obj
        mpEndDeque->action.pBlock = pActionBlock;
        mpEndDeque->action.pCIH   = pCIH;
        APT_INC(pCIH);
        mpEndDeque->input = input;
#if defined(APT_DEBUG)
        mpEndDeque->eAptActionType = (AptActionType)eAptActionType;
#endif
        mpEndDeque = pTempItem;
    }
    else
    {
        // the action pool is full; drop the action (increase AptInitParams::iActionPoolSize to avoid this)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "!!!!!!!!!!!!! AptAnimationPoolData:  Dequeue is full !!!!!!!!");
    }
}

void AptActionQueueC::AddActionFront(AptActionBlock *pActionBlock, AptCIH *pCIH, ACTION_TYPE_DECL_PARAM AptInput input)
{
    APT_ASSERT(pCIH->getIsDefined());

    AptActionPool *pTempItem = DecrementDequeLocation(mpStartDeque);
    if (pTempItem != mpEndDeque)
    {
        mpStartDeque                = pTempItem;
        mpStartDeque->action.nFrame = pCIH->GetSpriteInstBase()->mnGotoAnded; // Set the frame this action was execute on to the queued obj
        mpStartDeque->eActionType   = AAT_ACTION;
        mpStartDeque->action.pBlock = pActionBlock;
        mpStartDeque->action.pCIH   = pCIH;
        APT_INC(pCIH);
        mpStartDeque->input = input;
#if defined(APT_DEBUG)
        mpStartDeque->eAptActionType = (AptActionType)eAptActionType;
#endif
    }
    else
    {
        // the action pool is full; drop the action (increase AptInitParams::iActionPoolSize to avoid this)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "!!!!!!!!!!!!! AptAnimationPoolData:  Dequeue is full !!!!!!!!");
    }
}

void AptActionQueueC::AddFunctionBack(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptInput input)
{
    APT_ASSERT(pContext->getIsDefined());

    AptActionPool *pTempItem = IncrementDequeLocation(mpEndDeque);
    if (pTempItem != mpStartDeque)
    {
        mpEndDeque->eActionType       = AAT_FUNCTION;
        mpEndDeque->input             = input;
        mpEndDeque->function.pContext = pContext;
        APT_INC(mpEndDeque->function.pContext);
        mpEndDeque->function.pFuncDef = pFuncDef;
        APT_INC(mpEndDeque->function.pFuncDef);
        mpEndDeque->function.nParams = nParams;
#if defined(APT_DEBUG)
        mpEndDeque->eAptActionType = (AptActionType)eAptActionType;
#endif
        mpEndDeque = pTempItem;
    }
    else
    {
        // the action pool is full; drop the action (increase AptInitParams::iActionPoolSize to avoid this)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "!!!!!!!!!!!!! AptAnimationPoolData:  Dequeue is full !!!!!!!!");
    }
}

void AptActionQueueC::AddFunctionFront(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptInput input)
{
    APT_ASSERT(pContext->getIsDefined());

    AptActionPool *pTempItem = DecrementDequeLocation(mpStartDeque);
    if (pTempItem != mpEndDeque)
    {
        mpStartDeque                    = pTempItem;
        mpStartDeque->eActionType       = AAT_FUNCTION;
        mpStartDeque->input             = input;
        mpStartDeque->function.pContext = pContext;
        APT_INC(mpStartDeque->function.pContext);
        mpStartDeque->function.pFuncDef = pFuncDef;
        APT_INC(mpStartDeque->function.pFuncDef);
        mpStartDeque->function.nParams = nParams;
#if defined(APT_DEBUG)
        mpStartDeque->eAptActionType = (AptActionType)eAptActionType;
#endif
    }
    else
    {
        // the action pool is full; drop the action (increase AptInitParams::iActionPoolSize to avoid this)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "!!!!!!!!!!!!! AptAnimationPoolData:  Dequeue is full !!!!!!!!");
    }
}

void AptActionQueueC::RemoveActionFor(AptCIH *pCIH)
{
    AptActionPool *pCur = GetFirstItem();
    while (!IsLastItem(pCur))
    {
        if (pCur->eActionType == AAT_ACTION)
        {
            if (pCur->action.pCIH == pCIH)
            {
                if (pCur == GetCurItem()) // if pCur is the CIH that is in runActions, don't remove it. It will remove itself at the end of runActions.
                {
                    pCur = GetNextItem(pCur);
                    continue;
                }
                if (pCur < mpEndDeque)
                {
                    APT_DEC(pCIH);
                    memmove(pCur, pCur + 1, (mpEndDeque - pCur - 1) * sizeof(AptActionPool));
                    mpEndDeque = DecrementDequeLocation(mpEndDeque);
                    break;
                }
                else if (pCur > mpStartDeque)
                {
                    APT_DEC(pCIH);
                    memmove(mpStartDeque + 1, mpStartDeque, (pCur - mpStartDeque) * sizeof(AptActionPool));
                    mpStartDeque = IncrementDequeLocation(mpStartDeque);
                    break;
                }
                else if (pCur == mpStartDeque)
                {
                    mpStartDeque = IncrementDequeLocation(mpStartDeque);
                    break;
                }
                else
                {
                    APT_ASSERT(NOT_REACHED);
                }
            }
        }
        pCur = GetNextItem(pCur);
    }
}

int32_t AptActionQueueC::GetDequeSize() const
{
    int32_t iDelta = mpEndDeque - mpStartDeque;
    if (iDelta >= 0)
    {
        return (iDelta);
    }

    //  Add the roll-over size
    iDelta = miActionPoolSize + iDelta;
    APT_ASSERT(iDelta > 0); //  iDelta should be now positive
    return (iDelta);
}

AptActionQueueC::AptActionPool *AptActionQueueC::GetDequeLocation(const int32_t iIndex) const
{
    int32_t iOffset = mpStartDeque - maActionPool;
    APT_ASSERT((iOffset >= 0) && (iOffset < miActionPoolSize));
    iOffset += iIndex;
    iOffset %= miActionPoolSize;
    if (iOffset >= 0)
    {
        return (&maActionPool[iOffset]);
    }
    iOffset += miActionPoolSize;
    return (&maActionPool[iOffset]);
}

void AptActionQueueC::RegisterReferences()
{
    int nLoopCount = GetDequeSize();
    for (int i = 0; i < nLoopCount; i++)
    {
        AptActionPool *pActionPool;
        pActionPool = GetDequeLocation(i);
        if (pActionPool->eActionType == AAT_ACTION)
        {
            APT_REGISTER_REFERENCE_ANONYMOUS(pActionPool->action.pCIH, "AptAnimationPoolData::action.pCIH", APT_REFREG_IS_APTCIH);
        }
        else if (pActionPool->eActionType == AAT_FUNCTION)
        {
            APT_REGISTER_REFERENCE_ANONYMOUS(pActionPool->function.pContext, "AptAnimationPoolData::function.pContext", APT_REFREG_IS_APTCIH);
            APT_REGISTER_REFERENCE_ANONYMOUS(pActionPool->function.pFuncDef, "AptAnimationPoolData::function.pFuncDef", APT_REFREG_IS_APTVALUE);
        }
#if defined(APT_ALTERNATE_INPUT)
        else if (pActionPool->eActionType == AAT_ALT_INP_FUNCTION)
        {
            APT_REGISTER_REFERENCE_ANONYMOUS(pActionPool->function.pContext, "AptAnimationPoolData::function.pContext", APT_REFREG_IS_APTCIH);
            APT_REGISTER_REFERENCE_ANONYMOUS(pActionPool->function.pFuncDef, "AptAnimationPoolData::function.pFuncDef", APT_REFREG_IS_APTVALUE);
        }
#endif
        else
        {
            APT_ASSERT(NOT_REACHED && "Encountered invalid pActionPool->eActionType");
        }
    }
}
