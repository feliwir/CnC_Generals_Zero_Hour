#pragma once

/*** Include files ********************************************************************************/
#include "string/StringPool.h"
#include "AptAllocator.h"
#include "AptDefine.h"
#include "AptStd/AptMatrix.h"
#include "AptValue.h"

#include "_AptValue.h"
#include "_AptSet.h"
#include "_AptValuePtrStack.h"
#include "_AptActions.h"
#include "Display/AptDisplayList.h"
#include <vector>

/*** Defines **************************************************************************************/
// ASK added extra eActionType to all addAction/Function functions - 0.18.00
// note the way macros are used. and there is no comma in between ACTION_TYPE_PARAM AptInput input
// if profiling is enabled, pass an extra param to the addAction/Function functions

// if in debug, pass an extra param to the addAction/Function functions
// made the action type as int even if there is AptActionType defined so that we can use the same
// AptEventActionFlag enum to pass it in as it has same enum values.
#if defined(APT_DEBUG)
#define ACTION_TYPE_DECL_PARAM int eAptActionType,
#define ACTION_TYPE_CALL_PARAM(_eType) (_eType),
#else
#define ACTION_TYPE_DECL_PARAM
#define ACTION_TYPE_CALL_PARAM(_eType)
#endif
/*** Macros ***************************************************************************************/
// #define APT_SPREAD_ACTIONS_MULTI_FRAMES     1
/*** Type Definitions *****************************************************************************/
class AptCIH;
// class AptValue;
struct AptCharacterButton;
struct AptActionBlock;

using AptAnimationCihList = std::vector<AptCIH *>;

#if defined APT_USE_BUTTONS
using AptAnimationButtonSet = AptValueSet<AptCIH *>;
#endif
using AptAnimationInputSet    = AptValueSet<AptCIH *>;
using AptAnimationListenerSet = AptValueSet<AptValue *>; // keeps track of listener objects.

struct ButtonHitTestRecord
{
#if defined APT_USE_BUTTONS
    APT_NEW_DELETE_OPERATORS

    AptCIH *pCIH;
    AptMatrix matrix;
#endif
};

/** @brief This struct is used by the AptAnimationTarget to maintain flash intervals */
struct AptIntervalTimer
{
    APT_NEW_DELETE_OPERATORS

    int32_t bValid{false};
    AptValue *pCBFunction;
    float fInterval{0.0f};
    float fCurTime{0.0f};
    AptValue *pContext;
    AptValuePtrStack<AptValue> pParams;


    int32_t nIntervalId{0};
    static const int32_t MAX_INTERVAL_FUNC_PARAMS = 6;

    AptIntervalTimer()
        : pCBFunction(NULL),

          pContext(NULL),
          pParams(MAX_INTERVAL_FUNC_PARAMS)

    {
    }

    /** @brief Clears the function params for the intervals */
    void CleanParams();

    /** @brief Destructor. */
    ~AptIntervalTimer() { CleanParams(); }

    /** @brief Generates a unique interval Id */
    static int32_t GenerateId();
};

class AptActionQueueC
{
  public:
    APT_NEW_DELETE_OPERATORS

    ///! Type Definitions Begin
    enum APT_ACTION_TYPE
    {
        AAT_NONE,
        AAT_ACTION,
        AAT_FUNCTION
#if defined(APT_ALTERNATE_INPUT)
        ,
        AAT_ALT_INP_FUNCTION // added for special handling of alternate input
#endif
    };

    struct AptAction
    {
        int32_t nFrame; // Used to keep track of the frame this AptActionBlock should execute from
        AptActionBlock *pBlock;
        AptCIH *pCIH;
    };
    struct AptFunction
    {
        AptCIH *pContext;
        AptValue *pFuncDef;
        int32_t nParams;
    };
    struct AptActionPool
    {
        APT_NEW_DELETE_OPERATORS

        APT_ACTION_TYPE eActionType;
#if defined(APT_DEBUG)
        AptActionType eAptActionType = AptActionType_Invalid;
#endif
#if defined(APT_ALTERNATE_INPUT)
        // added for special handling of alternate input
        union
        {
            AptInput input;
            AptValue *mAltInputValue;
        };
#else
        AptInput input;
#endif
        union
        {
            AptAction action;
            AptFunction function;
        };
    };
    ///! Type Definitions End

    /** @brief Constructor. */
    AptActionQueueC(uint32_t nSize)
    {
        APT_ASSERT(nSize != 0);
        miActionPoolSize = nSize;
        maActionPool     = new AptActionPool[miActionPoolSize];
        mpCurDeque = mpStartDeque = mpEndDeque = &maActionPool[0];
        ClearActions();
    }
    /** @brief Destructor. */
    ~AptActionQueueC()
    {
        delete[] maActionPool;
    }

    /** @brief Functions to Add and remove actions */
    void AddActionBack(AptActionBlock *pActionBlock, AptCIH *pCIH, ACTION_TYPE_DECL_PARAM AptInput input = gNullInput);
    void AddActionFront(AptActionBlock *pActionBlock, AptCIH *pCIH, ACTION_TYPE_DECL_PARAM AptInput input = gNullInput);
    void AddFunctionBack(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptInput input);
    void AddFunctionFront(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptInput input);
#if defined(APT_ALTERNATE_INPUT)
    void AddAltInputFunctionBack(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptValue *pAltInputVal);
#endif
    void RemoveActionFor(AptCIH *pCIH);

    /** @brief Clears all actions */
    void ClearActions();
#if defined(APT_SPREAD_ACTIONS_MULTI_FRAMES)
    void ClearAction(AptActionPool *pCur);
    void SetAsStart(AptActionPool *pCur);
    APT_INLINE void ResetPool()
    {
        mpStartDeque = mpEndDeque = &maActionPool[0];
    }
#endif
    /** @brief Garbage collection callback */
    void RegisterReferences();

    /** @brief Returns first item in the action pool */
    APT_INLINE AptActionPool *GetFirstItem()
    {
#if defined(APT_DEBUG)
        CheckDequeueBounds(mpStartDeque);
#endif
        return mpStartDeque;
    }

    /** @brief Gets next action */
    APT_INLINE AptActionPool *GetNextItem(AptActionPool *pItem)
    {
        return IncrementDequeLocation(pItem);
    }

    /** @brief Checks if pItem is the last action  */
    APT_INLINE bool IsLastItem(AptActionPool *pItem)
    {
        return (pItem == mpEndDeque);
    }

    /** @brief Determines if pItem is the "last item" an item outside the used part of the array */
    APT_INLINE bool IsLastItemOrBeyond(AptActionPool *pItem)
    {
        if (mpStartDeque <= mpEndDeque)
        {
            return (pItem >= mpEndDeque);
        }
        else
        {
            return ((pItem >= mpEndDeque) && (pItem < mpStartDeque));
        }
    }

    /** @brief Returns the last item */
    APT_INLINE AptActionPool *GetLastItem()
    {
        return mpEndDeque;
    }

    /** @brief Returns the current action */
    APT_INLINE AptActionPool *GetCurItem()
    {
        return mpCurDeque;
    }

    /** @brief Sets the current action */
    APT_INLINE void SetCurItem(AptActionPool *pItem)
    {
        mpCurDeque = pItem;
    }

#if defined(APT_DEBUG)
    /** @brief Debug function to check action pool boundaries */
    APT_INLINE void CheckDequeueBounds(AptActionPool *pCur)
    {
        APT_ASSERT(pCur >= &maActionPool[0]);
        APT_ASSERT(pCur < &maActionPool[miActionPoolSize]);
    }
#endif

    /** @brief Returns action pool size */
    int32_t GetDequeSize() const;

  private:
    /** @brief Advances the 'curr' pointer by one and properly wraps it to the dequeue buffer */
    APT_INLINE AptActionPool *IncrementDequeLocation(AptActionPool *curr)
    {
        AptActionPool *ret = curr + 1;

        if (ret == &maActionPool[miActionPoolSize])
        {
            ret = &maActionPool[0];
        }
#if defined(APT_DEBUG)
        CheckDequeueBounds(ret);
#endif
        return ret;
    }

    /** @brief Devances the 'curr' pointer by one and properly wraps it to the dequeue buffer */
    APT_INLINE AptActionPool *DecrementDequeLocation(AptActionPool *curr)
    {
        AptActionPool *ret = curr - 1;

        if (ret < &maActionPool[0])
        {
            ret = &maActionPool[miActionPoolSize - 1];
        }
#if defined(APT_DEBUG)
        CheckDequeueBounds(ret);
#endif
        return ret;
    }

    /** @brief Returns the index of the current action */
    AptActionPool *GetDequeLocation(const int32_t iIndex) const;

    AptActionPool *maActionPool;
    AptActionPool *mpStartDeque;
    AptActionPool *mpEndDeque;
    AptActionPool *mpCurDeque; // mpTmpEndDeque wasn't being used, renamed to pCurDeque to store current pointer for remove check.
    int32_t miActionPoolSize;
};

/** @brief This class maintains the execution of flash */
class AptAnimationTarget
{
  public:
    APT_NEW_DELETE_OPERATORS

    AptAnimationTarget(const AptTargetInitParams *aptInitParms);
    ~AptAnimationTarget();

    void RemoveTimerFunctions(AptCIH *pAnimCIH);

    void AddInput(AptInputType eInput, AptInputState eState, AptInputController eController);
    void AddAnalogInput(AptAnalogStickInfo nInput); // analog stick support
    void AddGestureInput(AptGestureInfo nInput);    // gesture support
    bool AddInput(uint32_t nInput);

    void TickIntervalTimers(int32_t nMilliseconds);
    void RunActions(void);
    void TickNewInsts(void);

    void ProcessAnalogInputs(void); // analog stick support
    void ProcessInputs(void);

    void SetInputMask(AptCIH *pMask);
    bool IsInputMasked(AptCIH *pCIH);

    APT_INLINE int32_t GetMaxNewMovieClips(void) { return siMaxNewMovieClips; }
    APT_INLINE int32_t GetMaxIntervalTimers(void) { return miMaxIntervalTimers; }

    void PreDestroy(void); // This is not virtual because AptNativeHash is not designed to be overridden.
    void Reset();
    void RegisterReferences();  // This is not virtual because AptNativeHash is not designed to be overridden.
    void RemoveCIHReferences(); // this is for the ReplaceReferencesCb so that all global type references get registered
    void CleanRemList();
    void AddToRemList(AptCIH *pItem);

    APT_INLINE AptActionQueueC *GetActionPool()
    {
        return mpAptActionPool;
    }

    APT_INLINE void AddActionBack(AptActionBlock *pActionBlock, AptCIH *pCIH, ACTION_TYPE_DECL_PARAM AptInput input = gNullInput)
    {
        GetActionPool()->AddActionBack(pActionBlock, pCIH,
                                           ACTION_TYPE_CALL_PARAM(eAptActionType)
                                               input);
    }
    APT_INLINE void AddActionFront(AptActionBlock *pActionBlock, AptCIH *pCIH, ACTION_TYPE_DECL_PARAM AptInput input = gNullInput)
    {
        GetActionPool()->AddActionFront(pActionBlock, pCIH,
                                            ACTION_TYPE_CALL_PARAM(eAptActionType)
                                                input);
    }
    APT_INLINE void AddFunctionBack(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptInput input)
    {
        GetActionPool()->AddFunctionBack(pContext, pFuncDef, nParams,
                                             ACTION_TYPE_CALL_PARAM(eAptActionType)
                                                 input);
    }
    APT_INLINE void AddFunctionFront(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptInput input)
    {
        GetActionPool()->AddFunctionFront(pContext, pFuncDef, nParams,
                                              ACTION_TYPE_CALL_PARAM(eAptActionType)
                                                  input);
    }
#if defined(APT_ALTERNATE_INPUT)
    APT_INLINE void AddAltInputFunctionBack(AptCIH *pContext, AptValue *pFuncDef, int nParams, ACTION_TYPE_DECL_PARAM AptValue *pAltInputValue)
    {
        GetActionPool()->AddAltInputFunctionBack(pContext, pFuncDef, nParams,
                                                     ACTION_TYPE_CALL_PARAM(eAptActionType)
                                                         pAltInputValue);
    }
#endif
    APT_INLINE void DragMovieClip();

// Public button related functions
#if defined APT_USE_BUTTONS
    void ClearBIL();
    void AppendButtonToBIL(AptCIH *pBI, AptMatrix *pMatrix);
    void RemoveFromBIL(AptCIH *pBI);
    bool ValidateBIL();
    void SetValidFocusButton();
#endif

// Public mouse related functions
#if defined APT_USE_MOUSE
    void AddInput(int32_t x, int32_t y);
#endif

    //! Getters and Setters
    APT_INLINE AptAnimationCihList &GetNewInsts()
    {
        return sapNewInsts;
    }
    APT_INLINE int32_t GetNewInstSize()
    {
        return snNewInsts;
    }
    APT_INLINE int32_t IncNewInstSize()
    {
        // Grow the new instances list if we're at capacity
        if (sapNewInsts.capacity() == (size_t)snNewInsts)
        {
            sapNewInsts.resize(sapNewInsts.capacity() + siNewMovieClipsGrowSize);
        }

        return snNewInsts++;
    }
    APT_INLINE int32_t DecNewInstSize()
    {
        return snNewInsts++;
    }
    static APT_INLINE AptAnimationCihList &GetDelayedReleaseList()
    {
        return sapDelayedReleaseList;
    }
    static APT_INLINE int32_t GetDelayedReleaseListSize()
    {
        return snDelayedReleaseCount;
    }
    static APT_INLINE int32_t GetDelayedReleaseListSizeHW()
    {
        return snDelayedReleaseCountHW;
    }
    APT_INLINE AptAnimationListenerSet *GetListenerSet()
    {
        return &mListenerSet;
    }
    APT_INLINE AptAnimationInputSet *GetInputSet()
    {
        return &mInputSet;
    }
    APT_INLINE AptDisplayList *GetDisplayList()
    {
        return &mDisplayList;
    }
    APT_INLINE AptIntervalTimer *GetIntervalTimers()
    {
        return maIntervalTimers;
    }
    APT_INLINE void SetQueuedInputsSize(int32_t nSize)
    {
        mnQueuedInputs = nSize;
    }
#if defined(APT_ALTERNATE_INPUT)
    APT_INLINE AptAnimationListenerSet *GetAlternateInputSet()
    {
        return &mAltListenerSet;
    }
    APT_INLINE void SetAltQueuedInputsSize(int32_t nSize)
    {
        mnQueuedAltInputs = nSize;
    }
    APT_INLINE int32_t GetAltQueuedInputsSize()
    {
        return mnQueuedAltInputs;
    }
#endif
    APT_INLINE int32_t GetQueuedInputsSize()
    {
        return mnQueuedInputs;
    }
    APT_INLINE void SetDragMC(AptValue *pValue)
    {
        mpDragMC = pValue;
    }
    APT_INLINE AptValue *GetDragMC()
    {
        return mpDragMC;
    }
    APT_INLINE AptMatrix *GetDragPos()
    {
        return &mDragPos;
    }

    APT_INLINE AptValue *GetOnPressObject()
    {
        return mpOnPress;
    }
    APT_INLINE void SetOnPressObject(AptValue *pValue)
    {
        mpOnPress = pValue;
    }
    APT_INLINE AptValue *GetOnRollOverObject()
    {
        return mpOnRollOver;
    }
    APT_INLINE void SetOnRollOverObject(AptValue *pValue)
    {
        mpOnRollOver = pValue;
    }
    APT_INLINE AptValue *GetTopMostSprite()
    {
        return mpTopMostSprite;
    }
    APT_INLINE AptAnalogStickInfo *GetAStickLeft(int32_t Controller)
    {
        return &sgAStickLeft[Controller];
    }
    APT_INLINE AptAnalogStickInfo *GetAStickRight(int32_t Controller)
    {
        return &sgAStickRight[Controller];
    }
    APT_INLINE AptAnalogStickInfo *GetATriggers(int32_t Controller) // analog triggers data
    {
        return &sgATriggers[Controller];
    }
    APT_INLINE int32_t GetXMousePos()
    {
        return snXMousePos;
    }
    APT_INLINE int32_t GetYMousePos()
    {
        return snYMousePos;
    }

#if defined APT_USE_BUTTONS
    APT_INLINE void SetFocusButton(AptCIH *pCIH)
    {
        mpFocusButton = pCIH;
    }
    APT_INLINE AptCIH *GetFocusButton()
    {
        return mpFocusButton;
    }
    APT_INLINE AptAnimationButtonSet *GetButtonSet()
    {
        return &mButtonSet;
    }
#endif

#if defined APT_USE_MOUSE
    APT_INLINE AptAnimationListenerSet *GetMouseListenerSet()
    {
        return &mMouseListenerSet;
    }
#endif

    static void SetupStaticData(const AptInitParams &aptInitParms);
    static void CleanupStaticData();

#if defined(APT_ALTERNATE_INPUT)
    void AddAlternateInput(AptAltInput *pInput);
#endif

    AptGestureInfo *GetGestureInfo(int32_t Controller, AptInputType gestureID);

  private:
    // don't copy
    AptAnimationTarget(const AptAnimationTarget &);
    AptAnimationTarget &operator=(const AptAnimationTarget &);

    void AddListenerToQueue(AptValue *pValue, int32_t nEventFlags, AptInput input);
    void ProcessListenerEvents(AptInputType eType, AptInputState eState, AptInput input, AptInputController eController);
    void ProcessInputSet(AptInputType eType, AptInputState eState, AptInput input, AptInputController eController, bool bCheckTop);
    void ProcessAptInput(AptInput input, bool bCheckTop);

// Private button related functions
#if defined APT_USE_BUTTONS
    int32_t ActionConditionFlagToActionEventFlags(int32_t condition);
    void ProcessButtonState();
    bool HandleFocusButton(AptInputType eType, AptInputState eState, AptCIH **ppNewButton);
    void HandleAutoNav(AptCIH *pNewButton, AptInputType eType, AptInputState eState);
    void DoButtonActions(AptCIH *pInst, int nTransition);
    bool IsPointInButtonHitTestRegion(const AptCharacterButton *button, AptMatrix *pMatrix, int32_t nX, int32_t nY);
    AptCIH *GetButton(int nX, int nY);
#endif

// Private mouse related functions
#if defined APT_USE_MOUSE
    void ProcessMouseListenerEvents(AptInputType eType, AptInputState eState, AptInput input);
    void ProcessMouseState(bool bPress, bool bPressRelease, AptInput input);
#endif

#if defined(APT_ALTERNATE_INPUT)
    void ProcessAlternateEvents();
#endif

    AptCIH *mpInputMask;
    int32_t miMaxIntervalTimers;
    int32_t miMaxQueuedInputs;

    AptActionQueueC *mpAptActionPool;
    AptAnimationListenerSet mListenerSet;
    AptAnimationInputSet mInputSet;
    AptDisplayList mDisplayList;
    AptIntervalTimer *maIntervalTimers;

    int32_t mnQueuedInputs;   // queued inputs
    AptInput *maQueuedInputs; // these are now allocated at runtime in constructor

    AptValue *mpOnPress;       // Used to keep track of the currently pressed movieclip
    AptValue *mpOnRollOver;    // Used to keep track of the currently rolled on movieclip
    AptValue *mpTopMostSprite; // This is used to keep track of the highest sprite.        // #F563    3

    // These two AptAnalogStickInfo vars replace the old queue system.  We only care about the most recent analog positions.
    static AptAnalogStickInfo sgAStickLeft[AptInputController_NumControllers];
    static AptAnalogStickInfo sgAStickRight[AptInputController_NumControllers];
    // Information about the last positions of the analog triggers
    static AptAnalogStickInfo sgATriggers[AptInputController_NumControllers];

    AptGestureInfo maQueuedGestures[AptInputController_NumControllers][(AptInputType_GestureEnd - AptInputType_GestureStart) + 1]; // these are allocated at runtime in constructor

    static int32_t snXMousePos;
    static int32_t snYMousePos;

    AptValue *mpDragMC;
    AptMatrix mDragPos;

// Button related members
#if defined APT_USE_BUTTONS
    AptCIH *mpFocusButton;
    bool mbButtonDown;
    AptAnimationButtonSet mButtonSet;
#endif

// Mouse related members
#if defined APT_USE_MOUSE
    AptAnimationListenerSet mMouseListenerSet;
#endif

#if defined(APT_ALTERNATE_INPUT)
    AptAnimationListenerSet mAltListenerSet;
    int32_t mnQueuedAltInputs;
    AptAltInput *maQueuedAltInputs;
#endif

#if defined APT_USE_BUTTONS
    static int32_t snBILCount;                        // count of the used elements in buttonInstanceList
    static ButtonHitTestRecord *saButtonInstanceList; // content of the array will be clean and refilled every frame, these are now allocated at runtime in constructor
    static int32_t siButtonInstanceListSize;
#endif

    static int32_t siMaxNewMovieClips;
    static int32_t siNewMovieClipsGrowSize; // Custom growth size for the following vectors when at full capcity.
    static AptAnimationCihList sapNewInsts; // Store newly constructed CIH's to process at the end of the current frame.
    static int32_t snNewInsts;
    static AptAnimationCihList sapDelayedReleaseList; // Store CIH's scheduled for destruction at the end of the current frame.
    static int32_t snDelayedReleaseCount;
    static int32_t snDelayedReleaseCountHW;
};
/*** Functions ************************************************************************************/

