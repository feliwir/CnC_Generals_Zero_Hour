/** A ref-counted string class. */

#include "Apt.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "_Apt.h"

#include "MainInline.h"

#include "string/EAString_platform.h"

//  This is the empty string, we can't release it
char EAStringC::s_EmptyInternalData[8 + 1] = "\x01\x01\x00\x00\x00\x00\x00\x00";
/*static*/ char *EAStringC::Get_s_EmptyInternalData() { return s_EmptyInternalData; }

AptUserFunctions *EAStringC::sAptCallbacks = NULL;


#if !defined(APT_ENABLE_INLINE)
#include "string/EAString.inl"
#endif


EAStringC::EAStringC() : m_pData((DebugDataC *)&s_EmptyInternalData)
{
}

EAStringC::EAStringC(const char *const pStrText)
{
    InitFromBuffer(pStrText);
}

EAStringC::EAStringC(const EAStringC &strText) : m_pData(strText.m_pData)
{
    IncreaseInternalRefCount();
}

EAStringC::~EAStringC()
{
    DecreaseInternalRefCount(m_pData);
}

/** Reserves uReservedLength bytes up front, or shares the empty string if 0. */
EAStringC::EAStringC(const uint32_t uReservedLength)
{
    if (uReservedLength)
    {
        AllocateBuffer(uReservedLength);
        SetInternalSize(0);
        InvalidateHashValue();
        *GetInternalBuffer() = '\0';
    }
    else
    {
        //  The size is 0, we point to the empty string
        m_pData = (DebugDataC *)&s_EmptyInternalData;
        IncreaseInternalRefCount();
    }

}

/** Builds a string of uLength copies of uChar, or shares the empty string if uLength is 0. */
EAStringC::EAStringC(const uint32_t uChar, const uint32_t uLength)
{
    if (uLength)
    {
        AllocateBuffer(uLength);
        memset(GetInternalBuffer(), uChar, uLength);
        SetInternalSize(uLength);
        InvalidateHashValue();
        *(GetInternalBuffer() + uLength) = '\0';
    }
    else
    {
        //  The size is 0, we point to the empty string
        m_pData = (DebugDataC *)&s_EmptyInternalData;
        IncreaseInternalRefCount();
    }

}

/** Increments the shared buffer's ref count; no-op for the shared empty string. */
void EAStringC::IncreaseInternalRefCount()
{
    if ((char *)m_pData != s_EmptyInternalData)
    {
        APT_ASSERT(m_pData->m_uRefCount <= 0xfffe);
        ++m_pData->m_uRefCount;
    }
}

/** Decrements the shared buffer's ref count, freeing it once it reaches zero; no-op for the shared empty string. */
void EAStringC::DecreaseInternalRefCount(StringDataC *const pData)
{
    if ((char *)pData != s_EmptyInternalData)
    {
        uint32_t uRefCountLocal;
        APT_ASSERT(pData->m_uRefCount >= 1);
        uRefCountLocal = --pData->m_uRefCount;
        if (uRefCountLocal == 0)
        {
            //  We need to deallocate the buffer
            APT_ASSERT((char *)pData != s_EmptyInternalData); //  Can't deallocate the empty string

            APT_FREE_BLOCK_TRACKER(AptNativeStringBuffers, pData, sizeof(StringDataC) + pData->m_uMaxSize + 1);
        }
    }
}

EAStringC &EAStringC::operator=(const EAStringC &strText)
{
    //  We don't test equality A == B, as the increase and decrease are very fast
    //  Statistically, this case happens not often
    EAStringC *nonConstObject = (EAStringC *)&strText;
    nonConstObject->IncreaseInternalRefCount();
    DecreaseInternalRefCount(m_pData);
    m_pData = strText.m_pData;
    return (*this);
}

EAStringC &EAStringC::operator=(const StaticStringHelperT &strStruct)
{
    m_pData = (EAStringC::DebugDataC *)&strStruct;
    return (*this);
}

bool EAStringC::operator==(const EAStringC &strText) const
{
    uint32_t uSize;
    uSize = GetLength();

    if (uSize != strText.GetLength())
    {
        //  Not the same size, can't be equal
        return (false);
    }

    if (m_pData == strText.m_pData)
    {
        //  This the same string, don't need to compare
        return (true);
    }

    //  Don't need to test the ending '\0' byte
    if (memcmp(GetInternalBuffer(), strText.GetInternalBuffer(), uSize) == 0)
    {
        return (true);
    }

    return (false);
}

EAStringC EAStringC::operator+(const EAStringC &strText) const
{
    uint32_t uLength1, uLength2;

    uLength1 = GetLength();
    if (uLength1 == 0)
    {
        return (strText);
    }
    uLength2 = strText.GetLength();
    if (uLength2 == 0)
    {
        return (*this);
    }

    EAStringC strNewText(uLength1 + uLength2); //  Reserve the size
    char *pStrBuffer;

    pStrBuffer = strNewText.GetInternalBuffer();
    memcpy(pStrBuffer, GetBuffer(), uLength1);
    memcpy(pStrBuffer + uLength1, strText.GetBuffer(), uLength2);
    pStrBuffer[uLength1 + uLength2] = '\0';
    strNewText.SetInternalSize(uLength1 + uLength2);
    InvalidateHashValue();

    return (strNewText);
}

EAStringC &EAStringC::operator+=(const EAStringC &strText)
{
    char *pStrBuffer;
    uint32_t uLength1, uLength2;

    uLength1 = GetLength();
    if (uLength1 == 0)
    {
        (*this) = strText;
        return (*this);
    }
    uLength2 = strText.GetLength();
    if (uLength2 == 0)
    {
        return (*this);
    }

    ChangeBuffer(uLength1 + uLength2, 0, uLength1, CB_NO_PUSH_ZERO, uLength1 + uLength2);

    pStrBuffer = GetInternalBuffer();
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    memcpy(pStrBuffer + uLength1, strText.GetBuffer(), uLength2 + 1); //  +1 to copy the trailing zero

    return (*this);
}

EAStringC EAStringC::operator+(const char *const pStrText) const
{
    uint32_t uLength1, uLength2;

    uLength1 = GetLength();
    if (uLength1 == 0)
    {
        return (pStrText); //  Implicit creation of a string
    }
    uLength2 = static_cast<uint32_t>(strlen(pStrText));
    if (uLength2 == 0)
    {
        return (*this);
    }

    EAStringC strNewText(uLength1 + uLength2); //  Reserve the size
    char *pStrBuffer;

    pStrBuffer = strNewText.GetInternalBuffer();
    memcpy(pStrBuffer, GetBuffer(), uLength1);
    memcpy(pStrBuffer + uLength1, pStrText, uLength2);
    pStrBuffer[uLength1 + uLength2] = '\0';
    strNewText.SetInternalSize(uLength1 + uLength2);
    InvalidateHashValue();

    return (strNewText);
}

EAStringC &EAStringC::operator+=(const char *const pStrText)
{
    char *pStrBuffer;
    uint32_t uLength1, uLength2;

    uLength1 = GetLength();
    if (uLength1 == 0)
    {
        (*this) = pStrText;
        return (*this);
    }
    uLength2 = static_cast<uint32_t>(strlen(pStrText));
    if (uLength2 == 0)
    {
        return (*this);
    }

    ChangeBuffer(uLength1 + uLength2, 0, uLength1, CB_NO_PUSH_ZERO, uLength1 + uLength2);

    pStrBuffer = GetInternalBuffer();
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    memcpy(pStrBuffer + uLength1, pStrText, uLength2 + 1); //  +1 to copy the trailing zero

    return (*this);
}

EAStringC operator+(const char *const pStrText, const EAStringC &strText)
{
    uint32_t uLength1, uLength2;

    uLength2 = strText.GetLength(); //  Do string first because it's faster
    if (uLength2 == 0)
    {
        return (pStrText); //  Implicit creation of a string
    }
    uLength1 = static_cast<uint32_t>(strlen(pStrText));
    if (uLength1 == 0)
    {
        return (strText);
    }

    EAStringC strNewText(uLength1 + uLength2); //  Reserve the size
    char *pStrBuffer;

    pStrBuffer = strNewText.GetInternalBuffer();
    memcpy(pStrBuffer, pStrText, uLength1);
    memcpy(pStrBuffer + uLength1, strText.GetBuffer(), uLength2);
    pStrBuffer[uLength1 + uLength2] = '\0';
    strNewText.SetInternalSize(uLength1 + uLength2);
    strNewText.InvalidateHashValue();

    return (strNewText);
}

bool EAStringC::operator<(const EAStringC &strText) const
{
    if (m_pData == strText.m_pData)
    {
        //  This the same string, don't need to compare
        return (false);
    }

    //  Can't use advantage of the string here
    if (strcmp(GetInternalBuffer(), strText.GetInternalBuffer()) < 0)
    {
        return (true);
    }

    return (false);
}

bool EAStringC::operator<=(const EAStringC &strText) const
{
    if (m_pData == strText.m_pData)
    {
        //  This the same string, don't need to compare
        return (true);
    }

    //  Can't use advantage of the string here
    if (strcmp(GetInternalBuffer(), strText.GetInternalBuffer()) <= 0)
    {
        return (true);
    }

    return (false);
}

/** Case-insensitive strstr; used only by Find()/Replace() when ignoreCase is requested. */
static const char *StrstrNoCase(const char *s1, const char *s2)
{
    const char *cp = s1;

    if (!*s2)
        return s1;

    while (*cp)
    {
        const char *s = cp;
        const char *t = s2;

        while (*s && *t && (tolower(static_cast<unsigned char>(*s)) == tolower(static_cast<unsigned char>(*t))))
            ++s, ++t;

        if (*t == 0)
            return cp;
        ++cp;
    }

    return NULL;
}

static inline const char *StrstrCaseOption(const char *str, const char *find, bool ignoreCase)
{
    return ignoreCase ? StrstrNoCase(str, find) : strstr(str, find);
}

bool EAStringC::EqualNoCase(const EAStringC &strText) const
{
    uint32_t uSize;
    uSize = GetLength();

    if (uSize != strText.GetLength())
    {
        //  Not the same size, can't be equal
        return (false);
    }

    if (m_pData == strText.m_pData)
    {
        //  This the same string, don't need to compare
        return (true);
    }

    if (APT_STRING_NOCASE_COMPARE(GetInternalBuffer(), strText.GetInternalBuffer()) == 0)
    {
        return (true);
    }

    return (false);
}

/**
 * Compares hash values first (cheap and cache-friendly); only falls back to a full no-case
 * comparison if the hashes happen to match, since a hash match makes the strings almost
 * certainly equal.
 */
bool EAStringC::EqualNoCaseHash(const EAStringC &strText) const
{
    //  We first test the pointers, as we need to read them either way
    //  And we reduce then the cache miss for the hash comparison
    if (m_pData == strText.m_pData)
    {
        //  They are the same
        return (true);
    }

    if (GetHashValue() != strText.GetHashValue())
    {
        //  Different hash, values, can't be the same
        return (false);
    }

    //  Hash values are the same! We do the full comparison without testing the size
    //  If the two hash are the same, the two strings are certainly the same
    //  Comparing the same size in 99% of the cases is a waste of cycles

    return (APT_STRING_NOCASE_COMPARE(GetBuffer(), strText.GetBuffer()) == 0);
}

/** Allocates a new buffer of (at least) uSize bytes plus header and trailing '\0', 4-byte aligned. */
void EAStringC::AllocateBuffer(const uint32_t uSize)
{
    APT_ASSERT(uSize > 0);

    //  We need iSize + the internal struct + the trailing '\0'
    uint32_t uAllocateSize = sizeof(StringDataC) + uSize + 1;

    uAllocateSize += 3;  //  Any allocator allocates with at least 4 bytes alignment
    uAllocateSize &= -4; //  We align to 4 bytes We take advantage of the padding here

    APT_ASSERT(uAllocateSize <= 0xffff);
    if (uAllocateSize > 0xffff)
    {
        uAllocateSize = 0xffff;
    }

    m_pData = (DebugDataC *)APT_MALLOC_BLOCK_TRACKER(AptNativeStringBuffers, uAllocateSize);

    SetInternalRefCount(1); //  One reference at creation
    SetInternalMaxSize(uAllocateSize - sizeof(StringDataC) - 1);
}

/** Computes a case-insensitive FNV-1a-style hash of pText, remapped away from 0 (used as a sentinel). */
uint16_t EAStringC::CalculateHashValue(const char *const pText)
{
    uint32_t uHash = 0x811c9dc5; //  seed
    int32_t c;
    const char *pStr = pText;

    while ((c = *pStr++) != 0)
    {
        if ((c <= 'Z') && (c >= 'A'))
        {
            c += 32;
        }

        uHash ^= c;
        uHash *= 0x1000193;
    }

    if (static_cast<uint16_t>(uHash) == 0)
    {
        return (ZERO_HASH); //  Value used if the hash-value is equal to zero!
    }
    return static_cast<uint16_t>(uHash);
}

/** @return the byte size of the UTF8 character starting at pBuffer (1-4). */
int32_t EAStringC::UTF8_GetCharacterSize(const char *const pBuffer)
{
    uint8_t cChar0;

#if defined(APT_DEBUG)
    UTF8_GetCharacter(pBuffer); //  Only to check if the iCharacter is valid
#endif

    cChar0 = static_cast<uint8_t>(*pBuffer);
    APT_ASSERT((cChar0 != 0xFE) && (cChar0 != 0xFF)); //  No byte can contain 0xFE or 0xFF
    if (cChar0 <= 127)
    {
        return (1);
    }
    else
    {
        if ((cChar0 & 0xE0) == 0xC0)
        {
            return (2);
        }
        else if ((cChar0 & 0xF0) == 0xE0)
        {
            return (3);
        }
        else if ((cChar0 & 0xC0) == 0xC0)
        {
            //  We handle the unicode but not UCS-4
            return (4);
        }
        else
        {
            // badly-encoded string; try to handle gracefully
            APT_ASSERT((cChar0 & 0xC0) == 0xC0);
            return 1;
        }
    }
}

/** @return the UTF8 byte size (1-4) that iCharacter would encode to. */
int32_t EAStringC::UTF8_GetCharacterSize(const int32_t iCharacter)
{
    APT_ASSERT(UTF8_IsValid(iCharacter));
    if (iCharacter < 0x80)
    {
        return (1);
    }
    else if (iCharacter < 0x0800)
    {
        return (2);
    }
    else if (iCharacter < 0x10000)
    {
        return (3);
    }
    else
    {
        APT_ASSERT(iCharacter < 00200000);
        //  We handle the unicode but not UCS-4
        //  See here: http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
        //  This website explains why the max size is 4 and not 6
        return (4);
    }
}

/** @return the byte position of the iIndex-th UTF8 character in pBuffer, or NULL if iIndex is past the end. */
const char *EAStringC::UTF8_GetBuffer(const char *const pBuffer, const int32_t iIndex)
{
    int32_t i;
    int32_t iCharacter         = 0;
    const char *pBufferToParse = pBuffer;

    for (i = 0; i < iIndex; ++i)
    {
        pBufferToParse = UTF8_ReadCharacter(pBufferToParse, &iCharacter);
        if (iCharacter == 0)
        {
            //  If iIndex is greater than the UTF8 size, we indicate it by a NULL pointer
            return (NULL);
        }
    }
    return (pBufferToParse);
}

/** @return the number of UTF8 characters in pBuffer. */
int32_t EAStringC::UTF8_GetSize(const char *const pBuffer)
{
    int32_t nSize              = 0;
    int32_t iCharacter         = 0;
    const char *pBufferToParse = pBuffer;
    for (;; ++nSize)
    {
        pBufferToParse = UTF8_ReadCharacter(pBufferToParse, &iCharacter);
        if (iCharacter == 0)
        {
            break;
        }
    }
    return (nSize);
}

/** @return the unicode codepoint of the UTF8 character starting at pBuffer. */
int32_t EAStringC::UTF8_GetCharacter(const char *const pBuffer)
{
    int32_t unicode = 0;
    uint8_t cChar0, cChar1, cChar2, cChar3;

    cChar0 = static_cast<unsigned char>(*pBuffer);
    APT_ASSERT((cChar0 != 0xFE) && (cChar0 != 0xFF)); //  No byte can contain 0xFE or 0xFF
    if (cChar0 <= 127)
    {
        unicode = cChar0;
    }
    else
    {
        if ((cChar0 & 0xE0) == 0xC0)
        {
            unicode = (cChar0 & 0x1F) << 6;

            cChar1 = *(pBuffer + 1);
            APT_ASSERT((cChar1 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= cChar1 & 0x3F;

            APT_ASSERT(unicode >= 0x80); //  Check that we have the smallest coding
            APT_ASSERT(unicode < 0x0800);
        }
        else if ((cChar0 & 0xF0) == 0xE0)
        {
            unicode = (cChar0 & 0xF) << 12;

            cChar1 = *(pBuffer + 1);
            APT_ASSERT((cChar1 & 0xC0) == 0x80) //  All subsequent code should be b10xxxxxx
            unicode |= (cChar1 & 0x3F) << 6;

            cChar2 = *(pBuffer + 2);
            APT_ASSERT((cChar2 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= cChar2 & 0x3F;

            APT_ASSERT(unicode >= 0x800); //  Check that we have the smallest coding
            APT_ASSERT(unicode < 0x10000);
        }
        else if ((cChar0 & 0xC0) == 0xC0)
        {
            APT_ASSERT((cChar0 & 0xf8) == 0xf0); //  We handle the unicode but not UCS-4
            unicode = (cChar0 & 0x7) << 18;

            cChar1 = *(pBuffer + 1);
            APT_ASSERT((cChar1 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= (cChar1 & 0x3F) << 12;

            cChar2 = *(pBuffer + 2);
            APT_ASSERT((cChar2 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= (cChar2 & 0x3F) << 6;

            cChar3 = *(pBuffer + 3);
            APT_ASSERT((cChar3 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= cChar3 & 0x3F;

            APT_ASSERT(unicode >= 0x10000);  //  Check that we have the smallest coding
            APT_ASSERT(unicode <= 0x10FFFF); //  Unicode and not ucs-4
        }
        else
        {
            // badly-encoded string; try to handle gracefully by returning the unconverted byte
            APT_ASSERT((cChar0 & 0xC0) == 0xC0); //  The top two bits need to be equal to 1
            unicode = cChar0;
        }
    }

    APT_ASSERT(UTF8_IsValid(unicode));
    return (unicode);
}

/** Encodes iCharacter as UTF8 into pBuffer and returns the buffer advanced past it. */
char *EAStringC::UTF8_SetCharacter(char *pBuffer, const int32_t iCharacter)
{
    //  This function work only if the current element and the next are of the same size
    APT_ASSERT(iCharacter != 0);
    APT_ASSERT(UTF8_GetCharacter(pBuffer) != 0);
    APT_ASSERT(UTF8_GetCharacterSize(pBuffer) == UTF8_GetCharacterSize(iCharacter));
    APT_ASSERT(UTF8_IsValid(iCharacter));

    // if iCharacter is ASCII, or if the character is invalid, then just copy the byte and move on.
    if (iCharacter < 0x80 || (UTF8_GetCharacterSize(pBuffer) != UTF8_GetCharacterSize(iCharacter)))
    {
        *pBuffer++ = static_cast<char>(iCharacter);
    }
    else if (iCharacter < 0x0800)
    {
        *pBuffer++ = (iCharacter >> 6) | 0xC0;
        *pBuffer++ = (iCharacter & 0x3F) | 0x80;
    }
    else if (iCharacter < 0x10000)
    {
        *pBuffer++ = (iCharacter >> 12) | 0xE0;
        *pBuffer++ = ((iCharacter >> 6) & 0x3F) | 0x80;
        *pBuffer++ = (iCharacter & 0x3F) | 0x80;
    }
    else
    {
        *pBuffer++ = (iCharacter >> 18) | 0xF0;
        *pBuffer++ = ((iCharacter >> 12) & 0x3F) | 0x80;
        *pBuffer++ = ((iCharacter >> 6) & 0x3F) | 0x80;
        *pBuffer++ = (iCharacter & 0x3F) | 0x80;
    }

    return pBuffer;
}

/** Replaces this string's content with a single UTF8-encoded character. */
void EAStringC::UTF8_SetOneCharacter(const int32_t iCharacter)
{
    char *pBuffer = GetInternalBuffer();
    if (iCharacter < 0x80)
    {
        *pBuffer       = static_cast<char>(iCharacter);
        *(pBuffer + 1) = '\0';
        SetInternalSize(1);
    }
    else if (iCharacter < 0x0800)
    {
        *(pBuffer + 0) = (iCharacter >> 6) | 0xC0;
        *(pBuffer + 1) = (iCharacter & 0x3F) | 0x80;
        *(pBuffer + 2) = '\0';
        SetInternalSize(2);
    }
    else if (iCharacter < 0x10000)
    {
        *(pBuffer + 0) = (iCharacter >> 12) | 0xE0;
        *(pBuffer + 1) = ((iCharacter >> 6) & 0x3F) | 0x80;
        *(pBuffer + 2) = (iCharacter & 0x3F) | 0x80;
        *(pBuffer + 3) = '\0';
        SetInternalSize(3);
    }
    else
    {
        *(pBuffer + 0) = (iCharacter >> 18) | 0xF0;
        *(pBuffer + 1) = ((iCharacter >> 12) & 0x3F) | 0x80;
        *(pBuffer + 2) = ((iCharacter >> 6) & 0x3F) | 0x80;
        *(pBuffer + 3) = (iCharacter & 0x3F) | 0x80;
        *(pBuffer + 4) = '\0';
        SetInternalSize(4);
    }
    InvalidateHashValue();
}

/** Reads one UTF8 character from pBuffer into *iCharacter, returning the buffer advanced past it. */
const char *EAStringC::UTF8_ReadCharacter(const char *const pBuffer, int32_t *const iCharacter)
{
    int32_t unicode = 0;
    uint8_t cChar0, cChar1, cChar2, cChar3;
    const char *pCurrentBuffer;

    cChar0 = static_cast<unsigned char>(*pBuffer);
    APT_ASSERT((cChar0 != 0xFE) && (cChar0 != 0xFF)); //  No byte can contain 0xFE or 0xFF
    if (cChar0 <= 127)
    {
        unicode        = cChar0;
        pCurrentBuffer = pBuffer + 1;
    }
    else
    {
        if ((cChar0 & 0xE0) == 0xC0)
        {
            unicode = (cChar0 & 0x1F) << 6;

            cChar1 = *(pBuffer + 1);
            APT_ASSERT((cChar1 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= cChar1 & 0x3F;

            APT_ASSERT(unicode >= 0x80); //  Check that we have the smallest coding
            APT_ASSERT(unicode < 0x0800);

            pCurrentBuffer = pBuffer + 2;
        }
        else if ((cChar0 & 0xF0) == 0xE0)
        {
            unicode = (cChar0 & 0xF) << 12;

            cChar1 = *(pBuffer + 1);
            APT_ASSERT((cChar1 & 0xC0) == 0x80) //  All subsequent code should be b10xxxxxx
            unicode |= (cChar1 & 0x3F) << 6;

            cChar2 = *(pBuffer + 2);
            APT_ASSERT((cChar2 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= cChar2 & 0x3F;

            APT_ASSERT(unicode >= 0x800); //  Check that we have the smallest coding
            APT_ASSERT(unicode < 0x10000);

            pCurrentBuffer = pBuffer + 3;
        }
        else if ((cChar0 & 0xC0) == 0xC0)
        {
            APT_ASSERT((cChar0 & 0xf8) == 0xf0); //  We handle the unicode but not UCS-4
            unicode = (cChar0 & 0x7) << 18;

            cChar1 = *(pBuffer + 1);
            APT_ASSERT((cChar1 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= (cChar1 & 0x3F) << 12;

            cChar2 = *(pBuffer + 2);
            APT_ASSERT((cChar2 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= (cChar2 & 0x3F) << 6;

            cChar3 = *(pBuffer + 3);
            APT_ASSERT((cChar3 & 0xC0) == 0x80); //  All subsequent code should be b10xxxxxx
            unicode |= cChar3 & 0x3F;

            APT_ASSERT(unicode >= 0x10000);  //  Check that we have the smallest coding
            APT_ASSERT(unicode <= 0x10FFFF); //  Unicode and not ucs-4

            pCurrentBuffer = pBuffer + 4;
        }
        else
        {
            // badly-encoded string; try to handle gracefully by returning the unconverted byte
            APT_ASSERT((cChar0 & 0xC0) == 0xC0); //  The top two bits need to be equal to 1
            unicode        = cChar0;
            pCurrentBuffer = pBuffer + 1;
        }
    }

    APT_ASSERT(UTF8_IsValid(unicode));
    *iCharacter = unicode;

    return (pCurrentBuffer);
}

/** Copies strText's content into this string, without sharing its buffer. */
EAStringC &EAStringC::Duplicate(const EAStringC &strText)
{
    uint32_t uLength;
    char *pStrBuffer;

    uLength = strText.GetLength();
    ReserveSize(uLength);
    pStrBuffer = GetInternalBuffer();
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    memcpy(pStrBuffer, strText.GetBuffer(), uLength);
    pStrBuffer[uLength] = '\0';
    SetInternalSize(uLength);
    m_pData->m_uHash = strText.m_pData->m_uHash; //  Duplicate so use the same hash-value

    return (*this);
}

/** Grows the buffer to at least uSize bytes, preserving the existing content (clamped to uSize). */
void EAStringC::ReserveSize(const uint32_t uSize)
{
    uint32_t uCopySize;
    uCopySize = GetLength(); //  Keep the whole existing string
    if (uCopySize > uSize)
    {
        uCopySize = uSize; //  Clamp to the reserve string asked
    }
    ChangeBuffer(uSize, 0, uCopySize, CB_PUSH_ZERO, uCopySize);
}

bool EAStringC::IsEnoughSize(const uint32_t uSize) const
{
    if (uSize < GetInternalMaxSize())
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

/** Appends up to uSize bytes of pStrText (stopping earlier at a '\0'). */
EAStringC &EAStringC::Append(const char *const pStrText, const uint32_t uSize)
{
    const char *pStrCount;
    char *pStrBuffer;
    uint32_t uLength1, uLength2;

    pStrCount = pStrText;
    for (uLength2 = 0; uLength2 < uSize; ++uLength2)
    {
        if (*pStrCount++ == 0)
        {
            break;
        }
    }
    if (uLength2 == 0)
    {
        return (*this);
    }
    //  Now we got the min value between the size of the string and the asked size
    //  We didn't call strlen(pStrText) because if the input text is very very long with a small iSize, we will waste a lot of time

    uLength1 = GetLength();

    ChangeBuffer(uLength1 + uLength2, 0, uLength1, CB_PUSH_ZERO, uLength1 + uLength2);

    pStrBuffer = GetInternalBuffer();
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    pStrBuffer += uLength1;

    memcpy(pStrBuffer, pStrText, uLength2); //  The trailing zero is copied during the change buffer

    return (*this);
}

void EAStringC::AppendFormat(const char *const pStrFormat, ...)
{
    APT_ASSERT(pStrFormat != NULL);

    uint32_t uSize;
    uint32_t uBasicSize;
    int32_t iFilled;
    char *pStrBuffer;
    va_list Args;

    uSize      = 4 * static_cast<uint32_t>(strlen(pStrFormat));
    uBasicSize = GetLength();

    va_start(Args, pStrFormat); //  Initialize variable arguments.

    for (;;)
    {
        //  Reserve enough size, reset the buffer in the same time
        ChangeBuffer(uBasicSize + uSize, 0, 0, CB_NO_PUSH_ZERO, 0);

        pStrBuffer = GetInternalBuffer();
        APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");

#if defined(_MSC_VER)
        iFilled = _vsnprintf(pStrBuffer + uBasicSize, GetInternalMaxSize() - uBasicSize, pStrFormat, Args);
#else
        iFilled = vsnprintf(pStrBuffer + uBasicSize, GetInternalMaxSize() - uBasicSize, pStrFormat, Args);
#endif
        if (iFilled >= 0)
        {
            //  Succesfully fill
            break;
        }

        //  The size is not enough, allocate more memory
        uSize *= 2;
    }

    *(pStrBuffer + uBasicSize + iFilled) = '\0'; //  Put the trailing '\0' to be sure
    SetInternalSize(uBasicSize + iFilled);       //  Put the returned size
    InvalidateHashValue();
    va_end(Args);
}

void EAStringC::Format(const char *const pStrFormat, ...)
{
    va_list Args;
    APT_ASSERT(pStrFormat != NULL);
    va_start(Args, pStrFormat); //  Initialize variable arguments.

    vsFormat(pStrFormat, Args);

    va_end(Args);
}

void EAStringC::vsFormat(const char *const pStrFormat, va_list Args)
{
    APT_ASSERT(pStrFormat != NULL);

    int32_t iSize;
    int32_t iFilled;
    char *pStrBuffer;

    iSize = 4 * static_cast<uint32_t>(strlen(pStrFormat));

    for (;;)
    {
        ChangeBuffer(iSize, 0, 0, CB_NO_PUSH_ZERO, 0); //  Reserve enough size, reset the buffer in the same time

        pStrBuffer = GetInternalBuffer();
        APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");

#if defined(_MSC_VER)
        iFilled = _vsnprintf(pStrBuffer, GetInternalMaxSize(), pStrFormat, Args);
#else
        iFilled = vsnprintf(pStrBuffer, GetInternalMaxSize(), pStrFormat, Args);
#endif
        if (iFilled >= 0)
        {
            //  Succesfully fill
            break;
        }

        //  The size is not enough, allocate more memory
        iSize *= 2;
    }

    *(pStrBuffer + iFilled) = '\0'; //  Put the trailing '\0' to be sure
    SetInternalSize(iFilled);       //  Put the returned size
    InvalidateHashValue();
    va_end(Args);
}

int32_t EAStringC::Find(const char *const pStrText, const int32_t iStart, bool ignoreCase) const
{
    APT_ASSERT(pStrText != NULL);

    const char *pFound;
    int32_t iPos;

    if (iStart >= (int32_t)GetLength())
    {
        //  Over the size of the string, not found
        return (-1);
    }
    iPos = iStart;
    if (iPos < 0)
    {
        //  Before the string, start from the begining
        iPos = 0;
    }

    pFound = StrstrCaseOption(GetBuffer() + iPos, pStrText, ignoreCase);
    if (pFound)
    {
        return (pFound - GetBuffer());
    }
    else
    {
        //  Not found
        return (-1);
    }
}

int32_t EAStringC::Find(const char cChar, const int32_t iStart) const
{
    const char *pFound;
    int32_t iPos;

    if (iStart >= (int32_t)GetLength())
    {
        //  Over the size of the string, not found
        return (-1);
    }
    iPos = iStart;
    if (iPos < 0)
    {
        //  Before the string, start from the begining
        iPos = 0;
    }

    pFound = strchr(GetBuffer() + iPos, cChar);
    if (pFound)
    {
        return (pFound - GetBuffer());
    }
    else
    {
        //  Not found
        return (-1);
    }
}

/** @return the index of the last occurrence of pSubstring at or before iLastIdx, or -1 if not found. */
int32_t EAStringC::LastIndexOf(const char *const pSubstring, const int32_t iLastIdx) const
{
    // Start at the last possible position in the string
    int32_t iIndex = static_cast<int32_t>(GetLength() - strlen(pSubstring));

    if (iLastIdx < iIndex)
    {
        iIndex = iLastIdx;
    }

    while (iIndex >= 0)
    {
        const char *pInStr = GetBuffer() + iIndex;
        const char *pInSub = pSubstring;
        bool bMatch        = true;
        // Compare characters until a mismatch is found or until the end of the substring
        while (bMatch && *pInSub)
        {
            bMatch = (*(pInStr++) == *(pInSub++)); // bMatch becomes false if a mismatch is reached
        }
        if (bMatch)
        {
            // The end of the substring was reached without differences: a match was found, so return the index
            return AsciiIndexToUTF8Index(iIndex);
        }
        // Go backwards looking for a match
        --iIndex;
    }

    // No match
    return -1;
}

/**
 * Given an index into this string as though it were ASCII, returns the equivalent UTF8 character
 * index. @p utf8Start / @p asciiStart let the caller provide an already-known matching pair of
 * indices to start counting from, instead of the beginning of the string.
 */
int32_t EAStringC::AsciiIndexToUTF8Index(const int32_t asciiIndex, const int32_t utf8Start, const int32_t asciiStart) const
{
    int32_t utf8Index   = utf8Start;
    const char *curChar = c_str() + asciiStart;
    const char *end     = c_str() + asciiIndex;
    while (curChar < end)
    {
        curChar += UTF8_GetCharacterSize(curChar);
        ++utf8Index;
    }
    APT_ASSERT(curChar == end); //  check that we ended in the right spot (and not like one byte past it)
    return (utf8Index);
}

/** @note not implemented upstream; nobody calls this today. */
int32_t EAStringC::FindOneOf(const char *const pStrText) const
{
    APT_ASSERT(false); //  Not implemented...
                       /*
                       //  Nobody is using it. Uncomment this if the function is needed
                           APT_ASSERT(pStrText != NULL);
                           const char *pStart = GetBuffer();
                           for(const char *pCurrent = pStart ; *pCurrent != '\0'; ++pCurrent)
                           {
                               for(const char *pChar = pStrText; *pChar != '\0'; ++pChar)
                               {
                                   if (*pCurrent == *pChar)
                                   {
                                       return pCurrent - pStart;
                                   }
                               }
                           }
                       */
    return -1;
}

/** @note not implemented upstream; nobody calls this today. */
int32_t EAStringC::ReverseFind(const char cChar) const
{
    APT_ASSERT(false); //  Not implemented...
    return (0);
    /*
    //  Nobody is using it. Uncomment this if the function is needed
        const char *pStart = GetBuffer();
        const char *pFound = strrchr(pStart, cChar);
        return  pFound? pFound - pStart : -1;
    */
}

/** Deletes iCount characters starting at iIndex; both are clamped to the string's bounds. @return the new length. */
int32_t EAStringC::Delete(const int32_t iIndex, const int32_t iCount)
{
    int32_t iClampedIndex = iIndex;
    int32_t iClampedEnd   = iIndex + iCount;
    int32_t iOldSize;
    int32_t newSize;

    if (iCount <= 0)
    {
        Clear();
        return (0);
    }
    if (iClampedEnd <= 0)
    {
        Clear();
        return (0);
    }

    if (iClampedIndex < 0)
    {
        iClampedIndex = 0;
    }

    iOldSize = GetLength();
    if (iClampedEnd >= iOldSize)
    {
        iClampedEnd = iOldSize;
    }

    if (iClampedIndex == 0)
    {
        //  We remove something at the beginning
        newSize = iOldSize - iClampedEnd;
        ChangeBuffer(newSize, iClampedEnd, newSize, CB_PUSH_ZERO, newSize);
        return (newSize);
    }
    if (iClampedEnd == iOldSize)
    {
        //  We remove something at the end
        ChangeBuffer(iClampedIndex, 0, iClampedIndex, CB_PUSH_ZERO, iClampedIndex);
        return (iClampedIndex);
    }

    char *pOldBuffer = GetInternalBuffer();
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    newSize = iClampedIndex + (iOldSize - iClampedEnd);
    ChangeBuffer(newSize, 0, iClampedIndex, CB_NO_PUSH_ZERO, newSize);
    //  As the delete reduce the string, we should not have reallocation (except if shared buffer)
    //  Either way, the old buffer should be still there and accessible (see here for multi-threaded issues)
    //  +1 to copy the trailing zero
    memcpy(GetInternalBuffer() + iClampedIndex, pOldBuffer + iClampedEnd, (iOldSize - iClampedEnd) + 1);
    return (newSize);
}

int32_t EAStringC::Remove(const char cRemove)
{
    char strText[2] = "*";
    strText[0]      = cRemove;

    return (Replace(strText, ""));
}

/** @note not implemented upstream; nobody calls this today. */
int32_t EAStringC::Insert(const int32_t iIndex, const char *const pStrText)
{
    APT_ASSERT(pStrText != NULL);
    APT_ASSERT(false); //  Not implemented...
    return (0);
}

/** @note not implemented upstream; nobody calls this today. */
int32_t EAStringC::Insert(const int32_t iIndex, const char cChar)
{
    APT_ASSERT(false); //  Not implemented...
    return (0);
}

/** Replaces every occurrence of pStrOld with pStrNew. @return the number of instances replaced. */
int32_t EAStringC::Replace(const char *const pStrOld, const char *const pStrNew, bool ignoreCase)
{
    const char *pStrOccurence;
    const char *pStrStart;
    uint32_t uSize;
    uint32_t uSizeOld, uSizeNew;
    int32_t iInstancesNbr;

    APT_ASSERT(pStrOld != NULL);
    APT_ASSERT(pStrNew != NULL);

    uSizeOld = static_cast<uint32_t>(strlen(pStrOld));
    if (uSizeOld == 0)
    {
        return (0);
    }
    uSizeNew = static_cast<uint32_t>(strlen(pStrNew));

    //  First pass: Determine the needed size
    iInstancesNbr = 0;
    pStrStart     = GetBuffer();
    for (;;)
    {
        pStrOccurence = reinterpret_cast<const char *>(StrstrCaseOption(pStrStart, pStrOld, ignoreCase));
        if (pStrOccurence == NULL)
        {
            break;
        }
        //  We found an instance
        ++iInstancesNbr;
        pStrStart = pStrOccurence + uSizeOld; //  Push the new search index after the instance
    }

    if (iInstancesNbr == 0)
    {
        //  No replacing to do
        return (0);
    }

    uSize = GetLength();
    uSize += (uSizeNew - uSizeOld) * iInstancesNbr; //  We got the new size now

    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");

    //  Second pass: Do the replacement

    //  Now do the replacement, we need another buffer because of the overlapping
    //  For a better implementation we need to make the copy in the reverse order so we don't need another buffer
    //  But for the moment, use this naive implementation

    EAStringC strBuffer(uSize); //  Reserve the string with at least the needed size
    char *pStrBuffer;
    char *pStrOldBuffer;
    int32_t i;
    int32_t iShift;

    pStrStart  = GetBuffer();
    pStrBuffer = pStrOldBuffer = strBuffer.GetInternalBuffer();
    for (i = 0; i < iInstancesNbr; ++i)
    {
        pStrOccurence = reinterpret_cast<const char *>(StrstrCaseOption(pStrStart, pStrOld, ignoreCase));
        APT_ASSERT(pStrOccurence != NULL); //  Must be synchronized with the first pass

        iShift = pStrOccurence - pStrStart;
        if (iShift)
        {
            memcpy(pStrBuffer, pStrStart, iShift); //  Copy the string before the instance
            pStrBuffer += iShift;
        }

        pStrStart = pStrOccurence + uSizeOld; //  Skip on the old buffer

        memcpy(pStrBuffer, pStrNew, uSizeNew); //  Copy the replaced instance
        pStrBuffer += uSizeNew;
    }

    //  We need to copy the end of the buffer (after the last replace)
    iShift = uSize - (pStrBuffer - pStrOldBuffer);
    if (iShift)
    {
        memcpy(pStrBuffer, pStrStart, iShift); //  Copy the string before the instance
        pStrBuffer += iShift;
    }

    *pStrBuffer = '\0'; //  Put the final '\0' and the size
    strBuffer.SetInternalSize(uSize);
    strBuffer.InvalidateHashValue();

    (*this) = strBuffer; //  Copy the replaced string, free the old string
                         //  Remember that the copy is "free"
    return (iInstancesNbr);
}

int32_t EAStringC::Replace(const char cOld, const char cNew)
{
    const char *pStrStart;
    char *pStrBuffer;
    const char *pStrInstance;
    int32_t iInstancesNbr;

    APT_ASSERT(cOld != '\0'); //  Can't use this function on a null character
    APT_ASSERT(cNew != '\0'); //  Because this change the size in a complex manner

    pStrStart     = GetBuffer();
    iInstancesNbr = 0;
    for (;;)
    {
        pStrInstance = reinterpret_cast<const char *>(strchr(pStrStart, cOld));
        if (pStrInstance == NULL)
        {
            break;
        }

        pStrBuffer  = (char *)pStrInstance;
        *pStrBuffer = cNew;

        pStrStart = pStrInstance + 1;
    }
    return (iInstancesNbr);
}

EAStringC EAStringC::Left(const int32_t iCount) const
{
    if (iCount <= 0)
    {
        return (EAStringC(EMPTY_STRING));
    }
    if ((uint32_t)iCount >= GetLength())
    {
        //  Extract all the string
        return (*this);
    }

    EAStringC strText(*this);
    //  This duplicate the buffer and the possibly created buffer size is closer to iValue size
    strText.ChangeBuffer(iCount, 0, iCount, CB_PUSH_ZERO, iCount);
    return (strText);
}

EAStringC EAStringC::Right(const int32_t iCount) const
{
    int32_t iOffset;
    if (iCount <= 0)
    {
        return (EAStringC(EMPTY_STRING));
    }
    iOffset = GetLength() - iCount;
    if (iOffset <= 0)
    {
        //  Extract all the string
        return (*this);
    }

    EAStringC strText(*this);
    //  This duplicate the buffer and the possibly created buffer size is closer to iValue size
    strText.ChangeBuffer(iCount, iOffset, iCount, CB_PUSH_ZERO, iCount);
    return (strText);
}

EAStringC EAStringC::Mid(const int32_t iFirst) const
{
    int32_t iSize;

    if (iFirst <= 0)
    {
        return (*this);
    }

    iSize = GetLength() - iFirst;
    if (iSize <= 0)
    {
        return (EAStringC(EMPTY_STRING));
    }

    EAStringC strText(*this);
    //  This duplicate the buffer and the possibly created buffer size is closer to iValue size
    strText.ChangeBuffer(iSize, iFirst, iSize, CB_PUSH_ZERO, iSize);
    return (strText);
}

EAStringC EAStringC::Mid(const int32_t iFirst, const int32_t iCount) const
{
    int32_t iSize;
    int32_t iClampedFirst = iFirst;
    int32_t iClampedCount = iCount;

    if (iClampedFirst < 0)
    {
        iClampedCount += iClampedFirst; //  To resynchronize the number of elements to count
        iClampedFirst = 0;
    }

    if (iClampedCount <= 0)
    {
        return (EAStringC(EMPTY_STRING));
    }

    iSize = GetLength() - iClampedFirst;
    if (iSize <= 0)
    {
        return (EAStringC(EMPTY_STRING));
    }

    if (iClampedCount < iSize)
    {
        iSize = iClampedCount;
    }

    EAStringC strText(*this);
    //  This duplicate the buffer and the possibly created buffer size is closer to iValue size
    strText.ChangeBuffer(iSize, iFirst, iSize, CB_PUSH_ZERO, iSize);
    return (strText);
}

EAStringC &EAStringC::MakeLower()
{
    int nLength = GetLength();
    ChangeBuffer(nLength, 0, nLength, CB_PUSH_ZERO, nLength); //  Create a new instance of this string if multi-referenced
#if defined(_MSC_VER)
    _strlwr(GetInternalBuffer());
#else
    int32_t i;
    char *pBuffer = GetInternalBuffer();
    for (i = 0; i < nLength; ++i, ++pBuffer)
    {
        *pBuffer = static_cast<char>(tolower(static_cast<unsigned char>(*pBuffer)));
    }
#endif
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    //  MakeLower doesn't change the hash value as the hash is case insensitive
    return (*this);
}

EAStringC &EAStringC::MakeUpper()
{
    int nLength = GetLength();
    ChangeBuffer(nLength, 0, nLength, CB_PUSH_ZERO, nLength); //  Create a new instance of this string if multi-referenced
#if defined(_MSC_VER)
    _strupr(GetInternalBuffer());
#else
    int32_t i;
    char *pBuffer = GetInternalBuffer();
    for (i = 0; i < nLength; ++i, ++pBuffer)
    {
        *pBuffer = static_cast<char>(toupper(static_cast<unsigned char>(*pBuffer)));
    }
#endif
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    //  MakeUpper doesn't change the hash value as the hash is case insensitive
    return (*this);
}

EAStringC &EAStringC::MakeReverse()
{
    int nLength = GetLength();
    ChangeBuffer(nLength, 0, nLength, CB_PUSH_ZERO, nLength); //  Create a new instance of this string if multi-referenced

    char *pBuffer;
    char *pEndBuffer;
    char cTemp;
    uint32_t uSize;

    uSize = GetLength();
    if (uSize <= 1)
    {
        //  Less than one character, nothing to reverse
        return (*this);
    }

    pBuffer = GetInternalBuffer();
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    pEndBuffer = (char *)pBuffer + uSize - 1;

    while (pBuffer < pEndBuffer)
    {
        cTemp       = *pBuffer;
        *pBuffer    = *pEndBuffer;
        *pEndBuffer = cTemp;

        ++pBuffer;
        --pEndBuffer;
    }

    //  Reverse change the hash-value
    InvalidateHashValue();

    return (*this);
}

EAStringC &EAStringC::TrimLeft(const char *const pStrText)
{
    APT_ASSERT(pStrText != NULL);

    const char *pBuffer;
    char cChar;
    uint32_t i, uSize;

    uSize   = GetLength();
    pBuffer = GetBuffer();
    for (i = 0; i < uSize; ++i)
    {
        cChar = *pBuffer++;
        if (strchr(pStrText, cChar) == 0)
        {
            //  First different char
            break;
        }
    }

    EAStringC strText;
    strText = Mid(i);
    *this   = strText;
    return (*this);
}

EAStringC &EAStringC::TrimRight(const char *const pStrText)
{
    APT_ASSERT(pStrText != NULL);

    const char *pBuffer;
    char cChar;
    uint32_t i, uSize;

    uSize   = GetLength();
    pBuffer = GetBuffer() + uSize - 1;
    for (i = 0; i < uSize; ++i)
    {
        cChar = *pBuffer--;
        if (strchr(pStrText, cChar) == 0)
        {
            //  First different char
            break;
        }
    }

    EAStringC strText;
    strText = Left(uSize - i);
    *this   = strText;
    return (*this);
}

EAStringC &EAStringC::Trim(const char *const pStrText)
{
    TrimLeft(pStrText);
    return (TrimRight(pStrText));
}

/** @return true if this string starts with pStrText. */
bool EAStringC::StartWith(const char *const pStrText) const
{
    APT_ASSERT(pStrText != NULL);

    uint32_t uLength;
    uLength = static_cast<uint32_t>(strlen(pStrText));
    if (GetLength() < uLength)
    {
        return (false);
    }

    if (memcmp(GetBuffer(), pStrText, uLength) == 0)
    {
        return (true);
    }
    return (false);
}

/** @return true if this string starts with pStrText, ignoring case. */
bool EAStringC::StartWithIgnoreCase(const char *const pStrText) const
{
    APT_ASSERT(pStrText != NULL);

    uint32_t uLength;
    uLength = static_cast<uint32_t>(strlen(pStrText));
    if (GetLength() < uLength)
    {
        return (false);
    }

    if (APT_STRING_NOCASE_COMPARE(GetBuffer(), pStrText) == 0)
    {
        return (true);
    }
    return (false);
}

/** @return true if this string ends with pStrText. */
bool EAStringC::EndWith(const char *const pStrText) const
{
    APT_ASSERT(pStrText != NULL);

    uint32_t uLength1;
    uint32_t uLength2;

    uLength1 = GetLength();
    uLength2 = static_cast<uint32_t>(strlen(pStrText));

    if (uLength1 < uLength2)
    {
        return (false);
    }

    if (memcmp(GetBuffer() + uLength1 - uLength2, pStrText, uLength2) == 0)
    {
        return (true);
    }
    return (false);
}

/** @return true if this string ends with pStrText, ignoring case. */
bool EAStringC::EndWithIgnoreCase(const char *const pStrText) const
{
    APT_ASSERT(pStrText != NULL);

    uint32_t uLength1;
    uint32_t uLength2;

    uLength1 = GetLength();
    uLength2 = static_cast<uint32_t>(strlen(pStrText));

    if (uLength1 < uLength2)
    {
        return (false);
    }

    if (APT_STRING_NOCASE_COMPARE(GetBuffer() + uLength1 - uLength2, pStrText) == 0)
    {
        return (true);
    }
    return (false);
}

/** Removes pStrText from the start of this string if present. @return true if it was there and removed. */
bool EAStringC::StartWithRemove(const char *const pStrText)
{
    APT_ASSERT(pStrText != NULL);

    uint32_t uLength;
    uLength = static_cast<uint32_t>(strlen(pStrText));
    if (GetLength() < uLength)
    {
        return (false);
    }

    if (memcmp(GetBuffer(), pStrText, uLength) == 0)
    {
        *this = Mid(uLength);
        return (true);
    }
    return (false);
}

/** Removes pStrText from the start of this string if present, ignoring case. @return true if it was there and removed. */
bool EAStringC::StartWithRemoveIgnoreCase(const char *const pStrText)
{
    APT_ASSERT(pStrText != NULL);

    uint32_t uLength;
    uLength = static_cast<uint32_t>(strlen(pStrText));
    if (GetLength() < uLength)
    {
        return (false);
    }

    if (APT_STRING_NOCASE_COMPARE(GetBuffer(), pStrText) == 0)
    {
        *this = Mid(uLength);
        return (true);
    }
    return (false);
}

/** Removes pStrText from the end of this string if present. @return true if it was there and removed. */
bool EAStringC::EndWithRemove(const char *const pStrText)
{
    APT_ASSERT(pStrText != NULL);

    uint32_t uLength1;
    uint32_t uLength2;

    uLength1 = GetLength();
    uLength2 = static_cast<uint32_t>(strlen(pStrText));

    if (uLength1 < uLength2)
    {
        return (false);
    }

    if (memcmp(GetBuffer() + uLength1 - uLength2, pStrText, uLength2) == 0)
    {
        *this = Left(uLength1 - uLength2);
        return (true);
    }
    return (false);
}

/** Removes pStrText from the end of this string if present, ignoring case. @return true if it was there and removed. */
bool EAStringC::EndWithRemoveIgnoreCase(const char *const pStrText)
{
    APT_ASSERT(pStrText != NULL);

    uint32_t uLength1;
    uint32_t uLength2;

    uLength1 = GetLength();
    uLength2 = static_cast<uint32_t>(strlen(pStrText));

    if (uLength1 < uLength2)
    {
        return (false);
    }

    if (APT_STRING_NOCASE_COMPARE(GetBuffer() + uLength1 - uLength2, pStrText) == 0)
    {
        *this = Left(uLength1 - uLength2);
        return (true);
    }
    return (false);
}

const char *EAStringC::UTF8_GetBuffer(const int32_t iIndex)
{
    return (UTF8_GetBuffer(c_str(), iIndex));
}

int32_t EAStringC::UTF8_CharAt(const int32_t iIndex) const
{
    if (iIndex < 0)
    {
        return (-1);
    }

    const char *pBuffer;
    pBuffer = UTF8_GetBuffer(c_str(), iIndex);
    if (pBuffer == NULL)
    {
        //  After the end of the string
        return (-1);
    }
    return (UTF8_GetCharacter(pBuffer));
}

int32_t EAStringC::UTF8_Size() const
{
    return (UTF8_GetSize(c_str()));
}

EAStringC EAStringC::UTF8_Mid(const int32_t iFirst) const
{
    const char *pBufferStart;
    const char *pBufferFirst;

    int32_t iClampedFirst = iFirst;
    if (iClampedFirst < 0)
    {
        iClampedFirst = 0;
    }

    pBufferStart = c_str();
    pBufferFirst = UTF8_GetBuffer(pBufferStart, iClampedFirst);
    if (pBufferFirst == NULL)
    {
        //  First is after the end of the string
        return (EAStringC(EMPTY_STRING));
    }
    return (Mid(pBufferFirst - pBufferStart));
}

EAStringC EAStringC::UTF8_Mid(const int32_t iFirst, const int32_t iCount) const
{
    const char *pBufferStart;
    const char *pBufferFirst;
    const char *pBufferCount;

    int32_t iClampedFirst = iFirst;
    int32_t iClampedCount = iCount;
    if (iClampedFirst < 0)
    {
        iClampedCount += -iClampedFirst; //  Reduce the count accordingly
        iClampedFirst = 0;
    }
    if (iClampedCount <= 0)
    {
        return (EAStringC(EMPTY_STRING));
    }

    pBufferStart = c_str();
    pBufferFirst = UTF8_GetBuffer(pBufferStart, iClampedFirst);
    if (pBufferFirst == NULL)
    {
        //  First is after the end of the string
        return (EAStringC(EMPTY_STRING));
    }
    pBufferCount = UTF8_GetBuffer(pBufferFirst, iClampedCount);
    if (pBufferCount == NULL)
    {
        //  Count is after the end of the string
        return (Mid(pBufferFirst - pBufferStart));
    }

    return (Mid(pBufferFirst - pBufferStart, pBufferCount - pBufferFirst));
}

EAStringC &EAStringC::UTF8_Append(const char *const pStrText, const int32_t iSize)
{
    int32_t i;
    int32_t iCharacter  = 0;
    const char *pBuffer = pStrText;
    for (i = 0; i < iSize; ++i)
    {
        pBuffer = UTF8_ReadCharacter(pBuffer, &iCharacter);
        if (iCharacter == 0)
        {
            break;
        }
    }

    return (Append(pStrText, pBuffer - pStrText));
}

int32_t EAStringC::UTF8_Find(const char *const pStrText, const int32_t iStart) const
{
    const char *pBuffer;
    const char *pBufferStart = c_str();
    pBuffer                  = UTF8_GetBuffer(pBufferStart, iStart);
    if (pBuffer == NULL)
    {
        //  The start is after the end, can't find it
        return (-1);
    }

    int32_t iStartASCII = pBuffer - pBufferStart;
    int32_t iFoundASCII;

    //  We can use the standard search even in UTF8
    //  Because there is no ambiguity between ASCII and UTF8!
    iFoundASCII = Find(pStrText, iStartASCII);
    if (iFoundASCII < 0)
    {
        return (-1);
    }

    return AsciiIndexToUTF8Index(iFoundASCII, iStart, iStartASCII);
}

/** Lowercases the string, treating it as UTF8 (handles the Latin-1 range beyond plain ASCII). */
EAStringC &EAStringC::UTF8_MakeLower()
{
    // Lowercase conversion adds 32 to all characters between 192 and 223 except 215 and 223
    static unsigned char conversion[64] =
        {
            224,
            225,
            226,
            227,
            228,
            229,
            230,
            231,
            232,
            233,
            234,
            235,
            236,
            237,
            238,
            239,
            240,
            241,
            242,
            243,
            244,
            245,
            246,
            215,
            248,
            249,
            250,
            251,
            252,
            253,
            254,
            223,
            224,
            225,
            226,
            227,
            228,
            229,
            230,
            231,
            232,
            233,
            234,
            235,
            236,
            237,
            238,
            239,
            240,
            241,
            242,
            243,
            244,
            245,
            246,
            247,
            248,
            249,
            250,
            251,
            252,
            253,
            254,
            255,
        };

    int nLength = GetLength();
    ChangeBuffer(nLength, 0, nLength, CB_PUSH_ZERO, nLength); //  Create a new instance of this string if multi-referenced

    int32_t iCharacter     = 0;
    const char *pSrcBuffer = GetInternalBuffer();
    char *pDestBuffer      = (char *)pSrcBuffer;
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    for (;;)
    {
        pSrcBuffer = UTF8_ReadCharacter(pSrcBuffer, &iCharacter);
        if (iCharacter == 0)
        {
            break;
        }
        if (iCharacter < 128)
        {
            iCharacter = tolower(iCharacter);
        }
        else if (iCharacter >= 192 && iCharacter < 256)
        {
            // Sony's CType.h does not handle special characters correctly
            iCharacter = (int)conversion[iCharacter - 192];
        }
        pDestBuffer = UTF8_SetCharacter(pDestBuffer, iCharacter);
    }
    //  UTF8_MakeLower changes the hash value as it's not in ASCII
    return (*this);
}

/** Uppercases the string, treating it as UTF8 (handles the Latin-1 range beyond plain ASCII). */
EAStringC &EAStringC::UTF8_MakeUpper()
{
    // Uppercase conversion subtracts 32 to all characters between 224 and 255 except 247 and 255
    static unsigned char conversion[64] =
        {
            192,
            193,
            194,
            195,
            196,
            197,
            198,
            199,
            200,
            201,
            202,
            203,
            204,
            205,
            206,
            207,
            208,
            209,
            210,
            211,
            212,
            213,
            214,
            215,
            216,
            217,
            218,
            219,
            220,
            221,
            222,
            223,
            192,
            193,
            194,
            195,
            196,
            197,
            198,
            199,
            200,
            201,
            202,
            203,
            204,
            205,
            206,
            207,
            208,
            209,
            210,
            211,
            212,
            213,
            214,
            247,
            216,
            217,
            218,
            219,
            220,
            221,
            222,
            255,
        };

    int nLength = GetLength();
    ChangeBuffer(nLength, 0, nLength, CB_PUSH_ZERO, nLength); //  Create a new instance of this string if multi-referenced

    int32_t iCharacter     = 0;
    const char *pSrcBuffer = GetInternalBuffer();
    char *pDestBuffer      = (char *)pSrcBuffer;
    APT_ASSERT(m_pData->m_uSize <= m_pData->m_uMaxSize && "TRYING TO MODIFY A STATIC STRING IN THE STRING POOL");
    for (;;)
    {
        pSrcBuffer = UTF8_ReadCharacter(pSrcBuffer, &iCharacter);
        if (iCharacter == 0)
        {
            break;
        }
        if (iCharacter < 128)
        {
            iCharacter = toupper(iCharacter);
        }
        else if (iCharacter >= 192 && iCharacter < 256)
        {
            // Sony's CType.h does not handle special characters correctly
            iCharacter = (int)conversion[iCharacter - 192];
        }
        pDestBuffer = UTF8_SetCharacter(pDestBuffer, iCharacter);
    }
    //  UTF8_MakeUpper changes the hash value as it's not in ASCII
    return (*this);
}

EAStringC &EAStringC::UTF8_Initialize(const int32_t iCharacter)
{
    Clear(); //  Clean the string first
    int iSize = UTF8_GetCharacterSize(iCharacter);
    ReserveSize(iSize);
    UTF8_SetOneCharacter(iCharacter);
    return (*this);
}

/**
 * Central buffer-resize/copy primitive used by nearly every mutating method. If this string is
 * the sole owner of its buffer and the buffer is big enough, mutates it in place (optionally
 * shifting uOffsetCopy bytes off the front); otherwise allocates a new, slightly over-sized
 * buffer and copies uSizeCopy bytes from uOffsetCopy into it, releasing the old buffer.
 */
void EAStringC::ChangeBuffer(const uint32_t uSizeToReserve, const uint32_t uOffsetCopy, const uint32_t uSizeCopy, const CBPushZero bPushZero, const uint32_t uInternalSize)
{
    APT_ASSERT(uSizeCopy + uOffsetCopy <= GetLength()); //  Check that the current data are valid
    APT_ASSERT(uSizeCopy <= uSizeToReserve);

    if (GetInternalRefCount() == 1)
    {
        //  If there is only one reference on this buffer we can change it safely
        if (uSizeToReserve <= GetInternalMaxSize())
        {
            //  The size fit, we just need to make the copy
            if (uOffsetCopy)
            {
                //  We need to shift the buffer
                memmove(GetInternalBuffer(), GetInternalBuffer() + uOffsetCopy, uSizeCopy);
            }

            SetInternalSize(uInternalSize);
            InvalidateHashValue();
            if (bPushZero)
            {
                *(GetInternalBuffer() + uInternalSize) = '\0';
            }
            return;
        }
    }

    //  The size doesn't fit with the max size we can have
    //  Or other string points on the same buffer and we need to create another buffer to make the change

    DebugDataC *pData;
    uint32_t uRealReservedSize;
    char *pOldBuffer;

    pOldBuffer = GetInternalBuffer();

    pData = m_pData;
    if (uSizeToReserve != 0)
    {
        uRealReservedSize = uSizeToReserve;
        uRealReservedSize += uSizeToReserve / 8; //  We increase the size of the buffer, add 12% more
                                                 //  We anticipate another grow after
        AllocateBuffer(uRealReservedSize);       //  New buffer allocated

        SetInternalSize(uInternalSize); //  Copy the size
                                        //  Max Size is already set
        InvalidateHashValue();
        memcpy(GetInternalBuffer(), pOldBuffer + uOffsetCopy, uSizeCopy); //  Copy the buffer
        if (bPushZero)
        {
            *(GetInternalBuffer() + uInternalSize) = '\0'; //  Put the trailing '\0'
        }
    }
    else
    {
        //  Empty string
        m_pData = (DebugDataC *)&s_EmptyInternalData;
        IncreaseInternalRefCount();
    }

    DecreaseInternalRefCount(pData); //  Update the old string state
                                     //  The new string is already set up by AllocateBuffer
}

/** Initializes this string (assumed uninitialized) from a NUL-terminated C string. */
void EAStringC::InitFromBuffer(const char *const pStrText)
{
    APT_ASSERT(pStrText != NULL);

    if (*pStrText == 0)
    {
        //  This is an empty string, no need to make allocation and all this stuff
        m_pData = (DebugDataC *)&s_EmptyInternalData;
        IncreaseInternalRefCount();
    }
    else
    {
        int32_t iSize = static_cast<int32_t>(strlen(pStrText));
        AllocateBuffer(iSize);
        SetInternalSize(iSize);
        InvalidateHashValue();
        memcpy(GetInternalBuffer(), pStrText, iSize + 1); //  Copy also the trailing '\0'
    }
}

/** Sets sAptCallbacks internally to the passed-in AptUserFunctions struct. */
void EAStringC::MemInitialize(AptUserFunctions *const callbacks)
{
    APT_ASSERT(sizeof(s_EmptyInternalData) == sizeof(StringDataC) + 1);
    sAptCallbacks = callbacks;
}

/** Sets sAptCallbacks internally to NULL. */
void EAStringC::MemUninitialize()
{
    sAptCallbacks = NULL;
}

const char *EAStringC::c_str() const
{
    return (GetInternalBuffer());
}

/** Same as c_str(); kept for compatibility with older call sites. */
const char *EAStringC::ConstRawPtr() const
{
    return (GetInternalBuffer());
}
