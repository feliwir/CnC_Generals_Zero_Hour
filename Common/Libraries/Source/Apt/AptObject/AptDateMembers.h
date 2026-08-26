/* C++ code produced by gperf version 2.7.2 */
/* Command-line: '..\\..\\bin\\util\\gperf.exe' -t -K szName -E -L C++ -Z DateMembersIndex -D DateMembers.gperf  */
struct DateMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 55, duplicates = 6 */

class DateMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct DateMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char DateMembersIndex_asso_values[] =
    {
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 0, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 0, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 30, 58, 15, 30, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 5, 0, 25, 58, 58, 58,
        58, 0, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
        58, 58, 58, 58, 58, 58};

inline unsigned int
DateMembersIndex::hash(const char *str, unsigned int len)
{
    return len + DateMembersIndex_asso_values[(unsigned char)str[len - 1]] + DateMembersIndex_asso_values[(unsigned char)str[0]];
}

struct DateMembers *
DateMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 37,
        MIN_WORD_LENGTH = 3,
        MAX_WORD_LENGTH = 18,
        MIN_HASH_VALUE  = 3,
        MAX_HASH_VALUE  = 57
    };

    static struct DateMembers wordlist[] =
        {
            {"UTC", 37},
            {"setHours", 22},
            {"setMinutes", 24},
            {"setSeconds", 26},
            {"setUTCHours", 30},
            {"setYear", 35},
            {"setUTCMinutes", 32},
            {"setUTCSeconds", 34},
            {"setMilliseconds", 23},
            {"setFullYear", 21},
            {"setUTCMilliseconds", 31},
            {"setUTCFullYear", 29},
            {"getDay", 2},
            {"getHours", 4},
            {"getUTCDay", 12},
            {"getMinutes", 6},
            {"getSeconds", 8},
            {"getUTCHours", 14},
            {"getYear", 19},
            {"getUTCMinutes", 16},
            {"getUTCSeconds", 18},
            {"getMilliseconds", 5},
            {"getFullYear", 3},
            {"getUTCMilliseconds", 15},
            {"getUTCFullYear", 13},
            {"setDate", 20},
            {"setTime", 27},
            {"setMonth", 25},
            {"setUTCDate", 28},
            {"setUTCMonth", 33},
            {"toString", 36},
            {"getDate", 1},
            {"getTime", 9},
            {"getMonth", 7},
            {"getUTCDate", 11},
            {"getUTCMonth", 17},
            {"getTimezoneOffset", 10}};

    static signed char lookup[] =
        {
            -1, -1, -1, 0, -1, -1, -35, -2, 1, -1,
            -44, 4, 5, -84, -1, 8, 9, -1, 10, 11,
            -1, 12, -1, 13, 14, -82, 17, 18, -73, -1,
            21, 22, -1, 23, 24, -18, -2, -80, 27, -1,
            28, 29, -12, -2, -22, -2, -31, -2, 30, -1,
            -6, -2, -88, 33, -1, 34, 35, 36};

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
                int offset                     = -1 - TOTAL_KEYWORDS - index;
                struct DateMembers *wordptr    = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
                struct DateMembers *wordendptr = wordptr + -lookup[offset + 1];

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
