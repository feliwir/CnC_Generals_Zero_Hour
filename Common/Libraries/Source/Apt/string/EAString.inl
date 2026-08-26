#pragma once

#include <cstring>
#include "Apt.h"
#include "AptDefine.h"
#include "string/EAString_platform.h"


/** Sets the used length of the string; clamps to (and asserts on exceeding) the allocated max size. */
void EAStringC::SetInternalSize(uint32_t uSize)
{
    APT_ASSERT(uSize <= GetInternalMaxSize());
    if (uSize > GetInternalMaxSize())
    {
        uSize = GetInternalMaxSize();
        APT_ASSERT(uSize <= 0xffff);
    }

    m_pData->m_uSize = static_cast<uint16_t>(uSize);
}

/** @return the allocated capacity of the internal buffer, not counting the trailing '\0'. */
uint32_t EAStringC::GetInternalMaxSize() const
{
    return (m_pData->m_uMaxSize);
}

/** Sets the allocated capacity of the internal buffer. */
void EAStringC::SetInternalMaxSize(uint32_t uMaxSize)
{
    APT_ASSERT(uMaxSize <= 0xffff);
    m_pData->m_uMaxSize = static_cast<uint16_t>(uMaxSize);
}

/** @return the reference count on the shared buffer. */
uint32_t EAStringC::GetInternalRefCount() const
{
    return (m_pData->m_uRefCount);
}

/** Sets the reference count on the shared buffer. */
void EAStringC::SetInternalRefCount(const uint32_t uRefCount)
{
    APT_ASSERT(uRefCount <= 0xffff);
    m_pData->m_uRefCount = static_cast<uint16_t>(uRefCount);
}

/** @return true if this string points at the shared, un-refcounted empty string. */
bool EAStringC::IsEmpty() const
{
    return (m_pData == (DebugDataC *)&s_EmptyInternalData);
}

/** @return the cached case-insensitive hash value; must have already been computed. */
uint16_t EAStringC::GetHashValue() const
{
    APT_ASSERT(m_pData->m_uHash != 0);
    return (m_pData->m_uHash);
}

uint32_t EAStringC::operator[](const int32_t index) const
{
    return (*(GetBuffer() + index));
}

bool EAStringC::operator!=(const EAStringC &strText) const
{
    return (operator==(strText) == false);
}

bool EAStringC::operator>(const EAStringC &strText) const
{
    return (operator<=(strText) == false);
}

bool EAStringC::operator>=(const EAStringC &strText) const
{
    return (operator<(strText) == false);
}

bool EAStringC::operator==(const char *const pStrText) const
{
    return (strcmp(GetBuffer(), pStrText) == 0);
}

bool EAStringC::operator!=(const char *const pStrText) const
{
    return (operator==(pStrText) == false);
}

bool EAStringC::operator<(const char *const pStrText) const
{
    return (strcmp(GetBuffer(), pStrText) < 0);
}

bool EAStringC::operator<=(const char *const pStrText) const
{
    return (strcmp(GetBuffer(), pStrText) <= 0);
}

bool EAStringC::operator>(const char *const pStrText) const
{
    return (operator<=(pStrText) == false);
}

bool EAStringC::operator>=(const char *const pStrText) const
{
    return (operator<(pStrText) == false);
}

/** Releases this string's reference on its buffer and resets it to the shared empty string. */
void EAStringC::Clear()
{
    DecreaseInternalRefCount(m_pData);
    m_pData = (DebugDataC *)&s_EmptyInternalData;
    IncreaseInternalRefCount();
}

/** Resets a constant/pooled string's pointer to the shared empty string, without touching ref counts. */
void EAStringC::ClearConstantValue()
{
    APT_ASSERT(m_pData->m_uSize > m_pData->m_uMaxSize && "TRYING TO USE INCORRECT CLEAR A NON STATIC STRING IN THE STRING POOL");
    m_pData = (DebugDataC *)&s_EmptyInternalData;
}

bool EAStringC::Equal(const char *const pStrText) const
{
    APT_ASSERT(pStrText != NULL);
    return (strcmp(GetBuffer(), pStrText) == 0);
}

bool EAStringC::Equal(const EAStringC &strText) const
{
    return (*this == strText);
}

bool EAStringC::EqualNoCase(const char *const pStrText) const
{
    APT_ASSERT(pStrText != NULL);
    return (APT_STRING_NOCASE_COMPARE(GetBuffer(), pStrText) == 0);
}

int32_t EAStringC::Compare(const char *const pStrText) const
{
    APT_ASSERT(pStrText != NULL);
    return (strcmp(GetBuffer(), pStrText));
}

int32_t EAStringC::Compare(const EAStringC &strText) const
{
    if (m_pData == strText.m_pData)
    {
        //  Same string
        return (0);
    }

    return (strcmp(GetBuffer(), strText.GetBuffer()));
}

int32_t EAStringC::CompareNoCase(const char *const pStrText) const
{
    APT_ASSERT(pStrText != NULL);
    return (APT_STRING_NOCASE_COMPARE(GetBuffer(), pStrText));
}

int32_t EAStringC::CompareNoCase(const EAStringC &strText) const
{
    if (m_pData == strText.m_pData)
    {
        //  Same string
        return (0);
    }

    return (APT_STRING_NOCASE_COMPARE(GetBuffer(), strText.GetBuffer()));
}

int EAStringC::Size() const
{
    return (GetLength());
}

void EAStringC::Reserve(int32_t iSize)
{
    ReserveSize(iSize);
}

/** Invalidates this string; asserts it was previously valid. Only Validate() may follow. */
void EAStringC::Invalidate()
{
    APT_ASSERT(IsValid());
    DecreaseInternalRefCount(m_pData);
    m_pData = NULL;
}

/** Re-validates a previously Invalidate()'d string as the empty string. */
void EAStringC::Validate()
{
    APT_ASSERT(IsValid() == false);
    m_pData = (DebugDataC *)&s_EmptyInternalData;
    IncreaseInternalRefCount();
}

/** Re-validates a previously Invalidate()'d string so it shares strText's buffer. */
void EAStringC::Validate(const EAStringC &strText)
{
    APT_ASSERT(IsValid() == false);
    m_pData = strText.m_pData;
    IncreaseInternalRefCount();
}

bool EAStringC::IsValid() const
{
    return (m_pData != NULL);
}

/** Recomputes and caches the string's case-insensitive hash value. */
void EAStringC::CalculateHashValue() const
{
    m_pData->m_uHash = CalculateHashValue(GetInternalBuffer());
}

/** @return the cached hash value, computing it first if it hasn't been calculated yet. */
uint16_t EAStringC::UpdateHashValue() const
{
    if (m_pData->m_uHash == 0)
    {
        CalculateHashValue();
        APT_ASSERT(m_pData->m_uHash != 0);
    }
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    else
    {
        uint16_t uOldHash = m_pData->m_uHash;
        CalculateHashValue();
        APT_ASSERT(m_pData->m_uHash == uOldHash);
    }
#endif
    return (m_pData->m_uHash);
}

/** Marks the cached hash value as stale; lazily recomputed by UpdateHashValue(). */
void EAStringC::InvalidateHashValue() const
{
    m_pData->m_uHash = 0;
}
