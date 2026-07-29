#include <gtest/gtest.h>

#include "Common/FileSystem.h"
#include "Common/INI.h"
#include "Common/INIParsers.h"
#include "GameClient/Water.h"
#include "MiniAudioDevice/MiniAudioManager.h"
#include "SDL3Device/Common/SDL3BIGFileSystem.h"
#include "SDL3Device/Common/SDL3LocalFileSystem.h"

#define SAGE_TEST_INI_DATA_DIR SAGE_TEST_DATA_DIR "/ini"

TEST(INI, ParseWaterINI)
{
    // Filesystem Boilerplate
    SDL3LocalFileSystem local_fs;
    TheLocalFileSystem = &local_fs;
    FileSystem fs;
    TheFileSystem = &fs;

    // Register parsing functions
    INI::registerBlockParse("WaterSet", WaterSetting::parse);
    INI::registerBlockParse("WaterTransparency", WaterTransparencySetting::parse);

    INI ini;
    WaterSetting waterSetting;

    ini.load(SAGE_TEST_INI_DATA_DIR "/water.ini", INI_LOAD_OVERWRITE, NULL );

}

TEST(INI, ParseSoundEffectsINI)
{
    // Filesystem Boilerplate
    SDL3LocalFileSystem local_fs;
    TheLocalFileSystem = &local_fs;
    FileSystem fs;
    TheFileSystem = &fs;
    MiniAudioManager am;
    TheAudio = &am;

    // Register parsing functions
    INI::registerBlockParse("AudioEvent", INI::parseAudioEventDefinition);

    INI ini;
    ini.load(SAGE_TEST_INI_DATA_DIR "/soundeffects.ini", INI_LOAD_OVERWRITE, NULL );
}

TEST(INI, ParseMusicINI)
{
    // Filesystem Boilerplate
    SDL3LocalFileSystem local_fs;
    TheLocalFileSystem = &local_fs;
    FileSystem fs;
    TheFileSystem = &fs;
    MiniAudioManager am;
    TheAudio = &am;

    // Register parsing functions
    INI::registerBlockParse("MusicTrack", INI::parseMusicTrackDefinition);

    INI ini;
    ini.load(SAGE_TEST_INI_DATA_DIR "/music.ini", INI_LOAD_OVERWRITE, NULL );
}

TEST(INI, ParseMiscAudioINI)
{
    // Filesystem Boilerplate
    SDL3LocalFileSystem local_fs;
    TheLocalFileSystem = &local_fs;
    FileSystem fs;
    TheFileSystem = &fs;
    MiniAudioManager am;
    TheAudio = &am;

    // Register parsing functions
    INI::registerBlockParse("MiscAudio", INI::parseMiscAudio);

    INI ini;
    ini.load(SAGE_TEST_INI_DATA_DIR "/miscaudio.ini", INI_LOAD_OVERWRITE, NULL );
}