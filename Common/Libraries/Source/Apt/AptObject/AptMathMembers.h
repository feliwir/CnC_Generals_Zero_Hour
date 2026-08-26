/* C++ code produced by gperf version 2.7.2 */
struct MathMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 47, duplicates = 1 */

class MathMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct MathMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char MathMembersIndex_asso_values[] =
    {
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        5, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 20, 50, 5,
        5, 10, 0, 30, 50, 50, 50, 50, 0, 10,
        25, 50, 0, 50, 0, 0, 15, 50, 50, 0,
        5, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50};

inline unsigned int
MathMembersIndex::hash(const char *str, unsigned int len)
{
    return len + MathMembersIndex_asso_values[(unsigned char)str[len - 1]] + MathMembersIndex_asso_values[(unsigned char)str[0]];
}

struct MathMembers *
MathMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 18,
        MIN_WORD_LENGTH = 3,
        MAX_WORD_LENGTH = 6,
        MIN_HASH_VALUE  = 3,
        MAX_HASH_VALUE  = 49
    };

    static struct MathMembers wordlist[] =
        {
            {"pow", 15},
            {"floor", 13},
            {"cos", 2},
            {"ceil", 11},
            {"round", 4},
            {"exp", 12},
            {"random", 16},
            {"max", 6},
            {"sqrt", 17},
            {"abs", 7},
            {"acos", 8},
            {"sin", 1},
            {"atan2", 3},
            {"log", 14},
            {"min", 5},
            {"tan", 18},
            {"asin", 9},
            {"atan", 10}};

    static signed char lookup[] =
        {
            -1, -1, -1, 0, -1, 1, -1, -1, 2, 3,
            4, -1, -1, 5, -1, -1, 6, -1, 7, 8,
            -1, -1, -1, 9, 10, -1, -1, -1, 11, -1,
            12, -1, -1, 13, -1, -1, -1, -1, 14, -1,
            -1, -1, -1, 15, -1, -1, -1, -2, -2, -66};

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
                struct MathMembers *wordptr    = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
                struct MathMembers *wordendptr = wordptr + -lookup[offset + 1];

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
