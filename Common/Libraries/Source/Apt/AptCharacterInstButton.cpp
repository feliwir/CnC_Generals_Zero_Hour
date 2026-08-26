#include "_Apt.h"
#include "AptCharacterInst.h"

#include "MainInline.h"
#if defined APT_USE_BUTTONS
void AptCharacterButtonInst::PreDestroy()
{
    mDisplayList.PreDestroy();
}
#endif
