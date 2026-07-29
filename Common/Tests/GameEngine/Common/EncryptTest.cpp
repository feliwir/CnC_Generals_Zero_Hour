#include <gtest/gtest.h>

#include "Common/Encrypt.h"

TEST(Encrypt, EncryptString)
{
    const char* input = "Test123";
    const char* encrypted = EncryptString(input);

    // Check that the encrypted string is not null
    ASSERT_NE(encrypted, nullptr);

    ASSERT_STREQ(encrypted, "ucIfGbKa"); // Ensure consistent output for the same input

    // Check that the encrypted string is not the same as the input
    ASSERT_STRNE(encrypted, input);

    // Check that the length of the encrypted string is correct
    ASSERT_EQ(strlen(encrypted), MAX_ENCRYPTED_STRING);
}