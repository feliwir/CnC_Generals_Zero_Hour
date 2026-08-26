/* C++ code produced by gperf version 2.7.2 */
/* Command-line: '..\\..\\bin\\util\\gperf.exe' -t -K szName -E -L C++ -Z ArrayMembersIndex ArrayMembers.gperf  */
struct ArrayMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 25, duplicates = 0 */

class ArrayMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct ArrayMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char ArrayMembersIndex_asso_values[] =
    {
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 5,
        28, 15, 28, 0, 10, 28, 5, 28, 0, 28,
        0, 28, 0, 28, 5, 0, 0, 0, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28};

inline unsigned int
ArrayMembersIndex::hash(const char *str, unsigned int len)
{
    return len + ArrayMembersIndex_asso_values[(unsigned char)str[len - 1]] + ArrayMembersIndex_asso_values[(unsigned char)str[0]];
}

struct ArrayMembers *
ArrayMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 13,
        MIN_WORD_LENGTH = 3,
        MAX_WORD_LENGTH = 8,
        MIN_HASH_VALUE  = 3,
        MAX_HASH_VALUE  = 27
    };

    static struct ArrayMembers wordlist[] =
        {
            {""}, {""}, {""}, {"pop", 4}, {"sort", 9}, {"shift", 6}, {"sortOn", 12}, {"unshift", 7}, {"toString", 13}, {"join", 3}, {""}, {"concat", 2}, {""}, {""}, {"push", 5}, {""}, {"length", 1}, {""}, {""}, {""}, {"slice", 11}, {"splice", 10}, {""}, {""}, {""}, {""}, {""}, {"reverse", 8}};

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
