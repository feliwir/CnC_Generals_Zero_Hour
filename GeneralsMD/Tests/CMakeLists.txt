# CNC Modding Support published by EA
include(FetchContent)
FetchContent_Declare(
  cnc_mod_support
  GIT_REPOSITORY https://github.com/electronicarts/CnC_Modding_Support.git
  GIT_TAG main
)
FetchContent_MakeAvailable(cnc_mod_support)
set(SAGE_MOD_SUPPORT_DIR "${cnc_mod_support_SOURCE_DIR}")
set(SAGE_TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data")

add_subdirectory(GameEngine)
add_subdirectory(GameEngineDevice)

# Smoke tests: launch the actual RTS binary against the CI-downloaded retail
# data, one per startup scenario (see sage_add_smoke_test in
# Common/Tests/CMakeLists.txt). REQUIRE_ENV skips all of these when that data
# isn't configured. -autoexit gives a real process exit instead of relying on
# ctest's TIMEOUT to kill a healthy process.
sage_add_smoke_test(
  NAME_PREFIX GeneralsMD_RTS_SmokeTest
  TARGET z_rts_sdl
  TIMEOUT 60
  BASE_ARGS -dir "$ENV{SAGE_RETAIL_DATA_DIR_GENERALSMD}"
            -bigdir "$ENV{SAGE_RETAIL_DATA_DIR_GENERALS}"
            -win -ignoreAsserts -autoexit 15 -multi
  REQUIRE_ENV SAGE_RETAIL_DATA_DIR_GENERALSMD SAGE_RETAIL_DATA_DIR_GENERALS
  SCENARIOS
    "quickstart:-quickstart"
    "novideo:-novideo"
    "noaudio:-noaudio"
    "nomusic:-nomusic"
    "multi:-multi"
    "full:-multi -quickstart -noaudio -novideo -nomusic"
)

# Smoke test: play back a real replay (Data/rep, see its README.md), exercising
# the -file/.rep path. Replays load from <user-data-dir>/Replays/<filename>
# (RecorderClass::getReplayDir()), not -dir/-bigdir, so the checked-in .rep
# must be staged there first. That's a build-time file copy, not a ctest
# result, so it's a custom target z_rts_sdl depends on rather than a fixture
# test. Guarded the same as REQUIRE_ENV below to avoid writing into every
# developer's home directory when retail data isn't configured.
if(DEFINED ENV{SAGE_RETAIL_DATA_DIR_GENERALSMD} AND NOT "$ENV{SAGE_RETAIL_DATA_DIR_GENERALSMD}" STREQUAL ""
   AND DEFINED ENV{SAGE_RETAIL_DATA_DIR_GENERALS} AND NOT "$ENV{SAGE_RETAIL_DATA_DIR_GENERALS}" STREQUAL "")
  set(SAGE_REPLAY_SMOKETEST_DIR "$ENV{HOME}/Command and Conquer Generals Zero Hour Data/Replays")
  set(SAGE_REPLAY_SMOKETEST_SRC "${SAGE_TEST_DATA_DIR}/rep/alpine_assault_excal_spl.rep")
  set(SAGE_REPLAY_SMOKETEST_DEST "${SAGE_REPLAY_SMOKETEST_DIR}/alpine_assault_excal_spl.rep")
  add_custom_command(
    OUTPUT "${SAGE_REPLAY_SMOKETEST_DEST}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SAGE_REPLAY_SMOKETEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SAGE_REPLAY_SMOKETEST_SRC}" "${SAGE_REPLAY_SMOKETEST_DEST}"
    DEPENDS "${SAGE_REPLAY_SMOKETEST_SRC}"
    COMMENT "Staging alpine_assault_excal_spl.rep for GeneralsMD_Replay_SmokeTest"
  )
  add_custom_target(GeneralsMD_Replay_SmokeTest_StageReplay ALL
    DEPENDS "${SAGE_REPLAY_SMOKETEST_DEST}"
  )
  add_dependencies(z_rts_sdl GeneralsMD_Replay_SmokeTest_StageReplay)
endif()

sage_add_smoke_test(
  NAME_PREFIX GeneralsMD_Replay_SmokeTest
  TARGET z_rts_sdl
  TIMEOUT 120
  BASE_ARGS -dir "$ENV{SAGE_RETAIL_DATA_DIR_GENERALSMD}"
            -bigdir "$ENV{SAGE_RETAIL_DATA_DIR_GENERALS}"
            -win -ignoreAsserts -multi
  REQUIRE_ENV SAGE_RETAIL_DATA_DIR_GENERALSMD SAGE_RETAIL_DATA_DIR_GENERALS
  SCENARIOS
    "default:-quickstart -file alpine_assault_excal_spl.rep -autoexit 45"
)
# Smoke test: boot straight into each campaign's first mission via -campaign
# (GameEngine::init mirrors the shell's campaign launch) and let it simulate
# for 120 in-game seconds. -novideo skips the mission intro movies. The sim
# runs at real time, so TIMEOUT leaves headroom for startup/mission load and
# CPU contention from parallel tests.
sage_add_smoke_test(
  NAME_PREFIX GeneralsMD_Campaign_SmokeTest
  TARGET z_rts_sdl
  TIMEOUT 300
  BASE_ARGS -dir "$ENV{SAGE_RETAIL_DATA_DIR_GENERALSMD}"
            -bigdir "$ENV{SAGE_RETAIL_DATA_DIR_GENERALS}"
            -win -ignoreAsserts -multi -quickstart -novideo -autoexit 120
  REQUIRE_ENV SAGE_RETAIL_DATA_DIR_GENERALSMD SAGE_RETAIL_DATA_DIR_GENERALS
  SCENARIOS
    "training:-campaign TRAINING"
    "usa:-campaign USA"
    "gla:-campaign GLA"
    "china:-campaign China"
)

# Smoke test: launch WorldBuilder, load a real map from cnc_mod_support, then
# quit. WorldBuilder has no -autoexit/-quickstart, so WB_TEST_OPEN /
# WB_TEST_AUTOEXIT (WorldBuilder.cpp's main()) drive the same behavior. Still
# guarded on SAGE_BUILD_TOOLS (a CMake variable, not env var, so REQUIRE_ENV
# can't cover it) since z_worldbuilder only exists when Tools is built.
if(SAGE_BUILD_TOOLS)
  set(SAGE_WORLDBUILDER_SMOKETEST_MAP
      "${SAGE_MOD_SUPPORT_DIR}/Zero Hour/Maps/official/Tournament Plains/Tournament Plains.map")
  sage_add_smoke_test(
    NAME_PREFIX GeneralsMD_WorldBuilder_SmokeTest
    TARGET z_worldbuilder
    TIMEOUT 90
    BASE_ARGS -dir "$ENV{SAGE_RETAIL_DATA_DIR_GENERALSMD}"
              -bigdir "$ENV{SAGE_RETAIL_DATA_DIR_GENERALS}"
              -win
    ENVIRONMENT "WB_TEST_OPEN=${SAGE_WORLDBUILDER_SMOKETEST_MAP};WB_TEST_AUTOEXIT=15;QT_QPA_PLATFORM=xcb;SDL_VIDEODRIVER=x11"
    REQUIRE_ENV SAGE_RETAIL_DATA_DIR_GENERALSMD SAGE_RETAIL_DATA_DIR_GENERALS
    SCENARIOS
      "default:-ignoreAsserts"
  )
endif()
