/** Reference-counted, copy-on-write string class used throughout Apt as AptNativeString. */

#pragma once
#include "AptDefine.h"
#include <stdarg.h>

struct AptUserFunctions;

class EAStringC
{
  public:
    EAStringC();

    explicit EAStringC(const uint32_t uReservedLength);
    EAStringC(const uint32_t cChar, const uint32_t uLength);

    EAStringC(const char *const pStrText);
    EAStringC(const EAStringC &strText);
    ~EAStringC();

    // Defining private inline functions here so they can be used by public functions below.
  private:
    class alignas(4) StringDataC
    {
      public:
        uint16_t m_uRefCount;
        uint16_t m_uSize;
        uint16_t m_uMaxSize;
        uint16_t m_uHash;
    };

    class DebugDataC : public StringDataC
    {
      public:
        char m_strText[256];
    };

    APT_INLINE char *GetInternalBuffer() const
    {
        return (((char *)m_pData) + sizeof(StringDataC));
    }

  public:
    struct StaticStringHelperT
    {
        EAStringC::StringDataC sd;
        char pbuf[256];
    };

    APT_INLINE const char *GetBuffer() const
    {
        return (GetInternalBuffer());
    }

    APT_INLINE operator const char *() const
    {
        return (GetBuffer());
    }

    const char *c_str() const;
    const char *ConstRawPtr() const;

    APT_INLINE bool IsConstantString()
    {
        return (m_pData->m_uSize > m_pData->m_uMaxSize);
    }

    APT_INLINE uint32_t operator[](const int32_t index) const;

    EAStringC &operator=(const EAStringC &strText);

    EAStringC &operator=(const StaticStringHelperT &strStruct);

    bool operator==(const EAStringC &strText) const;

    APT_INLINE bool operator!=(const EAStringC &strText) const;

    bool operator<(const EAStringC &strText) const;

    bool operator<=(const EAStringC &strText) const;
    APT_INLINE bool operator>(const EAStringC &strText) const;
    APT_INLINE bool operator>=(const EAStringC &strText) const;

    APT_INLINE bool operator==(const char *const pStrText) const;
    APT_INLINE bool operator!=(const char *const pStrText) const;
    APT_INLINE bool operator<(const char *const pStrText) const;
    APT_INLINE bool operator<=(const char *const pStrText) const;
    APT_INLINE bool operator>(const char *const pStrText) const;
    APT_INLINE bool operator>=(const char *const pStrText) const;

    EAStringC operator+(const EAStringC &strText) const;
    EAStringC &operator+=(const EAStringC &strText);
    EAStringC operator+(const char *const pStrText) const;
    EAStringC &operator+=(const char *const pStrText);

    friend EAStringC operator+(const char *const pStrText, const EAStringC &strText);

    EAStringC &Duplicate(const EAStringC &strText);

    APT_INLINE uint32_t GetLength() const
    {
        return (GetInternalSize());
    }

    APT_INLINE void Clear();
    APT_INLINE void ClearConstantValue();

    void ReserveSize(const uint32_t uSize);
    bool IsEnoughSize(const uint32_t uSize) const;

    EAStringC &Append(const char *const pStrText, const uint32_t uSize);
    void AppendFormat(const char *const pStrFormat, ...);
    void Format(const char *const pStrFormat, ...);
    void vsFormat(const char *const pStrFormat, va_list Args);

    APT_INLINE bool Equal(const char *const pStrText) const;
    APT_INLINE bool Equal(const EAStringC &strText) const;
    APT_INLINE bool EqualNoCase(const char *const pStrText) const;

    bool EqualNoCase(const EAStringC &strText) const;
    APT_INLINE int32_t Compare(const char *const pStrText) const;
    APT_INLINE int32_t Compare(const EAStringC &strText) const;
    APT_INLINE int32_t CompareNoCase(const char *const pStrText) const;
    APT_INLINE int32_t CompareNoCase(const EAStringC &strText) const;

    APT_INLINE bool EqualHash(const EAStringC &strText) const;

    bool EqualNoCaseHash(const EAStringC &strText) const;

    int32_t Find(const char *const pStrText, const int32_t iStart = 0, bool ignoreCase = false) const;
    int32_t Find(const char cChar, const int32_t iStart = 0) const;
    int32_t FindOneOf(const char *const pStrText) const;
    int32_t ReverseFind(const char cChar) const;
    int32_t LastIndexOf(const char *const pStrText, const int32_t iStart = 0) const;

    int32_t Delete(const int32_t iIndex, const int32_t iCount = 1);
    int32_t Remove(const char cRemove);
    int32_t Insert(const int32_t iIndex, const char *const pStrText);
    int32_t Insert(const int32_t iIndex, const char cChar);
    int32_t Replace(const char *const pStrOld, const char *const pStrNew, bool ignoreCase = false);
    int32_t Replace(const char cOld, const char cNew);

    EAStringC Left(const int32_t iCount) const;
    EAStringC Right(const int32_t iCount) const;
    EAStringC Mid(const int32_t iFirst) const;
    EAStringC Mid(const int32_t iFirst, const int32_t iCount) const;

    EAStringC &MakeLower();
    EAStringC &MakeUpper();
    EAStringC &MakeReverse();

    EAStringC &TrimLeft(const char *const pStrText);
    EAStringC &TrimRight(const char *const pStrText);
    EAStringC &Trim(const char *const pStrText);
    bool StartWith(const char *const pStrText) const;
    bool StartWithIgnoreCase(const char *const pStrText) const;
    bool EndWith(const char *const pStrText) const;
    bool EndWithIgnoreCase(const char *const pStrText) const;
    bool StartWithRemove(const char *const pStrText);
    bool StartWithRemoveIgnoreCase(const char *const pStrText);
    bool EndWithRemove(const char *const pStrText);
    bool EndWithRemoveIgnoreCase(const char *const pStrText);

    const char *UTF8_GetBuffer(const int32_t iIndex);
    int32_t UTF8_CharAt(const int32_t iIndex) const;
    int32_t UTF8_Size() const;
    EAStringC UTF8_Mid(const int32_t iFirst) const;
    EAStringC UTF8_Mid(const int32_t iFirst, const int32_t iCount) const;
    EAStringC &UTF8_Append(const char *const pStrText, const int32_t iSize);
    int32_t UTF8_Find(const char *const pStrText, const int32_t iStart = 0) const;
    EAStringC &UTF8_MakeLower();
    EAStringC &UTF8_MakeUpper();
    EAStringC &UTF8_Initialize(const int32_t iCharacter);

    static int32_t UTF8_GetCharacterSize(const char *const pBuffer);
    static int32_t UTF8_GetCharacterSize(const int32_t iCharacter);
    static const char *UTF8_GetBuffer(const char *const pBuffer, const int32_t iIndex);
    static int32_t UTF8_GetSize(const char *const pBuffer);
    static int32_t UTF8_GetCharacter(const char *const pBuffer);
    static char *UTF8_SetCharacter(char *pBuffer, const int32_t iCharacter);
    void UTF8_SetOneCharacter(const int32_t iCharacter);

    static const char *UTF8_ReadCharacter(const char *const pBuffer, int32_t *const iCharacter);

    APT_INLINE static bool UTF8_IsValid(const int32_t iCharacter) { return (iCharacter <= 0x10FFFF); }

    APT_INLINE bool IsEmpty() const;

    //  Adapter for Apt
    APT_INLINE int Size() const;
    APT_INLINE void Reserve(int32_t iSize);

    static void MemInitialize(AptUserFunctions *const callbacks);
    static void MemUninitialize();

    APT_INLINE void Invalidate();
    APT_INLINE void Validate();
    APT_INLINE void Validate(const EAStringC &strText);
    APT_INLINE bool IsValid() const;

    APT_INLINE uint16_t GetHashValue() const;

    APT_INLINE uint16_t UpdateHashValue() const;

    APT_INLINE void CalculateHashValue() const;

    static uint16_t CalculateHashValue(const char *const pText);

    //  Can get the internal informations for debug and profiling purpose
    APT_INLINE uint32_t GetInternalSize() const
    {
        return (m_pData->m_uSize);
    }

    APT_INLINE uint32_t GetInternalMaxSize() const;
    APT_INLINE uint32_t GetInternalRefCount() const;

    static char *Get_s_EmptyInternalData();


  private:
    enum EmptyString
    {
        EMPTY_STRING
    };

    int32_t AsciiIndexToUTF8Index(const int32_t asciiIndex, const int32_t utf8Start = 0, const int32_t asciiStart = 0) const;

    APT_INLINE explicit EAStringC(const EmptyString) : m_pData((DebugDataC *)Get_s_EmptyInternalData())
    {
        IncreaseInternalRefCount();
    }

    APT_INLINE void SetInternalSize(const uint32_t uSize);
    APT_INLINE void SetInternalMaxSize(const uint32_t uMaxSize);
    APT_INLINE void SetInternalRefCount(const uint32_t uRefCount);

    void IncreaseInternalRefCount();

    static void DecreaseInternalRefCount(StringDataC *const pData);

    APT_INLINE void InvalidateHashValue() const;

    enum CBPushZero
    {
        CB_NO_PUSH_ZERO,
        CB_PUSH_ZERO
    };

    void ChangeBuffer(const uint32_t uSizeToReserve, const uint32_t uOffsetCopy, const uint32_t uSizeCopy, const CBPushZero ePushZero, const uint32_t uInternalSize);
    void InitFromBuffer(const char *const pStrText);

    void AllocateBuffer(const uint32_t uSize);

    DebugDataC *m_pData;

    static char s_EmptyInternalData[8 + 1];

    static AptUserFunctions *sAptCallbacks;

    enum
    {
        ZERO_HASH = 0x4567,
    };

};

EAStringC operator+(const char *const pStrText, const EAStringC &strText);

using AptNativeString = EAStringC;
