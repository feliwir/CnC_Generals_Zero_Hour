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

# Header-only helpers shared by the Common, Generals and GeneralsMD test trees.
# Common/Tests is added before either game, so this target is already defined
# by the time they link against it.
add_library(sage_test_support INTERFACE)
target_include_directories(sage_test_support INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Include")

# sage_add_smoke_test: registers one ctest test per startup scenario (same
# TARGET/BASE_ARGS, extra flags per scenario). Splitting into named scenarios
# means a failure points at the specific flag that broke, not one opaque
# pass/fail. Shared so RTS/WorldBuilder/replay smoke tests can all reuse it.
#
# Usage:
#   sage_add_smoke_test(
#     NAME_PREFIX GeneralsMD_RTS_SmokeTest
#     TARGET z_rts_sdl
#     TIMEOUT 30
#     BASE_ARGS -dir "${SOME_DIR}" -ignoreAsserts -autoexit 15
#     ENVIRONMENT "FOO=bar;BAZ=qux"
#     REQUIRE_ENV SAGE_RETAIL_DATA_DIR_GENERALSMD SAGE_RETAIL_DATA_DIR_GENERALS
#     SCENARIOS
#       "quickstart:-quickstart"
#       "novideo:-novideo"
#   )
# Each SCENARIOS entry is "name:extra flags" (parsed with separate_arguments),
# appended after BASE_ARGS. Registers one test per scenario named
# "${NAME_PREFIX}_${name}".
#
# REQUIRE_ENV: env vars that must all be set and non-empty at configure time
# (e.g. CI's retail data dirs); if any are missing, this is a silent no-op --
# no caller-side if(DEFINED ENV{...}) guard needed.
function(sage_add_smoke_test)
  set(oneValueArgs NAME_PREFIX TARGET TIMEOUT)
  set(multiValueArgs BASE_ARGS SCENARIOS ENVIRONMENT REQUIRE_ENV)
  cmake_parse_arguments(SAT "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT SAT_NAME_PREFIX OR NOT SAT_TARGET OR NOT SAT_SCENARIOS)
    message(FATAL_ERROR "sage_add_smoke_test: NAME_PREFIX, TARGET and SCENARIOS are required")
  endif()

  foreach(env_var IN LISTS SAT_REQUIRE_ENV)
    if(NOT DEFINED ENV{${env_var}} OR "$ENV{${env_var}}" STREQUAL "")
      return()
    endif()
  endforeach()

  foreach(scenario IN LISTS SAT_SCENARIOS)
    string(REPLACE ":" ";" scenario_split "${scenario}")
    list(GET scenario_split 0 scenario_name)
    list(REMOVE_AT scenario_split 0)
    # Rejoin (in case the flags themselves contained a ':') into one string,
    # then parse that into real, separate command-line arguments.
    list(JOIN scenario_split ":" scenario_flags_str)
    separate_arguments(scenario_flags UNIX_COMMAND "${scenario_flags_str}")

    set(test_name "${SAT_NAME_PREFIX}_${scenario_name}")
    add_test(
      NAME ${test_name}
      COMMAND $<TARGET_FILE:${SAT_TARGET}> ${SAT_BASE_ARGS} ${scenario_flags}
    )

    # Set each property with its own call rather than building one combined
    # PROPERTIES list: ENVIRONMENT's value is itself a ';'-joined string, and
    # list(APPEND ...) can't hold a literal ';' inside one element -- it
    # would just splice in as extra (unpaired) list items.
    if(DEFINED SAT_TIMEOUT AND NOT "${SAT_TIMEOUT}" STREQUAL "")
      set_tests_properties(${test_name} PROPERTIES TIMEOUT ${SAT_TIMEOUT})
    endif()
    if(SAT_ENVIRONMENT)
      set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "${SAT_ENVIRONMENT}")
    endif()
  endforeach()
endfunction()

add_subdirectory(Stub)
add_subdirectory(GameEngineDevice)
add_subdirectory(Apt)
add_subdirectory(Compression)
add_subdirectory(WWVegas)