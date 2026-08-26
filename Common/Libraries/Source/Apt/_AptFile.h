#pragma once

struct AptConstantTable
{
    AptVirtualFunctionTable_Indices eType;
    union
    {
        char *szString;
        float fFloat;
        int nInteger;
        int nRegister;
        int bBoolean;
        unsigned int nLookup;
    };
};

struct AptCharacter;

/** Direct memory image created by swfc; the exporter and importer must stay in sync. */
struct AptConstFile
{
    char aMagic[20];

    // relative to base of .apt file
    AptCharacter *pMainCharacter;

    // constants used in the streams
    int nConstants;
    AptConstantTable *aConstants;

    char *GetTimeStamp()
    {
        return aMagic + 4; // aMagic's format is here
        // byte1 byte2 byte3 byte4 byte5 byte 6 ... byte20
        // A     p     t     1     [5 - 20 are time stamp]
    }
    // void fixup(void *pData, void *pUserData, int *pnCurrentConstantIndex, int bUnresolve = 0);
};

/**
 * Mirror of AptConstFile with an explicit pointer-slot width, matching the layout
 * written by a 32-bit (PTR_T = uint32_t) or 64-bit (PTR_T = uint64_t) swfc.
 * Used by the loader to reinterpret a .const blob whose build pointer size differs
 * from the platform's; see AptFileConvert.cpp.
 */
template <typename PTR_T>
struct AptConstFileT
{
    char aMagic[20];

    // relative to base of .apt file
    PTR_T pMainCharacter;

    // constants used in the streams
    int nConstants;
    PTR_T aConstants; // relative to base of .const file
};

using AptConstFile32 = AptConstFileT<uint32_t>;
using AptConstFile64 = AptConstFileT<uint64_t>;