#include "_Apt.h"
#if defined(APT_USE_XML_OBJECT)
#include "AptXmlMembers.h"
#include "MainInline.h"

static unsigned char XmlMemberIndex_asso_values[] =
    {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 10, 10, 77,
        77, 77, 77, 77, 77, 77, 10, 77, 15, 77,
        77, 77, 77, 77, 10, 77, 77, 77, 15, 77,
        77, 77, 77, 77, 77, 77, 77, 25, 20, 77,
        0, 0, 77, 20, 0, 0, 77, 77, 0, 77,
        0, 31, 5, 77, 30, 0, 0, 5, 77, 77,
        77, 10, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77};

inline unsigned int
XmlMemberIndex::hash(const char *str, unsigned int len)
{
    int hval = len;

    switch (hval)
    {
    default:
    case 8:
        hval += XmlMemberIndex_asso_values[(unsigned char)str[7]];
    case 7:
    case 6:
        hval += XmlMemberIndex_asso_values[(unsigned char)str[5]];
    case 5:
    case 4:
    case 3:
    case 2:
        hval += XmlMemberIndex_asso_values[(unsigned char)str[1]];
        break;
    }
    return hval;
}

struct XmlMembers *
XmlMemberIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 29,
        MIN_WORD_LENGTH = 4,
        MAX_WORD_LENGTH = 15,
        MIN_HASH_VALUE  = 4,
        MAX_HASH_VALUE  = 76
    };

    static struct XmlMembers wordlist[] =
        {
            {""}, {""}, {""}, {""}, {"send", 110}, {""}, {"status", 112}, {""}, {""}, {""}, {""}, {"nextSibling", 9}, {"insertBefore", 7}, {"getBytesTotal", 104}, {"getBytesLoaded", 105}, {""}, {"appendChild", 1}, {""}, {""}, {""}, {"firstChild", 5}, {"sendAndLoad", 111}, {""}, {""}, {"cloneNode", 4}, {"childNodes", 3}, {""}, {""}, {""}, {""}, {"attributes", 2}, {"ignoreWhite", 106}, {""}, {""}, {"lastChild", 8}, {"load", 107}, {""}, {"loaded", 108}, {"hasChildNodes", 6}, {""}, {""}, {"removeNode", 15}, {""}, {"createElement", 101}, {"createTextNode", 102}, {""}, {""}, {""}, {"parseXml", 109}, {"nodeType", 11}, {""}, {""}, {"contentType", 100}, {""}, {""}, {""}, {""}, {"docTypeDecl", 103}, {""}, {"toString", 16}, {""}, {""}, {""}, {""}, {"nodeName", 10}, {""}, {"parentNode", 13}, {""}, {""}, {""}, {"nodeValue", 12}, {""}, {""}, {""}, {""}, {""}, {"previousSibling", 14}};

    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
        int key = hash(str, len);

        if (key <= MAX_HASH_VALUE && key >= 0)
        {
            const char *s = wordlist[key].szName;

            if (*str == *s && !strcmp(str + 1, s + 1))
                return &wordlist[key];
        }
    }
    return 0;
}
#endif // APT_USE_XML_OBJECT
