#include "_Apt.h"
#include "Display/AptRenderingContext.h"
#include "Display/AptDisplayListState.h"

#include "AptRenderItem.h"
#include "AptRenderList.h"
#include "AptRenderableGeometry.h"
#include "MainInline.h"


/** Initializes the ref-counting/loading state for a character based on its type; only relevant under decoupled rendering. */
void AptCharacter::SetupCharacter()
{
#if defined(APT_DECOUPLED_RENDERING)
    m_pAnimFile = AptFilePtr(0);
    switch (eType)
    {
    case AptCharacterType_Shape:
        m_shapeData.m_nRef = 0;
        if (m_shapeData.m_textID != 0)
            m_shapeData.m_bNotLoaded = 1;
        else
            m_shapeData.m_bNotLoaded = 0;
        break;
    case AptCharacterType_Bitmap:
        m_bitmapData.m_nRef    = 0;
        m_bitmapData.m_bLoaded = 0;
        m_bitmapData.m_bBinded = 0;
        break;
    default:
        APT_ASSERT(false);
        // lint -fallthrough
    case AptCharacterType_Font:
    case AptCharacterType_Sound:
    case AptCharacterType_Morph:
    case AptCharacterType_StaticText:
    case AptCharacterType_Video:
    case AptCharacterType_Text:
    case AptCharacterType_Button:
    case AptCharacterType_Sprite:
    case AptCharacterType_Animation:
    case AptCharacterType_None:
        m_data.m_nRef = 0;
        break;
    case AptCharacterType_Image:
        m_data.m_nRef = 0;
        image.texture = NULL;
        break;
    }
#endif
}

/** Releases the AptFilePtr reference held for rendered data; only relevant under decoupled rendering. */
void AptCharacter::ReleaseAnimationFile() const
{
#if defined(APT_DECOUPLED_RENDERING)
    m_pAnimFile = AptFilePtr(0);
#endif
}

/** Increments the character's reference count, clamping at APT_RENDER_MAX_REFCOUNT. */
void AptCharacter::AddCharacterReference() const
{
    APT_ASSERT(this != NULL);
#if defined(APT_DECOUPLED_RENDERING)
#if defined(EA_DEBUG)
    if (eType != AptCharacterType_Animation && m_pAnimFile.pData == NULL)
    {
        if (eType == AptCharacterType_Image)
        {
            // Not sure what these checks are for!  Sorry for not understanding Apt too well!
            // - Colin C. 3/7/12
        }
        else if (eType == AptCharacterType_Shape || eType == AptCharacterType_Bitmap)
        {
            APT_ASSERTM(this->m_pAnimFile.pData != NULL, ("pData should not be NULL!"));
        }
        else if (m_data.m_nDynamic != 1)
        {
            APT_ASSERTM(this->m_pAnimFile.pData != NULL, ("pData should not be NULL!"));
        }
    }
#endif

    if (m_data.m_nRef >= APT_RENDER_MAX_REFCOUNT)
    {
        APT_FAIL("APT_RENDER_MAX_REFCOUNT hit for pCharacter");
        m_data.m_nRef = APT_RENDER_MAX_REFCOUNT;
    }
    else
    {
        ++m_data.m_nRef;
    }
#endif
}

/** Decrements the character's reference count and releases the animation file once it reaches zero. */
void AptCharacter::ReleaseCharacterReference() const
{
#if defined(APT_DECOUPLED_RENDERING)
    uint32_t uRefCountLocal;
    APT_ASSERT(this != NULL);
    APT_ASSERT(m_shapeData.m_nRef > 0 && "pCharacter ref count hitting negative values");

    uRefCountLocal = --m_data.m_nRef;

    // APT_ASSERT(m_data.m_nRef == uRefCountLocal);
    if (uRefCountLocal == 0)
    {
        // If the reference count is zero, we delete this instance.
        ReleaseAnimationFile();
    }
#endif
}

/**
 * Renders the character. Shapes are queued into the AptRenderList using the current clip
 * transform and mask state; other types are either ignored or unsupported for direct rendering.
 * @param pRenderingContext Rendering context passed around internally by Apt.
 * @param eMaskOperation Type of mask render operation to apply.
 * @param nMaskDepth Number of recursive masks below this character.
 * @param pRenderItem The render item this character belongs to.
 */
void AptCharacter::render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, const AptRenderItem *pRenderItem)
{
    switch (eType)
    {
    case AptCharacterType_Shape:
    {
        // we can probly get the matrix from pRenderContext instead (which would be cleaner)
        AptRenderInfo info;
        info.mnLevel     = AptRenderItem::suCurrentRenderAnimLevel;
        info.mTransform  = *AptMath::_ClipStackGetTop();
        info.meMaskOper  = eMaskOperation;
        info.mnMaskDepth = nMaskDepth;
        AptRenderList::Add(info, pRenderItem, shape.pRenderUnit);
        break;
    }

    break;
    case AptCharacterType_None:

    {
        // ignore
    }

    break;
    default:
    {
        APT_ASSERT(NOT_REACHED);
    }
    }
}

/**
 * Loads and binds all textures needed by the shape characters of this animation file. Called
 * from the render thread just before the first render of a shape character in this file, since
 * before lockless Apt this used to happen directly inside render().
 */
#if defined(APT_DECOUPLED_RENDERING)
void AptCharacter::LoadResourcesFromCharacter()
{
    switch (eType)
    {
    case AptCharacterType_Shape:
    {
        // APT_ASSERT(shape.zID);

        if (m_shapeData.m_bNotLoaded == 1)
        {
            APT_ASSERT(pParentAnim->animation.apCharacters[m_shapeData.m_textID]->eType == AptCharacterType_Bitmap);

            // code to go through all the characters in the parent animation and load any bitmaps which are there.
            //  this is needed because a shape can have multiple bitmaps in it and we need to load and bind them
            // all before we render them.
            int32_t nTemp = pParentAnim->animation.nCharacters;
            for (int32_t i = 0; i < nTemp; i++)
            {
                if (pParentAnim->animation.apCharacters[i] == NULL)
                {
                    continue;
                }
                if (pParentAnim->animation.apCharacters[i]->eType == AptCharacterType_Bitmap)
                {
                    if (pParentAnim->animation.apCharacters[i]->m_bitmapData.m_bLoaded == 0)
                    {
                        int32_t nTempId = pParentAnim->animation.IsImport(i);
                        if (nTempId != -1)
                        {
                            pParentAnim->animation.apCharacters[i]->bitmap.zID = AptGetUserFuncs().pfnLoadTexture(AptAnimationFile::Cast(pParentAnim->animation.apCharacters[i]->m_pAnimFile)->GetUserData(), pParentAnim->animation.GetIDFromImportFile(nTempId));
                            AptGetUserFuncs().pfnBindTexture(AptAnimationFile::Cast(m_pAnimFile)->GetUserData(), i, pParentAnim->animation.apCharacters[i]->bitmap.zID);
                            AptGetUserFuncs().pfnBindTexture(AptAnimationFile::Cast(pParentAnim->animation.apCharacters[i]->m_pAnimFile)->GetUserData(), pParentAnim->animation.GetIDFromImportFile(nTempId), pParentAnim->animation.apCharacters[i]->bitmap.zID);
                            pParentAnim->animation.apCharacters[i]->m_bitmapData.m_bLoaded = 1;
                            pParentAnim->animation.apCharacters[i]->m_bitmapData.m_bBinded = 1;
                        }
                        else
                        {
                            // If pAnimFile is NULL, we search for an imported item and uses that m_pAnimFile pointer. This is safe
                            // because if we reach this case, we know that at least one thing in the exporting character list is being used.
                            AptAnimationFile *pTmp = AptAnimationFile::Cast(m_pAnimFile);
                            if (pTmp == NULL)
                            {
                                int32_t ii = 0;
                                for (ii = 0; ii < pParentAnim->animation.nExports; ii++)
                                {
                                    if (pParentAnim->animation.apCharacters[pParentAnim->animation.aExports[ii].nID] != NULL && pParentAnim->animation.apCharacters[pParentAnim->animation.aExports[ii].nID]->m_pAnimFile.pData != NULL)
                                    {
                                        pTmp = AptAnimationFile::Cast(pParentAnim->animation.apCharacters[pParentAnim->animation.aExports[ii].nID]->m_pAnimFile);
                                        break;
                                    }
                                }
                                APT_ASSERT(ii != pParentAnim->animation.nExports);
                            }
                            if (pTmp) // If pTmp continues being NULL the assert is triggered, but this avoids an eventual crash
                            {
                                pParentAnim->animation.apCharacters[i]->bitmap.zID = AptGetUserFuncs().pfnLoadTexture(pTmp->GetUserData(), i);
                                AptGetUserFuncs().pfnBindTexture(pTmp->GetUserData(), i, pParentAnim->animation.apCharacters[i]->bitmap.zID);
                                pParentAnim->animation.apCharacters[i]->m_bitmapData.m_bLoaded = 1;
                            }
                        }
                    }
                    else if (pParentAnim->animation.apCharacters[i]->m_pAnimFile != m_pAnimFile && pParentAnim->animation.apCharacters[i]->m_pAnimFile.pData != NULL)
                    {
                        // that means this is a imported bitmap, now we have to bind it to all shapes in this animation.
                        if (pParentAnim->animation.apCharacters[i]->m_bitmapData.m_bBinded == 0 || m_shapeData.m_bNotLoaded == 1)
                        {
                            AptGetUserFuncs().pfnBindTexture(AptAnimationFile::Cast(m_pAnimFile)->GetUserData(), i, pParentAnim->animation.apCharacters[i]->bitmap.zID);
                            pParentAnim->animation.apCharacters[i]->m_bitmapData.m_bBinded = 1;
                        }
                    }
                    else if (pParentAnim->animation.apCharacters[i]->m_pAnimFile.pData == NULL && m_shapeData.m_bNotLoaded == 1)
                    {
                        AptGetUserFuncs().pfnBindTexture(AptAnimationFile::Cast(m_pAnimFile)->GetUserData(), i, pParentAnim->animation.apCharacters[i]->bitmap.zID);
                    }
                }
                else if (pParentAnim->animation.apCharacters[i]->eType == AptCharacterType_Shape)
                {
                    pParentAnim->animation.apCharacters[i]->m_shapeData.m_bNotLoaded = 0;
                }
            }
        }
        break;
    }

    break;
    case AptCharacterType_None:

    {
        // ignore
    }

    break;
    default:
    {
        APT_ASSERT(NOT_REACHED);
    }
    }
}
#endif

/**
 * Computes the bounding rectangle of the character (shapes, static text, and images) after
 * being transformed by the given matrix, expanding pRect to include it.
 * @param pRenderingContext Rendering context used to perform the bounding-rect expansion.
 * @param pMatrix Transform to apply to the character's local bounds.
 * @param pRect Bounding rectangle to expand.
 */
void AptCharacter::GetBoundingRect(AptRenderingContext *pRenderingContext, const AptMatrix *pMatrix, AptRect *pRect) const
{
    switch (eType)
    {
    case AptCharacterType_Shape:
    {
        pRenderingContext->expandBoundingRect(&shape.rBounds, pMatrix, pRect);
        break;
    }

    case AptCharacterType_None:
    {
        // ignore
        break;
    }

    case AptCharacterType_StaticText:
    {
        pRenderingContext->expandBoundingRect(&statictext.rBounds, pMatrix, pRect);
        break;
    }

    case AptCharacterType_Image:
    {
        pRenderingContext->expandBoundingRect(&image.bounds, pMatrix, pRect);
        break;
    }

    default:
    {
        APT_ASSERT(NOT_REACHED);
        break;
    }
    }
}
