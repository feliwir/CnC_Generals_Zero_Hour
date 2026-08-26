#pragma once

/* C++ code produced by gperf version 2.7.2 */
struct UtilMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 20, duplicates = 0 */

class UtilMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct UtilMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char UtilMembersIndex_asso_values[] =
    {
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 10, 10, 29, 29, 29,
        29, 29, 0, 0, 29, 29, 29, 0, 29, 29,
        29, 29, 29, 0, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 0, 29, 15,
        29, 0, 29, 29, 0, 0, 29, 29, 0, 29,
        29, 0, 29, 29, 0, 5, 0, 0, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29};

inline unsigned int
UtilMembersIndex::hash(const char *str, unsigned int len)
{
    return len + UtilMembersIndex_asso_values[(unsigned char)str[7]] + UtilMembersIndex_asso_values[(unsigned char)str[5]] + UtilMembersIndex_asso_values[(unsigned char)str[1]];
}

struct UtilMembers *
UtilMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 14,
        MIN_WORD_LENGTH = 9,
        MAX_WORD_LENGTH = 26,
        MIN_HASH_VALUE  = 9,
        MAX_HASH_VALUE  = 28
    };

    static unsigned char lengthtable[] =
        {
            9, 10, 11, 13, 14, 15, 18, 19, 10, 18, 15, 26, 17, 13};
    static struct UtilMembers wordlist[] =
        {
            {"safeForIn", 10},
            {"trimString", 3},
            {"searchArray", 6},
            {"getAptVersion", 9},
            {"trimLeftString", 4},
            {"trimRightString", 5},
            {"formatNumberString", 1},
            {"colorMatrixMultiply", 12},
            {"countArray", 8},
            {"reverseSearchArray", 7},
            {"profileBlockEnd", 14},
            {"convertHsvToColorTransform", 11},
            {"profileBlockStart", 13},
            {"replaceString", 2}};

    static signed char lookup[] =
        {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, -1, 3,
            4, 5, -1, -1, 6, 7, 8, -1, -1, 9, -1, 10, 11, 12,
            13};

    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
        int key = hash(str, len);

        if (key <= MAX_HASH_VALUE && key >= 0)
        {
            int index = lookup[key];

            if (index >= 0)
            {
                if (len == lengthtable[index])
                {
                    const char *s = wordlist[index].szName;

                    if (*str == *s && !memcmp(str + 1, s + 1, len - 1))
                        return &wordlist[index];
                }
            }
        }
    }
    return 0;
}
