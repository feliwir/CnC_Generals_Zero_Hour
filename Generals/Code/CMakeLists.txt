# Set binary & library output directories to lib/Generals
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/Generals)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/Generals)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Generals)

add_subdirectory(Libraries)
add_subdirectory(Tools)
add_subdirectory(GameEngine)
add_subdirectory(GameEngineDevice)
add_subdirectory(Main)