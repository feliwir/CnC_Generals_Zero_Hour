#pragma once

/* C++ code produced by gperf version 2.7.2 */
/* Command-line: 'F:\\TibStudio\\IonCore\\IonCoreV1\\ML\\owner\\Apt\\dev\\bin\\util\\gperf-2.7.2-bin\\bin\\gperf.exe' -t -K szName -E -D -L C++ -Z ObjectIndex -k 1,5,7 'F:\\TibStudio\\IonCore\\IonCoreV1\\ML\\owner\\Apt\\dev\\bin\\util\\..\\..\\source\\Apt\\objects.gperf'  */
struct Objects
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 39, duplicates = 13 */

class ObjectIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct Objects *in_word_set(const char *str, unsigned int len);
};

static unsigned char ObjectIndex_asso_values[] =
    {
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 5, 0,
        15, 20, 25, 30, 23, 8, 13, 18, 42, 42,
        42, 42, 42, 42, 42, 5, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 0, 42, 30, 42, 42,
        42, 42, 42, 0, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 0, 42, 0, 10, 0,
        42, 0, 42, 10, 42, 42, 42, 42, 0, 42,
        0, 42, 42, 42, 5, 30, 9, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
        42, 42, 42, 42, 42, 42};

inline unsigned int
ObjectIndex::hash(const char *str, unsigned int len)
{
    int hval = len;

    switch (hval)
    {
    default:
    case 7:
        hval += ObjectIndex_asso_values[(unsigned char)str[6]];
    case 6:
    case 5:
        hval += ObjectIndex_asso_values[(unsigned char)str[4]];
    case 4:
    case 3:
    case 2:
    case 1:
        hval += ObjectIndex_asso_values[(unsigned char)str[0]];
        break;
    }
    return hval;
}

struct Objects *
ObjectIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 41,
        MIN_WORD_LENGTH = 3,
        MAX_WORD_LENGTH = 14,
        MIN_HASH_VALUE  = 3,
        MAX_HASH_VALUE  = 41
    };

    static struct Objects wordlist[] =
        {
            {"Key", 4},
            {"Stage", 37},
            {"String", 36},
            {"_level1", 7},
            {"_level10", 21},
            {"_level19", 30},
            {"_level18", 29},
            {"_level17", 28},
            {"_level16", 27},
            {"_level15", 26},
            {"_level14", 25},
            {"_level13", 24},
            {"_level12", 23},
            {"_level11", 22},
            {"extern", 17},
            {"_level0", 6},
            {"this", 2},
            {"_root", 3},
            {"_level7", 13},
            {"_parent", 16},
            {"_global", 19},
            {"clearInterval", 51},
            {"_level8", 14},
            {"AptUtil", 39},
            {"_level2", 8},
            {"_level20", 31},
            {"_level24", 35},
            {"_level23", 34},
            {"_level22", 33},
            {"_level21", 32},
            {"AlternateInput", 38},
            {"_level9", 15},
            {"_target", 1},
            {"_level3", 9},
            {"_level6", 12},
            {"_level4", 10},
            {"Math", 5},
            {"Mouse", 20},
            {"_level5", 11},
            {"super", 18},
            {"setInterval", 50}};

    static signed char lookup[] =
        {
            -1, -1, -1, 0, -1, 1, 2, 3, -51, -37,
            -10, 14, 15, 16, 17, 18, 19, 20, 21, -1,
            22, 23, 24, -70, 30, 31, 32, 33, -16, -5,
            34, -1, 35, -1, 36, 37, -1, 38, -1, -1,
            39, 40};

    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
        int key = hash(str, len);

        if (key <= MAX_HASH_VALUE && key >= 0)
        {
            int index = lookup[key];

            if (index >= 0)
            {
                const char *s = wordlist[index].szName;

                if (*str == *s && !strcmp(str + 1, s + 1))
                    return &wordlist[index];
            }
            else if (index < -TOTAL_KEYWORDS)
            {
                int offset                 = -1 - TOTAL_KEYWORDS - index;
                struct Objects *wordptr    = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
                struct Objects *wordendptr = wordptr + -lookup[offset + 1];

                while (wordptr < wordendptr)
                {
                    const char *s = wordptr->szName;

                    if (*str == *s && !strcmp(str + 1, s + 1))
                        return wordptr;
                    wordptr++;
                }
            }
        }
    }
    return 0;
}
