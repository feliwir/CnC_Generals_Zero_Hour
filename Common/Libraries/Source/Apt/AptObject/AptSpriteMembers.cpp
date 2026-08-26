#include "_Apt.h"
#include "AptSpriteMembers.h"
#include "MainInline.h"

/* C++ code produced by gperf version 2.7.2 */
/* maximum key range = 249, duplicates = 1 */

static unsigned char SpriteMembersIndex_asso_values[] =
    {
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 10, 30, 251,
        0, 0, 251, 0, 251, 251, 10, 20, 251, 251,
        20, 251, 251, 5, 0, 75, 251, 0, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 95, 5, 30,
        0, 0, 40, 20, 70, 5, 251, 251, 35, 0,
        124, 80, 60, 20, 65, 15, 55, 0, 0, 10,
        30, 15, 0, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
        251, 251, 251, 251, 251, 251};

/** Computes the gperf hash of @p str for the sprite member keyword table. */
inline unsigned int SpriteMembersIndex::hash(const char *str, unsigned int len)
{
    int hval = len;

    switch (hval)
    {
    default:
    case 8:
        hval += SpriteMembersIndex_asso_values[(unsigned char)str[7]];
    case 7:
    case 6:
        hval += SpriteMembersIndex_asso_values[(unsigned char)str[5]];
    case 5:
    case 4:
    case 3:
    case 2:
        hval += SpriteMembersIndex_asso_values[(unsigned char)str[1]];
        break;
    }
    return hval;
}

/** Looks up @p str (length @p len) in the sprite member keyword table, or returns NULL if not found. */
struct SpriteMembers *SpriteMembersIndex::in_word_set(const char *str, unsigned int len)
{
    enum
    {
        TOTAL_KEYWORDS  = 90,
        MIN_WORD_LENGTH = 2,
        MAX_WORD_LENGTH = 20,
        MIN_HASH_VALUE  = 2,
        MAX_HASH_VALUE  = 250
    };

    static struct SpriteMembers wordlist[] =
        {
            {(char *)"_z", 128},
            {(char *)"_url", 16},
            {(char *)"getBounds", 117},
            {(char *)"_visible", 8},
            {(char *)"removeTextField", 119},
            {(char *)"getURL", 102},
            {(char *)"_y", 2},
            {(char *)"isNaN", 26},
            {(char *)"escape", 28},
            {(char *)"setMask", 122},
            {(char *)"getNewTextFormat", 123},
            {(char *)"hitTest", 118},
            {(char *)"_soundbuftime", 19},
            {(char *)"_x", 1},
            {(char *)"_ymouse", 22},
            {(char *)"play", 107},
            {(char *)"_zscale", 135},
            {(char *)"getTextFormat", 124},
            {(char *)"setTextFormat", 125},
            {(char *)"_quality", 20},
            {(char *)"_xmouse", 21},
            {(char *)"_yscale", 4},
            {(char *)"stop", 110},
            {(char *)"_target", 12},
            {(char *)"blendMode", 30},
            {(char *)"_focusrect", 18},
            {(char *)"_xscale", 3},
            {(char *)"nextFrame", 113},
            {(char *)"swapDepths", 120},
            {(char *)"filters", 31},
            {(char *)"createTextField", 114},
            {(char *)"getBytesTotal", 112},
            {(char *)"getBytesLoaded", 111},
            {(char *)"createEmptyMovieClip", 116},
            {(char *)"_width", 9},
            {(char *)"_framesloaded", 13},
            {(char *)"localToGlobal", 127},
            {(char *)"removeMovieClip", 109},
            {(char *)"_currentframe", 5},
            {(char *)"duplicateMovieClip", 101},
            {(char *)"_renderflags", 131},
            {(char *)"_CustomControlType", 36},
            {(char *)"_skipeval", 35},
            {(char *)"_yrotation", 130},
            {(char *)"getInstanceAtDepth", 136},
            {(char *)"_name", 14},
            {(char *)"onLoad", 207},
            {(char *)"_droptarget", 15},
            {(char *)"onEnterFrame", 203},
            {(char *)"getDepth", 115},
            {(char *)"prevFrame", 108},
            {(char *)"_xrotation", 129},
            {(char *)"onPress", 211},
            {(char *)"_height", 10},
            {(char *)"onRelease", 212},
            {(char *)"onMouseWheel", 218},
            {(char *)"onDragOut", 201},
            {(char *)"onDragOver", 202},
            {(char *)"onReleaseOutside", 213},
            {(char *)"parseInt", 32},
            {(char *)"extern", 23},
            {(char *)"globalToLocal", 132},
            {(char *)"onSetFocus", 216},
            {(char *)"_totalframes", 6},
            {(char *)"onRollOut", 214},
            {(char *)"onRollOver", 215},
            {(char *)"onMouseMove", 209},
            {(char *)"onKeyDown", 204},
            {(char *)"loadMovie", 105},
            {(char *)"loadMovieNum", 133},
            {(char *)"onMouseDown", 208},
            {(char *)"Boolean", 29},
            {(char *)"parseFloat", 33},
            {(char *)"startDrag", 126},
            {(char *)"_parent", 34},
            {(char *)"loadVariables", 106},
            {(char *)"_alpha", 7},
            {(char *)"_highquality", 17},
            {(char *)"onKeyUp", 205},
            {(char *)"_rotation", 11},
            {(char *)"onUnload", 217},
            {(char *)"unloadMovie", 121},
            {(char *)"attachMovie", 100},
            {(char *)"unloadMovieNum", 134},
            {(char *)"gotoAndStop", 104},
            {(char *)"onMouseUp", 210},
            {(char *)"onData", 200},
            {(char *)"unescape", 27},
            {(char *)"gotoAndPlay", 103},
            {(char *)"onKillFocus", 206}};

    static short lookup[] =
        {
            -1, -1, 0, -1, 1, -1, -1, -1,
            -1, 2, -1, -1, -1, 3, -1, 4,
            5, 6, -1, -1, 7, 8, 9, -1,
            -1, -1, 10, 11, 12, -1, -1, -1,
            13, -1, -1, -1, -1, 14, -1, 15,
            -1, -1, 16, -135, -73, -2, -1, -1,
            19, -1, -1, -1, 20, -1, -1, -1,
            -1, 21, -1, 22, -1, -1, 23, -1,
            24, 25, -1, -1, -1, -1, -1, -1,
            26, -1, 27, 28, -1, 29, -1, -1,
            30, -1, -1, 31, 32, 33, 34, -1,
            35, -1, -1, -1, -1, 36, -1, 37,
            -1, -1, 38, -1, -1, -1, -1, 39,
            -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, 40, 41, 42,
            -1, -1, -1, -1, -1, 43, -1, -1,
            44, 45, 46, 47, -1, -1, -1, -1,
            48, -1, 49, 50, 51, -1, -1, -1,
            -1, -1, 52, 53, 54, -1, -1, 55,
            -1, 56, 57, 58, -1, -1, 59, -1,
            60, -1, -1, 61, 62, -1, -1, 63,
            64, 65, 66, -1, -1, 67, 68, -1,
            -1, 69, -1, -1, 70, -1, 71, -1,
            -1, 72, -1, -1, -1, 73, -1, 74,
            -1, 75, -1, -1, 76, 77, -1, -1,
            -1, -1, -1, -1, -1, -1, 78, -1,
            -1, 79, -1, -1, 80, -1, -1, 81,
            82, -1, 83, -1, 84, -1, -1, 85,
            -1, 86, -1, 87, -1, -1, -1, -1,
            -1, -1, -1, 88, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, 89};

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
                struct SpriteMembers *wordptr    = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
                struct SpriteMembers *wordendptr = wordptr + -lookup[offset + 1];

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
