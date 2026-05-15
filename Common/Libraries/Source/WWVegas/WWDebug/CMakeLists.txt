# Target setup
add_library(c_wwdebug)

target_sources(
  c_wwdebug
  PRIVATE wwdebug.cpp
          wwdebug.h
          wwhack.h
          wwmemlog.cpp
          wwmemlog.h
          wwprofile.cpp
          wwprofile.h)

set_target_properties(c_wwdebug PROPERTIES OUTPUT_NAME "WWDebug")

target_include_directories(c_wwdebug PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(c_wwdebug
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../WWLib)

target_link_libraries(c_wwdebug PRIVATE c_library_include)
