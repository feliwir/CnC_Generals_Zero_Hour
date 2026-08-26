/** Inline methods for AptMath, inlined only if !defined(APT_3D). */

#pragma once

#include "Apt.h"
#include "AptDefine.h"

#if !defined(APT_3D)

AptMath::ClipTransformT *AptMath::ClipStackPush()
{
    APT_ASSERT((m_nStackCount + 1) < m_nStackCapacity);
    return &m_pStackBase[++m_nStackCount];
}
AptMath::ClipTransformT *AptMath::ClipStackPop()
{
    APT_ASSERT(m_nStackCount < m_nStackCapacity);
    return &m_pStackBase[--m_nStackCount];
}
AptMath::ClipTransformT *AptMath::_ClipStackGetTop()
{
    APT_ASSERT(m_nStackCount < m_nStackCapacity);
    return (&m_pStackBase[m_nStackCount]);
}
bool AptMath::ClipStackIsEmpty()
{
    APT_ASSERT(m_nStackCount < m_nStackCapacity);
    return (m_nStackCount == 0);
}

#endif // !APT_3D
