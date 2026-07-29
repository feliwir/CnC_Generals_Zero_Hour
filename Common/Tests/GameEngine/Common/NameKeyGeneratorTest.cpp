#include <gtest/gtest.h>

#include "Common/NameKeyGenerator.h"

TEST(NameKeyGenerator, GenerateKeys)
{
    NameKeyGenerator generator;
    generator.init();

    // Different strings should produce different keys
    NameKeyType key1 = generator.nameToKey("Test1");
    NameKeyType key2 = generator.nameToKey("Test2");
    NameKeyType key3 = generator.nameToKey("Test3");

    EXPECT_NE(key1, NAMEKEY_INVALID);
    EXPECT_NE(key1, key2);
    EXPECT_NE(key1, key3);
    EXPECT_NE(key2, key3);

    // Same string should produce the same key
    NameKeyType key1Again = generator.nameToKey("Test1");
    EXPECT_EQ(key1, key1Again);
    NameKeyType key2Again = generator.nameToKey("Test2");
    EXPECT_EQ(key2, key2Again);
    NameKeyType key3Again = generator.nameToKey("Test3");
    EXPECT_EQ(key3, key3Again);

    // Check that the keys can be converted back to names
    EXPECT_EQ(generator.keyToName(key1), "Test1");
    EXPECT_EQ(generator.keyToName(key2), "Test2");
    EXPECT_EQ(generator.keyToName(key3), "Test3");

    // Check if a non-existent key returns an empty string
    EXPECT_EQ(generator.keyToName(static_cast<NameKeyType>(9999)), AsciiString::TheEmptyString);

    // Check if the lowercase key generation works correctly
    NameKeyType lowerKey1 = generator.nameToLowercaseKey("Test1");
    NameKeyType lowerKey1Again = generator.nameToLowercaseKey("test1");
    EXPECT_EQ(lowerKey1, lowerKey1Again);
}
