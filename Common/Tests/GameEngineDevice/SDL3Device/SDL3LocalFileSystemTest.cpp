#include <gtest/gtest.h>

#include "Common/File.h"
#include "SDL3Device/Common/SDL3LocalFileSystem.h"

TEST(SDL3LocalFileSystem, ListFiles)
{
    SDL3LocalFileSystem fs;

    // Recurse through the mod directory and get a list of all files.
    AsciiString mod_dir = AsciiString(SAGE_MOD_SUPPORT_DIR);
    FilenameList fileList;
    fs.getFileListInDirectory(AsciiString(""), mod_dir, "*", fileList, true);
    EXPECT_EQ(fileList.size(), 35110) << "Expected 35110 files in the mod directory, but found " << fileList.size();
}

TEST(SDL3LocalFileSystem, OpenFileExistent)
{
    SDL3LocalFileSystem fs;

    File* file = fs.openFile(SAGE_MOD_SUPPORT_DIR "/README.md", File::READ);
    ASSERT_NE(file, nullptr) << "Failed to open README.md file.";
    EXPECT_STREQ(SAGE_MOD_SUPPORT_DIR "/README.md", file->getName());

    file->close();
}

TEST(SDL3LocalFileSystem, OpenFileNonExistent)
{
    SDL3LocalFileSystem fs;

    File* file = fs.openFile(SAGE_MOD_SUPPORT_DIR "/NON_EXISTENT_FILE.md", File::READ);
    ASSERT_EQ(file, nullptr) << "Unexpectedly opened a non-existent file.";
}

TEST(SDL3LocalFileSystem, OpenFileCreateNew)
{
    SDL3LocalFileSystem fs;

    // Attempt to create a new file
    File* file = fs.openFile("test_file.txt", File::WRITE | File::CREATE);
    ASSERT_NE(file, nullptr) << "Failed to create a new file.";
    EXPECT_STREQ("test_file.txt", file->getName());

    // Close cleans up the file and deletes it if deleteOnClose is set.
    file->close();
}

TEST(SDL3LocalFileSystem, DoesFileExistExistent)
{
    SDL3LocalFileSystem fs;

    EXPECT_TRUE(fs.doesFileExist(SAGE_MOD_SUPPORT_DIR "/README.md"));
}

TEST(SDL3LocalFileSystem, DoesFileExistNonExistent)
{
    SDL3LocalFileSystem fs;

    EXPECT_FALSE(fs.doesFileExist(SAGE_MOD_SUPPORT_DIR "/NON_EXISTENT_FILE.md"));
}

TEST(SDL3LocalFileSystem, CreateDirectory)
{
    SDL3LocalFileSystem fs;

    AsciiString newDir = AsciiString("test_dir");
    EXPECT_TRUE(fs.createDirectory(newDir)) << "Failed to create directory: " << newDir.str();

    // Clean up by removing the created directory (if needed)
    // Note: Implement directory removal if necessary.
}

TEST(SDL3LocalFileSystem, GetFileInfoExistent)
{
    SDL3LocalFileSystem fs;

    FileInfo fileInfo;
    EXPECT_TRUE(fs.getFileInfo(SAGE_MOD_SUPPORT_DIR "/README.md", &fileInfo)) << "Failed to get file info for README.md";
    Int64 size = (static_cast<Int64>(fileInfo.sizeHigh) << 32) | static_cast<Int64>(fileInfo.sizeLow);
    EXPECT_GT(size, 0) << "File size should be greater than 0 for README.md";
}

TEST(SDL3LocalFileSystem, GetFileInfoNonExistent)
{
    SDL3LocalFileSystem fs;

    FileInfo fileInfo;
    EXPECT_FALSE(fs.getFileInfo(SAGE_MOD_SUPPORT_DIR "/NON_EXISTENT_FILE.md", &fileInfo)) << "Unexpectedly got file info for a non-existent file.";
}