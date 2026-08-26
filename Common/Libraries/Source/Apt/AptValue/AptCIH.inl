// Tons of inlined functions provided to give callers type safety.

AptCharacterSpriteInst *AptCIH::GetSpriteInst()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetSpriteInst();
}

AptCharacterSpriteInstBase *AptCIH::GetSpriteInstBase()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetSpriteInstBase();
}

AptCharacterTextInst *AptCIH::GetDynamicTextInst()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetDynamicTextInst();
}

AptCharacterStaticTextInst *AptCIH::GetStaticTextInst()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetStaticTextInst();
}

AptCharacterMorphInst *AptCIH::GetMorphInst()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetMorphInst();
}

AptCharacterButtonInst *AptCIH::GetButtonInst()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetButtonInst(); // Always returns false if no button support is compiled in.
}

AptCharacterAnimationInst *AptCIH::GetAnimationInst()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetAnimationInst();
}

AptCharacterShapeInst *AptCIH::GetShapeInst()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetShapeInst();
}

AptCharacterImageInst *AptCIH::GetImageInst()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetImageInst();
}

AptCharacterInst *AptCIH::GetCharacterInst()
{
    return mpCharacterInst;
}

//-------------------------------------------------------------------------------------------------------------
// Const versions of the above get functions.

const AptCharacterSpriteInst *AptCIH::GetSpriteInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetSpriteInst();
}

const AptCharacterSpriteInstBase *AptCIH::GetSpriteInstBase() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetSpriteInstBase();
}

const AptCharacterTextInst *AptCIH::GetDynamicTextInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetDynamicTextInst();
}

const AptCharacterStaticTextInst *AptCIH::GetStaticTextInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetStaticTextInst();
}

const AptCharacterMorphInst *AptCIH::GetMorphInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetMorphInst();
}

const AptCharacterButtonInst *AptCIH::GetButtonInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetButtonInst();
}

const AptCharacterAnimationInst *AptCIH::GetAnimationInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetAnimationInst();
}

const AptCharacterShapeInst *AptCIH::GetShapeInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetShapeInst();
}

const AptCharacterImageInst *AptCIH::GetImageInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetImageInst();
}

const AptCharacterInst *AptCIH::GetCharacterInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetCharacterInst();
}

//-------------------------------------------------------------------------------------------------------------
// Each function returns true if the character inst is the specified type.

bool AptCIH::IsSpriteInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsSpriteInst();
}

bool AptCIH::IsSpriteInstBase() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsSpriteInstBase();
}

bool AptCIH::IsButtonInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsButtonInst();
}

bool AptCIH::IsShapeInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsShapeInst();
}

bool AptCIH::IsDynamicTextInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsDynamicTextInst();
}

bool AptCIH::IsStaticTextInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsStaticTextInst();
}

bool AptCIH::IsMorphInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsMorphInst();
}

bool AptCIH::IsAnimationInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsAnimationInst();
}

bool AptCIH::IsLevelInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsLevelInst();
}

bool AptCIH::IsImageInst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsImageInst();
}

/** Added for the global gpUndefinedCIH object. */
bool AptCIH::IsNone() const
{
    APT_ASSERT(this);
    return getVtblIndex() == AptVFT_CIHNone;
}

bool AptCIH::IsCustomControlInst(void) const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->IsCustomControlInst();
}

bool AptCIH::GetHasBlendMode() const
{
    return mbHasBlendMode;
}

void AptCIH::SetHasBlendMode(uint32_t nBlendMode)
{
    mbHasBlendMode = nBlendMode;
}

bool AptCIH::GetHasFilterEffects() const
{
    return mbHasFilters;
}

void AptCIH::SetHasFilterEffects(uint32_t nUIEffect)
{
    mbHasFilters = nUIEffect;
}

bool AptCIH::GetHasUIEffects() const
{
    return (mbHasBlendMode || mbHasFilters);
}

/**
 * @return the AptNativeHash of this instance. Static, not virtual: prefer calling this over the
 * virtual version if possible.
 */
AptNativeHash *AptCIH::GetNativeHash() const
{
    const AptCharacterInst *pData = mpCharacterInst;

    if (pData)
    {
        return (pData->GetNativeHash());
    }
    return (NULL);
}

/**
 * Marks an event-handler bit as set. The CIH has no knowledge of what the bits mean; it's up to
 * the caller. Lets callers check whether certain event handlers are present without searching.
 */
void AptCIH::SetEventHandler(int nEvent)
{
    if (ContainsNativeHashVirtual())
    {
        GetNativeHash()->SetEventHandler(nEvent);
    }
}

/** Clears an event-handler bit set via SetEventHandler(). */
void AptCIH::RemoveEventHandler(int nEvent)
{
    if (ContainsNativeHashVirtual())
    {
        GetNativeHash()->RemoveEventHandler(nEvent);
    }
}

/** @return true if any mouse events are set. */
int AptCIH::HasMouseEvent()
{
    return (HasEvent(AptEventActionFlag_MouseEvents));
}

/** Notifies the render tree manager that this item is being added. */
void AptCIH::SetIsInserted()
{
    if (mpCharacterInst != NULL)
    {
        AptCharacterInst::ItemInserted(this);
    }
}

/** Sets the next display-list item pointed to by this object; also notifies the character inst. */
void AptCIH::SetDisplayListNext(AptCIH *pNew)
{
    APT_ASSERT(mbInRemList == false);
    mpNext = pNew;
}

/**
 * Sets the display list's previous pointer to the new item. If this object was moved to the head
 * of a list, notifies that the parent's first child (this) has changed.
 */
void AptCIH::SetDisplayListPrevious(AptCIH *pNew)
{
    APT_ASSERT(mbInRemList == false);
    mpPrev = pNew;
}

/** @return true if this CIH is itself a mask. */
bool AptCIH::IsMask() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetIsMask();
}

/** @return true if this CIH has a mask. */
bool AptCIH::HasMask() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetHasMask();
}

void AptCIH::SetIsMask(bool bNewValue, AptMatrix *pMatrix)
{
    APT_ASSERT(mpCharacterInst != NULL);
    mpCharacterInst->SetIsMask(bNewValue, pMatrix);
}

void AptCIH::SetHasMask(bool bNewValue, AptRenderItem *pMask)
{
    APT_ASSERT(mpCharacterInst != NULL);
    mpCharacterInst->SetHasMask(bNewValue, pMask);
}

/** @return the depth of this CIH. */
int32_t AptCIH::GetDepth() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetDepth();
}

void AptCIH::SetDepth(uint32_t nDepth)
{
    APT_ASSERT(mpCharacterInst != NULL);
    mpCharacterInst->SetDepth(nDepth);
}

/** Sets the display-list parent pointer. */
void AptCIH::SetDisplayListParent(AptCIH *pNew)
{
    mpParent = pNew;
}

/** @return a writable pointer to the position matrix. */
AptMatrix *AptCIH::GetPositionMatrixWritable()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetPositionMatrixWritable();
}

/** @return a const pointer to the position matrix. */
const AptMatrix *AptCIH::GetPositionMatrixConst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetPositionMatrixConst();
}

/** @return a writable pointer to the color matrix. */
AptCXForm *AptCIH::GetColorMatrixWritable()
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetColorMatrixWritable();
}

/** @return a const pointer to the color matrix (cxform inside RenderItem). */
const AptCXForm *AptCIH::GetColorMatrixConst() const
{
    APT_ASSERT(mpCharacterInst != NULL);
    return mpCharacterInst->GetColorMatrixConst();
}

/** @return the playing flag. */
bool AptCIH::GetIsPlaying() const
{
    return GetSpriteInstBase()->mbIsPlaying == 1;
}

/** @return the dirty state. */
bool AptCIH::GetDirtyState() const
{
    return mbDirty == 1;
}

/** @return the head item of the child display list, or NULL. */
const AptCIH *AptCIH::GetFirstChild() const
{
    if (IsSpriteInstBase())
    {
        const AptCharacterSpriteInstBase *base = GetSpriteInstBase();
        if (base->mDisplayList.getState())
        {
            const AptDisplayListState *state = base->mDisplayList.getState();
            return state->GetFirstItem();
        }
    }
#if defined APT_USE_BUTTONS
    else if (IsButtonInst())
    {
        const AptCharacterButtonInst *pButtonInst = GetButtonInst();
        if (pButtonInst->mDisplayList.getState())
        {
            const AptDisplayListState *state = pButtonInst->mDisplayList.getState();
            return state->GetFirstItem();
        }
    }
#endif
    return NULL;
}
