/* C++ code produced by gperf version 2.7.2 */
/* Command-line: '..\\..\\bin\\util\\gperf' -t -K szName -E -L C++ -Z SoundMembersIndex sound.gperf  */

#if defined(APT_USE_SOUND_OBJECT)

struct SoundMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 8, duplicates = 0 */

class SoundMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct SoundMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char SoundMembersIndex_asso_values[] =
    {
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 0, 12, 12,
        0, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 0, 12, 12, 0, 0, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12};

inline unsigned int
SoundMembersIndex::hash(const char *str, unsigned int len)
{
    return len + SoundMembersIndex_asso_values[(unsigned char)str[len - 1]] + SoundMembersIndex_asso_values[(unsigned char)str[0]];
}

struct SoundMembers *
SoundMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 3,
        MIN_WORD_LENGTH = 4,
        MAX_WORD_LENGTH = 11,
        MIN_HASH_VALUE  = 4,
        MAX_HASH_VALUE  = 11
    };

    static struct SoundMembers wordlist[] =
        {
            {""}, {""}, {""}, {""}, {"stop", 3}, {"start", 2}, {""}, {""}, {""}, {""}, {""}, {"attachSound", 1}};

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
#endif // #if defined(APT_USE_SOUND_OBJECT)
