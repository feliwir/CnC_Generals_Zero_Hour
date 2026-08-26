#pragma once

/* C++ code produced by gperf version 2.7.2 */
struct KeyMembers
{
    const char *szName;
    int nIndex;
};
/* maximum key range = 38, duplicates = 2 */

class KeyMembersIndex
{
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct KeyMembers *in_word_set(const char *str, unsigned int len);
};

static unsigned char KeyMembersIndex_asso_values[] =
    {
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 0, 40, 25, 40, 5,
        40, 15, 10, 5, 40, 20, 40, 40, 15, 0,
        0, 40, 40, 5, 0, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 5, 40, 0,
        5, 0, 40, 20, 40, 0, 40, 40, 40, 40,
        0, 0, 40, 40, 0, 0, 40, 25, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40};

inline unsigned int
KeyMembersIndex::hash(const char *str, unsigned int len)
{
    int hval = len;

    switch (hval)
    {
    default:
    case 8:
        hval += KeyMembersIndex_asso_values[(unsigned char)str[7]];
    case 7:
    case 6:
        hval += KeyMembersIndex_asso_values[(unsigned char)str[5]];
    case 5:
    case 4:
    case 3:
    case 2:
        hval += KeyMembersIndex_asso_values[(unsigned char)str[1]];
        break;
    }
    return hval;
}

struct KeyMembers *
KeyMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 28,
        MIN_WORD_LENGTH = 2,
        MAX_WORD_LENGTH = 20,
        MIN_HASH_VALUE  = 2,
        MAX_HASH_VALUE  = 39
    };

    static unsigned char lengthtable[] =
        {
            2, 3, 4, 4, 5, 6, 7, 8, 4, 5, 7, 13, 14, 5,
            11, 7, 3, 4, 4, 5, 6, 18, 9, 20, 8, 9, 9, 14};
    static struct KeyMembers wordlist[] =
        {
            {"UP", 18},
            {"TAB", 17},
            {"DOWN", 5},
            {"HOME", 9},
            {"SPACE", 16},
            {"isDown", 100},
            {"CONTROL", 3},
            {"getAscii", 107},
            {"LEFT", 11},
            {"RIGHT", 14},
            {"getCode", 102},
            {"getController", 103},
            {"removeListener", 105},
            {"SHIFT", 15},
            {"addListener", 104},
            {"ESCAPED", 8},
            {"END", 6},
            {"PGDN", 12},
            {"PGUP", 13},
            {"ENTER", 7},
            {"INSERT", 10},
            {"getAnalogStickInfo", 106},
            {"DELETEKEY", 4},
            {"getAnalogTriggerInfo", 108},
            {"CAPSLOCK", 2},
            {"isToggled", 101},
            {"BACKSPACE", 1},
            {"getGestureInfo", 109}};

    static signed char lookup[] =
        {
            -1, -1, 0, 1, -59, 4, 5, 6, 7, 8,
            9, -1, 10, 11, 12, 13, 14, 15, 16, -55,
            19, 20, -1, 21, 22, 23, -11, -2, 24, 25,
            -26, -2, -1, -1, 26, -1, -1, -1, -1, 27};

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
            else if (index < -TOTAL_KEYWORDS)
            {
                int offset                     = -1 - TOTAL_KEYWORDS - index;
                const unsigned char *lengthptr = &lengthtable[TOTAL_KEYWORDS + lookup[offset]];
                struct KeyMembers *wordptr     = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
                struct KeyMembers *wordendptr  = wordptr + -lookup[offset + 1];

                while (wordptr < wordendptr)
                {
                    if (len == *lengthptr)
                    {
                        const char *s = wordptr->szName;

                        if (*str == *s && !memcmp(str + 1, s + 1, len - 1))
                            return wordptr;
                    }
                    lengthptr++;
                    wordptr++;
                }
            }
        }
    }
    return 0;
}
