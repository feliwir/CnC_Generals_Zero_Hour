/* C++ code produced by gperf version 2.7.2 */
/* Command-line: '..\\..\\bin\\util\\gperf.exe' -t -K szName -E -L C++ -Z StringMembersIndex -D StringMembers.gperf  */
struct StringMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 18, duplicates = 2 */

class StringMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct StringMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char AptStringMembers_asso_values[] =
    {
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 0,
        23, 10, 0, 0, 10, 0, 23, 23, 0, 23,
        23, 23, 23, 23, 14, 0, 0, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23};

inline unsigned int
StringMembersIndex::hash(const char *str, unsigned int len)
{

    return len + AptStringMembers_asso_values[(unsigned char)str[len - 1]] + AptStringMembers_asso_values[(unsigned char)str[0]];
}

struct StringMembers *
StringMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 13,
        MIN_WORD_LENGTH = 5,
        MAX_WORD_LENGTH = 12,
        MIN_HASH_VALUE  = 5,
        MAX_HASH_VALUE  = 22
    };

    static struct StringMembers wordlist[] =
        {
            {"split", 9},
            {"charAt", 2},
            {"concat", 4},
            {"indexOf", 6},
            {"substring", 11},
            {"charCodeAt", 3},
            {"lastIndexOf", 7},
            {"slice", 8},
            {"length", 1},
            {"substr", 10},
            {"toLowerCase", 12},
            {"toUpperCase", 13},
            {"fromCharCode", 5}};

    static signed char lookup[] =
        {
            -1, -1, -1, -1, -1, 0, -26, 3, -1, 4,
            5, 6, -12, -2, -1, 7, 8, -1, -3, -2,
            9, -32, 12};

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
                int offset                       = -1 - TOTAL_KEYWORDS - index;
                struct StringMembers *wordptr    = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
                struct StringMembers *wordendptr = wordptr + -lookup[offset + 1];

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
