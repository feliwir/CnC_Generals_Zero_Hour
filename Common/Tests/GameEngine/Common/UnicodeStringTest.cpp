#include <gtest/gtest.h>

#include "Common/UnicodeString.h"

bool UTF16EQ(const WideChar *s1, const WideChar *s2)
{
    if (s1 == nullptr || s2 == nullptr)
        return s1 == s2; // both null is equal, one null is not equal

    while (*s1 && *s2)
    {
        if (*s1 != *s2)
            return false;
        ++s1;
        ++s2;
    }
    return *s1 == *s2; // both should be null at the end
}

TEST(UnicodeString, CheckEmpty)
{
    UnicodeString str;

    EXPECT_TRUE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 0);
    EXPECT_TRUE(UTF16EQ(str.str(), u""));
    EXPECT_EQ(str, UnicodeString::TheEmptyString);
}

TEST(UnicodeString, CheckSetAndGet)
{
    UnicodeString str;
    str.set(u"Hello, World!");

    EXPECT_FALSE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 13);
    EXPECT_TRUE(UTF16EQ(str.str(), u"Hello, World!"));
    EXPECT_EQ(str, UnicodeString(u"Hello, World!"));
}

TEST(UnicodeString, CheckConcat)
{
    UnicodeString str;
    str.set(u"Hello");
    str.concat(u", ");
    str.concat(u"World!");

    EXPECT_FALSE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 13);
    EXPECT_TRUE(UTF16EQ(str.str(), u"Hello, World!"));
}

TEST(UnicodeString, CheckTrim)
{
    UnicodeString str;
    str.set(u"   Hello, World!   ");
    str.trim();

    EXPECT_FALSE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 13);
    EXPECT_TRUE(UTF16EQ(str.str(), u"Hello, World!"));
}

TEST(UnicodeString, Utf16)
{
    UnicodeString str;
    str.set(u"Hello, 世界!"); // "Hello, World!" in Chinese characters

    EXPECT_FALSE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 10); // Each Chinese character counts as one character
    EXPECT_TRUE(UTF16EQ(str.str(), u"Hello, 世界!"));
}

TEST(UnicodeString, CheckRemoveLastChar)
{
    UnicodeString str;
    str.set(u"Hello, World!");
    str.removeLastChar();

    EXPECT_EQ(str.getLength(), 12);
    EXPECT_TRUE(UTF16EQ(str.str(), u"Hello, World"));
}

TEST(UnicodeString, CheckFormat)
{
    UnicodeString str;
    str.format(u"Hello, %S!", u"World");

    EXPECT_EQ(str.getLength(), 13);
    EXPECT_TRUE(UTF16EQ(str.str(), u"Hello, World!"));

    str.clear();
    str.format(UnicodeString(u"Hello, %S!"), u"World");

    EXPECT_EQ(str.getLength(), 13);
    EXPECT_TRUE(UTF16EQ(str.str(), u"Hello, World!"));
}

TEST(UnicodeString, CheckCompare)
{
    UnicodeString str1(u"Hello");
    UnicodeString str2(u"World");
    UnicodeString str3(u"Hello");

    EXPECT_LT(str1.compare(str2), 0);
    EXPECT_GT(str2.compare(str1), 0);
    EXPECT_EQ(str1.compare(str3), 0);

    EXPECT_LT(str1.compareNoCase(str2), 0);
    EXPECT_GT(str2.compareNoCase(str1), 0);
    EXPECT_EQ(str1.compareNoCase(str3), 0);
}

TEST(UnicodeString, GetToken)
{
    UnicodeString str(u"Lorem ipsum dolor sit amet");
    UnicodeString token;
    EXPECT_TRUE(str.nextToken(&token, UnicodeString(u" ")));
    EXPECT_EQ(token, UnicodeString(u"Lorem"));
    EXPECT_TRUE(str.nextToken(&token, UnicodeString(u" ")));
    EXPECT_EQ(token, UnicodeString(u"ipsum"));
    EXPECT_TRUE(str.nextToken(&token, UnicodeString(u" ")));
    EXPECT_EQ(token, UnicodeString(u"dolor"));
    EXPECT_TRUE(str.nextToken(&token, UnicodeString(u" ")));
    EXPECT_EQ(token, UnicodeString(u"sit"));
    EXPECT_TRUE(str.nextToken(&token, UnicodeString(u" ")));
    EXPECT_EQ(token, UnicodeString(u"amet"));
    EXPECT_FALSE(str.nextToken(&token, UnicodeString(u" ")));
}
