#pragma once

/*** Include files ********************************************************************************/
#include "_Apt.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

#define APT_SETUP_SIM_TARGET_STATES(_)              \
    AptTarget *pPrevTargetSim = AptGetLib()->mpCurrentTargetSim; \
    AptGetLib()->mpCurrentTargetSim        = (AptTarget *)_;     \
    AptGetLib()->mpCurrentTargetRender     = (AptTarget *)_;

#define APT_TEARDOWN_SIM_TARGET_STATES      \
    AptGetLib()->mpCurrentTargetSim    = pPrevTargetSim; \
    AptGetLib()->mpCurrentTargetRender = pPrevTargetSim;

#define APT_SETUP_RENDER_TARGET_STATES(_)                 \
    AptTarget *pPrevTargetRender = AptGetLib()->mpCurrentTargetRender; \
    AptGetLib()->mpCurrentTargetRender        = (AptTarget *)_;        \
    AptGetLib()->mpCurrentTargetSim           = (AptTarget *)_;

#define APT_TEARDOWN_RENDER_TARGET_STATES      \
    AptGetLib()->mpCurrentTargetRender = pPrevTargetRender; \
    AptGetLib()->mpCurrentTargetSim    = pPrevTargetRender;

/*** Type Definitions *****************************************************************************/
/**
 * @brief A renderable/simulatable instance: an animation pool, a loader, a linker and
 * a render tree manager. Several may exist at once (see AptCreateTargetInstance);
 * which one is current is the ambient selection above.
 *
 * Not to be confused with AptLibrary, which owns the library-wide state a
 * target lives inside. Every target carries a back-pointer to its library.
 */
class AptTarget
{
  public:
    APT_NEW_DELETE_OPERATORS

    /** @brief Constructor. */
    AptTarget(const AptInitParams *aptInitParms);

    /** @brief Constructor. */
    AptTarget(const AptTargetInitParams *aptInitParms);

    /** @brief Destructor. */
    ~AptTarget();

    /** @brief Shuts down and deletes all internal members. */
    void Shutdown();

    /** @brief Resets all internal states. */
    void Reset();

    /** @brief Returns the AnimationTarget. */
    APT_INLINE AptAnimationTarget *GetAnimationTarget()
    {
        return mpPool;
    }

    /** @brief Returns the AptLoader for this target. */
    APT_INLINE AptLoader *GetLoader()
    {
        return mpLoader;
    }

    /** @brief Returns the AptLinker for this target. */
    APT_INLINE AptLinker *GetLinker()
    {
        return mpLinker;
    }

    /** @brief Returns the next AptTarget in the list. */
    APT_INLINE AptTarget *GetNext()
    {
        return mpNext;
    }

    /** @brief Returns the previous target in the list. */
    APT_INLINE AptTarget *GetPrevious()
    {
        return mpPrev;
    }

    /** @brief Sets the next item in the target list. */
    APT_INLINE void SetNext(AptTarget *pNew)
    {
        mpNext = pNew;
    }

    /** @brief Sets the previous target in the target list. */
    APT_INLINE void SetPrevious(AptTarget *pNew)
    {
        mpPrev = pNew;
    }

    APT_INLINE AptBCRenderTreeManager *GetRenderManager()
    {
        return mpRTMgr;
    }

    /** @brief Returns the library this target belongs to.
     *
     * Prefer this over AptGetLib() in code that already has a target in hand:
     * it is the form that keeps working if the library ever stops being
     * ambient. */
    APT_INLINE AptLibrary *GetLibrary()
    {
        return mpLibrary;
    }

  private:
    // don't copy
    AptTarget(const AptTarget &);
    AptTarget &operator=(const AptTarget &);

    AptLibrary *mpLibrary;
    AptTargetInitParams mInitParms;
    AptAnimationTarget *mpPool;
    AptLoader *mpLoader;
    AptLinker *mpLinker;
    AptTarget *mpNext;
    AptTarget *mpPrev;

    AptBCRenderTreeManager *mpRTMgr;
};

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/
