#if APT_USE_LOADVARS_OBJECT
/* C++ code produced by gperf version 2.7.2 */
struct LoadVarsMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 13, duplicates = 0 */

class LoadVarsMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct LoadVarsMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char LoadVarsMembersIndex_asso_values[] =
    {
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 0,
        0, 0, 17, 0, 17, 17, 17, 17, 0, 17,
        17, 17, 17, 17, 17, 5, 0, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
        17, 17, 17, 17, 17, 17};

inline unsigned int
LoadVarsMembersIndex::hash(const char *str, unsigned int len)
{
    return len + LoadVarsMembersIndex_asso_values[(unsigned char)str[len - 1]] + LoadVarsMembersIndex_asso_values[(unsigned char)str[0]];
}

struct LoadVarsMembers *
LoadVarsMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 8,
        MIN_WORD_LENGTH = 4,
        MAX_WORD_LENGTH = 14,
        MIN_HASH_VALUE  = 4,
        MAX_HASH_VALUE  = 16
    };

    static struct LoadVarsMembers wordlist[] =
        {
            {""}, {""}, {""}, {""}, {"load", 1}, {""}, {"loaded", 6}, {""}, {"toString", 7}, {"send", 2}, {""}, {"contentType", 8}, {""}, {"getBytesTotal", 4}, {"getBytesLoaded", 5}, {""}, {"sendAndLoad", 3}};

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
#endif // #if APT_USE_LOADVARS_OBJECT
