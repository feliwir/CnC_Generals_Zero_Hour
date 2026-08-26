#include "AptObject/AptExternalFunction.h"
#include "_AptValue.h"

/** @param classSize size of the concrete subclass, e.g. sizeof(Blah). */
AptExternalFunction::AptExternalFunction(uint32_t classSize)
    : AptObject(AptVFT_ExternalFunction), mClassSize(classSize)
{
    // Guessing this should match AptNativeFunction.
    SetAllowDelayedDeletion(false);
}

AptExternalFunction::~AptExternalFunction()
{
    // Don't touch mClassSize here -- it's still read after the destructor runs.
}

/** @return the size of this object; used where Apt needs an object's size but a global lookup table doesn't apply to extensions. */
uint32_t AptExternalFunction::GetClassSize() const
{
    return mClassSize;
}
