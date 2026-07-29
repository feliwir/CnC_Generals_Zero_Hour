#include <gtest/gtest.h>

#include "Common/CRC.h"

TEST(CRC, ComputeCRC)
{
    CRC crc;

    // Test with a simple string
    const char* testString = "Hello, World!";
    crc.computeCRC(testString, strlen(testString));
    UnsignedInt result1 = crc.get();
    EXPECT_EQ(result1, 0xAD4F9); // 709881 

    // Reset and compute again to ensure consistency
    crc.clear();
    crc.computeCRC(testString, strlen(testString));
    UnsignedInt result2 = crc.get();

    EXPECT_EQ(result1, result2);

    // Test with a different string
    const char* testString2 = "Goodbye!";
    crc.clear();
    crc.computeCRC(testString2, strlen(testString2));
    UnsignedInt result3 = crc.get();

    EXPECT_EQ(result3, 0x593F ); // 22847
    EXPECT_NE(result1, result3);
}
