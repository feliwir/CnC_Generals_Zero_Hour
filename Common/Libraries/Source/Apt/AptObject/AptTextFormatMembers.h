/* C++ code produced by gperf version 2.7.2 */
struct TextFormatMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 44, duplicates = 0 */

class TextFormatMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct TextFormatMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char TextFormatMembersIndex_asso_values[] =
    {
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 0, 47, 47, 47, 0, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 5, 10, 20,
        0, 47, 15, 0, 47, 20, 47, 47, 0, 47,
        0, 0, 47, 47, 0, 0, 0, 0, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
        47, 47, 47, 47, 47, 47};

inline unsigned int
TextFormatMembersIndex::hash(const char *str, unsigned int len)
{
    int hval = len;

    switch (hval)
    {
    default:
    case 8:
        hval += TextFormatMembersIndex_asso_values[(unsigned char)str[7]];
        // lint -fallthrough
    case 7:
    case 6:
        hval += TextFormatMembersIndex_asso_values[(unsigned char)str[5]];
        // lint -fallthrough
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
        hval += TextFormatMembersIndex_asso_values[(unsigned char)str[0]];
        break;
    }
    return hval;
}

struct TextFormatMembers *
TextFormatMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 17,
        MIN_WORD_LENGTH = 3,
        MAX_WORD_LENGTH = 11,
        MIN_HASH_VALUE  = 3,
        MAX_HASH_VALUE  = 46
    };

    static struct TextFormatMembers wordlist[] =
        {
            {""}, {""}, {""}, {"url", 16}, {"size", 14}, {""}, {"target", 13}, {"leading", 9}, {"tabStops", 12}, {"underline", 15}, {"align", 1}, {"rightMargin", 11}, {""}, {""}, {"bold", 3}, {"leftMargin", 10}, {"bullet", 4}, {""}, {""}, {"font", 6}, {""}, {"blockIndent", 2}, {""}, {""}, {""}, {"color", 5}, {"indent", 7}, {""}, {"tracking", 17}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {"italic", 8}};

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
