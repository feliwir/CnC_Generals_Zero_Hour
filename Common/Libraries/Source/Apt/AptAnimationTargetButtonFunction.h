#pragma once

#if defined APT_USE_BUTTONS

#if defined(_MSC_VER)
#pragma once
#endif

/*** Include files ********************************************************************************/
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptCharacterInst.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/
#if defined APT_USE_BUTTONS
struct InputFlagT
{
    int32_t nFlag;
    StringCode eName;
    ;
    int32_t nInputState;
};

static InputFlagT _aInputFlags[] =
    {
        {AptEventActionFlag_Release, SC_onRelease, AptInputState_Released},
        {AptEventActionFlag_Press, SC_onPress, AptInputState_Pressed},
        {AptEventActionFlag_ReleaseOutside, SC_onReleaseOutside, AptInputState_Released},
        {AptEventActionFlag_RollOver, SC_onRollOver, AptInputState_Released},
        {AptEventActionFlag_RollOut, SC_onRollOut, AptInputState_Released},
        {AptEventActionFlag_DragOver, SC_onDragOver, AptInputState_Pressed},
        {AptEventActionFlag_DragOut, SC_onDragOut, AptInputState_Pressed},
};

#endif

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/
static float _getButtonScore(AptInputType eInput, int nSourceX, int nSourceY, int nDestX, int nDestY)
{
#define _ABS(a) (a < 0 ? -a : a)
#define PERPENDICULAR_SCALE 10.f
    int yDiff = nDestY - nSourceY;
    int xDiff = nDestX - nSourceX;
    switch (eInput)
    {
    case AptInputType_Up:
        if (yDiff >= 0)
            return -1.f;
        return _ABS(yDiff) + PERPENDICULAR_SCALE * _ABS(xDiff);
        break;
    case AptInputType_Down:
        if (yDiff <= 0)
            return -1.f;
        return _ABS(yDiff) + PERPENDICULAR_SCALE * _ABS(xDiff);
        break;
    case AptInputType_Left:
        if (xDiff >= 0)
            return -1.f;
        return _ABS(xDiff) + PERPENDICULAR_SCALE * _ABS(yDiff);
        break;
    case AptInputType_Right:
        if (xDiff <= 0)
            return -1.f;
        return _ABS(xDiff) + PERPENDICULAR_SCALE * _ABS(yDiff);
        break;
    default:
        APT_ASSERT(NOT_REACHED);
        break;
    }

    return -1.f;
}

static bool GetCharacterGridPosition(const AptNativeString &strText, int32_t *pX, int32_t *pY)
{
    if (strText.IsEmpty())
    {
        return (false);
    }

    const char *pStr  = strText.c_str();
    int32_t len       = strText.Size();
    const char *szPtr = &pStr[len - 1];

    // get y position
    if (!isdigit(*szPtr))
        return false;
    while (isdigit(*szPtr))
        szPtr--;
    if (pY)
        *pY = atoi(&szPtr[1]);
    // separator
    if (*szPtr-- != '_')
        return false;
    // get x position
    if (!isdigit(*szPtr))
        return false;
    while (isdigit(*szPtr))
        szPtr--;
    if (pX)
        *pX = atoi(&szPtr[1]);

    return true;
}

/** @brief this function is moved from AptAuxPCOpenGL for hit test */
static void MatrixVecMult(float *pIn, AptMatrix *pMat, float *pOut)
{
    pOut[0] = pMat->a * pIn[0] + pMat->c * pIn[1] + pMat->tx;
    pOut[1] = pMat->b * pIn[0] + pMat->d * pIn[1] + pMat->ty;
}

/** @brief this function is moved from AptAuxPCOpenGL for hit test */
static int32_t PointInTri(float *v, float x, float y)
{
    int32_t i, j, c = 0;
    for (i = 0, j = 2; i < 3; j = i++)
    {
        float *pj = &v[j * 2];
        float *pi = &v[i * 2];
        if ((((pi[1] <= y) && (y < pj[1])) || ((pj[1] <= y) && (y < pi[1]))) &&
            (x < (pj[0] - pi[0]) * (y - pi[1]) / (pj[1] - pi[1]) + pi[0]))
        {
            c = !c;
        }
    }
    return c;
}
static AptCIH *GetBestButton(AptInputType eType, AptCIH *pCIH, AptCIH *pInstStart)
{
    if (pCIH == NULL)
        return NULL;

    float fBestScore   = 999999999.f, fCurScore;
    AptHashItem *pItem = NULL;
    int32_t nFocusX = 0, nFocusY = 0, nX = 0, nY = 0;
    AptCIH *pNewInst = NULL;

    APT_ASSERT(pCIH->IsSpriteInstBase());

    if (pInstStart)
    {
#if (APT_DEBUG_LEVEL > APT_DEBUG_NONE)
        bool bRet =
#endif
            GetCharacterGridPosition(pInstStart->GetInstanceName(), &nFocusX, &nFocusY);
#if (APT_DEBUG_LEVEL > APT_DEBUG_NONE)
        APT_ASSERT(bRet);
#endif
    }

    for (pItem = pCIH->GetCharacterInst()->GetNativeHash()->GetFirstItem(); pItem != NULL; pItem = pCIH->GetCharacterInst()->GetNativeHash()->GetNextItem(pItem))
    {
        AptValue *pValue = pItem->mValue;
        if (!pValue->isCIH())
            continue;

        AptCIH *pInst = pValue->c_cih();

        if (pInstStart == pInst)
            continue;
        if (!pInst->IsSpriteInstBase() && !pInst->IsButtonInst())
            continue;

        if (GetCharacterGridPosition(pItem->Key, &nX, &nY) == false)
            continue;
        if (GetTargetSim()->GetAnimationTarget()->IsInputMasked(pInst))
            continue;
        // just choose first button if we have no start point
        if (pInstStart == NULL)
        {
            pNewInst = pInst;
            break;
        }
        else // choose best button if we have a start point
        {
            APT_ASSERT(nFocusX != nX || nFocusY != nY);

            fCurScore = _getButtonScore(eType, nFocusX, nFocusY, nX, nY);
            if (fCurScore >= 0.f && fCurScore < fBestScore)
            {
                pNewInst   = pInst;
                fBestScore = fCurScore;
            }
        }
    }

    if (pNewInst)
    {
        // if it's a sprite, we want to look inside of it
        if (pNewInst->IsSpriteInst())
        {
            return GetBestButton(eType, pNewInst, NULL);
        }
        // if it's a button, we're done
        return pNewInst;
    }

    // if we didn't find anything, look in the parent
    return GetBestButton(eType, pCIH->GetDisplayListParent(), pCIH);
}

#endif // #if defined APT_USE_BUTTONS

