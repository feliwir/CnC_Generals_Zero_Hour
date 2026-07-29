#include <gtest/gtest.h>

#include "Common/Dict.h"

TEST(Dict, SetAndGetValues)
{
    NameKeyGenerator nkg;
    nkg.init();
    Dict dict;

    // Test setting and getting a boolean value
    NameKeyType boolKey = nkg.nameToKey("boolKey");
    dict.setBool(boolKey, true);
    EXPECT_TRUE(dict.getBool(boolKey));

    // Test setting and getting an integer value
    NameKeyType intKey = nkg.nameToKey("intKey");
    dict.setInt(intKey, 42);
    EXPECT_EQ(dict.getInt(intKey), 42);

    // Test setting and getting a real value
    NameKeyType realKey = nkg.nameToKey("realKey");
    dict.setReal(realKey, 3.14f);
    EXPECT_FLOAT_EQ(dict.getReal(realKey), 3.14f);

    // Test setting and getting an ASCII string value
    NameKeyType asciiStringKey = nkg.nameToKey("asciiStringKey");
    AsciiString asciiValue("Hello");
    dict.setAsciiString(asciiStringKey, asciiValue);
    EXPECT_EQ(dict.getAsciiString(asciiStringKey), asciiValue);

    // Test setting and getting a Unicode string value
    NameKeyType unicodeStringKey = nkg.nameToKey("unicodeStringKey");
    UnicodeString unicodeValue(u"World");
    dict.setUnicodeString(unicodeStringKey, unicodeValue);
    EXPECT_EQ(dict.getUnicodeString(unicodeStringKey), unicodeValue);
}

TEST(Dict, RemoveValue)
{
    NameKeyGenerator nkg;
    nkg.init();
    Dict dict;

    // Test removing a value
    NameKeyType key = nkg.nameToKey("keyToRemove");
    dict.setInt(key, 100);
    EXPECT_EQ(dict.getInt(key), 100);
    dict.remove(key);
    EXPECT_EQ(dict.getType(key), Dict::DICT_NONE);
}

TEST(Dict, ClearDict)
{
    NameKeyGenerator nkg;
    nkg.init();
    Dict dict;

    // Test clearing the dictionary
    NameKeyType key1 = nkg.nameToKey("key1");
    NameKeyType key2 = nkg.nameToKey("key2");
    dict.setInt(key1, 10);
    dict.setBool(key2, false);
    EXPECT_EQ(dict.getPairCount(), 2);
    dict.clear();
    EXPECT_EQ(dict.getPairCount(), 0);
}