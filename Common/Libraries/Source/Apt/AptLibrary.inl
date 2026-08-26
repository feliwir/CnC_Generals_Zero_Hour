#pragma once

#include "AptLibrary.h"

/** @return the live library.
 *
 * The assert is the point of the whole exercise: reaching Apt state before
 * initialize or after shutdown now fails loudly here, rather than silently
 * reading a stale global.
 */
AptLibrary *AptGetLib()
{
    APT_ASSERT(gpAptLibrary && "Apt library used before initialize or after shutdown");
    return gpAptLibrary;
}
