#include <gtest/gtest.h>

#include "Common/AsciiString.h"

TEST(AsciiString, CheckEmpty)
{
    AsciiString str;

    EXPECT_TRUE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 0);
    EXPECT_STREQ(str.str(), "");
    EXPECT_EQ(str, AsciiString::TheEmptyString);
}

TEST(AsciiString, CheckSetAndGet)
{
    AsciiString str;
    str.set("Hello, World!");

    EXPECT_FALSE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 13);
    EXPECT_STREQ(str.str(), "Hello, World!");
    EXPECT_EQ(str, AsciiString("Hello, World!"));
    EXPECT_EQ(str, "Hello, World!");
}

TEST(AsciiString, CheckConcat)
{
    AsciiString str;
    str.set("Hello");
    str.concat(", ");
    str.concat("World!");

    EXPECT_FALSE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 13);
    EXPECT_STREQ(str.str(), "Hello, World!");
}

TEST(AsciiString, CheckTrim)
{
    AsciiString str;
    str.set("   Hello, World!   ");
    str.trim();

    EXPECT_FALSE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 13);
    EXPECT_STREQ(str.str(), "Hello, World!");
}

TEST(AsciiString, Utf8)
{
    AsciiString str;
    str.set(u8"Hello, 世界!"); // "Hello, World!" in Chinese characters

    EXPECT_FALSE(str.isEmpty());
    EXPECT_EQ(str.getLength(), 10); // Each Chinese character counts as one character
    EXPECT_STREQ(str.str(), u8"Hello, 世界!");
}

TEST(AsciiString, CheckToLower)
{
    AsciiString str;
    str.set("Hello, World!");
    str.toLower();

    EXPECT_EQ(str.getLength(), 13);
    EXPECT_STREQ(str.str(), "hello, world!");
}

TEST(AsciiString, CheckRemoveLastChar)
{
    AsciiString str;
    str.set("Hello, World!");
    str.removeLastChar();

    EXPECT_EQ(str.getLength(), 12);
    EXPECT_STREQ(str.str(), "Hello, World");
}

TEST(AsciiString, CheckFormat)
{
    AsciiString str;
    str.format("Hello, %s!", "World");

    EXPECT_EQ(str.getLength(), 13);
    EXPECT_STREQ(str.str(), "Hello, World!");

    str.clear();
    str.format(AsciiString("Hello, %s!"), "World");

    EXPECT_EQ(str.getLength(), 13);
    EXPECT_STREQ(str.str(), "Hello, World!");
}

TEST(AsciiString, CheckCompare)
{
    AsciiString str1("Hello, World!");
    AsciiString str2("hello, world!");

    EXPECT_TRUE(str1.compare(str2) != 0);
    EXPECT_TRUE(str1.compare(AsciiString("Hello, World!")) == 0);

    EXPECT_TRUE(str1.compareNoCase(str2) == 0);
    EXPECT_TRUE(str2.compareNoCase(str1) == 0);

    AsciiString str3("Hello, C++!");
    EXPECT_FALSE(str1.compareNoCase(str3) == 0);
}

TEST(AsciiString, CheckStartsWith)
{
    AsciiString str("Hello, World!");

    EXPECT_TRUE(str.startsWith("Hello"));
    EXPECT_FALSE(str.startsWith("World"));
    EXPECT_TRUE(str.startsWith(AsciiString("Hello")));
    EXPECT_FALSE(str.startsWith(AsciiString("World")));

    EXPECT_TRUE(str.startsWithNoCase("hello"));
    EXPECT_FALSE(str.startsWithNoCase("world"));
    EXPECT_TRUE(str.startsWithNoCase(AsciiString("hello")));
    EXPECT_FALSE(str.startsWithNoCase(AsciiString("world")));
}

TEST(AsciiString, CheckEndsWith)
{
    AsciiString str("Hello, World!");

    EXPECT_TRUE(str.endsWith("World!"));
    EXPECT_FALSE(str.endsWith("Hello"));
    EXPECT_TRUE(str.endsWith(AsciiString("World!")));
    EXPECT_FALSE(str.endsWith(AsciiString("Hello")));

    EXPECT_TRUE(str.endsWithNoCase("world!"));
    EXPECT_FALSE(str.endsWithNoCase("hello"));
    EXPECT_TRUE(str.endsWithNoCase(AsciiString("world!")));
    EXPECT_FALSE(str.endsWithNoCase(AsciiString("hello")));
}

TEST(AsciiString, GetToken)
{
    AsciiString str("Lorem ipsum dolor sit amet");
    AsciiString token;
    EXPECT_TRUE(str.nextToken(&token, " "));
    EXPECT_EQ(token, AsciiString("Lorem"));
    EXPECT_TRUE(str.nextToken(&token, " "));
    EXPECT_EQ(token, AsciiString("ipsum"));
    EXPECT_TRUE(str.nextToken(&token, " "));
    EXPECT_EQ(token, AsciiString("dolor"));
    EXPECT_TRUE(str.nextToken(&token, " "));
    EXPECT_EQ(token, AsciiString("sit"));
    EXPECT_TRUE(str.nextToken(&token, " "));
    EXPECT_EQ(token, AsciiString("amet"));
    EXPECT_FALSE(str.nextToken(&token, " "));
}