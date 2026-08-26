/* C++ code produced by gperf version 2.7.2 */
/* Command-line: '..\\..\\bin\\util\\gperf.exe' -t -K szName -E -L C++ -Z TextMembersIndex -k 1,2,9 text.gperf  */
struct TextMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 26, duplicates = 0 */

class TextMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct TextMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char TextMembersIndex_asso_values[] =
    {
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 5, 30, 0, 0, 15,
        30, 0, 30, 30, 0, 30, 30, 30, 5, 0,
        5, 0, 30, 30, 15, 0, 0, 10, 5, 15,
        30, 25, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30};

inline unsigned int
TextMembersIndex::hash(const char *str, unsigned int len)
{
    int hval = len;

    switch (hval)
    {
    default:
    case 9:
        hval += TextMembersIndex_asso_values[(unsigned char)str[8]];
        // lint -fallthrough
    case 8:
    case 7:
    case 6:
    case 5:
    case 4:
    case 3:
    case 2:
        hval += TextMembersIndex_asso_values[(unsigned char)str[1]];
        // lint -fallthrough
    case 1:
        hval += TextMembersIndex_asso_values[(unsigned char)str[0]];
        break;
    }
    return hval;
}

struct TextMembers *
TextMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 21,
        MIN_WORD_LENGTH = 4,
        MAX_WORD_LENGTH = 17,
        MIN_HASH_VALUE  = 4,
        MAX_HASH_VALUE  = 29
    };

    static struct TextMembers wordlist[] =
        {
            {""}, {""}, {""}, {""}, {"text", 12}, {""}, {"border", 4}, {"hscroll", 6}, {"maxChars", 8}, {"textWidth", 15}, {"textHeight", 14}, {"length", 7}, {"_height", 19}, {"variable", 17}, {"maxscroll", 9}, {"background", 2}, {"borderColor", 5}, {"mouseWheelEnabled", 21}, {"autoSize", 1}, {"multiline", 10}, {"backgroundColor", 3}, {"scroll", 11}, {""}, {"wordWrap", 18}, {"textColor", 13}, {""}, {"_width", 20}, {""}, {""}, {"type", 16}};

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
