#include "_Apt.h"
#include "AptObject/AptSound.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "AptCharacter/AptCharacter.h"
#if defined(APT_USE_SOUND_OBJECT)
#include "AptSoundMembers.h"
#endif

#include "MainInline.h"

#if defined(APT_USE_SOUND_OBJECT)

static const int AptSoundMethod_attachSound = 1;
static const int AptSoundMethod_start       = 2;
static const int AptSoundMethod_stop        = 3;

AptSound::AptSound(AptCIH *pParent) : AptObject(AptVFT_Sound)
{
    zID         = 0;
    pParentAnim = pParent->GetSpriteInstBase()->GetCharacterConst()->pParentAnim;
    szName      = 0;
}

void AptSound::CleanNativeFunctions()
{
    NATIVE_MEMBER_FUNCTION_DESTROY(attachSound);
    NATIVE_MEMBER_FUNCTION_DESTROY(stop);
    NATIVE_MEMBER_FUNCTION_DESTROY(start);
}

NATIVE_MEMBER_FUNCTION(AptSound, attachSound)
{
    if (pThis->isSound())
    {
        AptValue *pName = gAptActionInterpreter.stackAt(0);
        AptNativeString sBuf;
        AptSound *pSound                   = pThis->c_sound();
        const AptCharacterAnimation *pAnim = &pSound->pParentAnim->animation;
        pName->toString(sBuf);

        for (int i = 0; i < pAnim->nExports; i++)
        {
            if (sBuf.EqualNoCase(pAnim->aExports[i].szName))
            {
                if (pAnim->apCharacters[pAnim->aExports[i].nID]->eType == AptCharacterType_Sound)
                {
                    pSound->zID    = pAnim->apCharacters[pAnim->aExports[i].nID]->sound.zID;
                    pSound->szName = pAnim->aExports[i].szName; // const pointer, do not change the name
                }
                break;
            }
        }
    }

    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptSound, start)
{
    if (pThis->isSound())
    {
        AptSound *pSound = pThis->c_sound();
        if (pSound->zID)
        {
            AptGetUserFuncs().pfnStartSound(pSound->zID, pSound->szName);
        }
    }

    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptSound, stop)
{
    if (pThis->isSound())
    {
        APT_ASSERT(NOT_REACHED); // no pfnStopSound yet
    }

    return gpUndefinedValue;
}

/**
 * @return the AptSound native member function for pName, if any.
 */
AptValue *AptSound::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    SoundMembers *pProp = pContext ? SoundMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case AptSoundMethod_attachSound:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(attachSound);
        }

        break;
        case AptSoundMethod_start:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(start);
        }

        break;
        case AptSoundMethod_stop:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(stop);
        }
        }
    }
    return 0;
}
#else
void AptSound::CleanNativeFunctions()
{
    // Nothing to do when the sound object is compiled out.
}
#endif
