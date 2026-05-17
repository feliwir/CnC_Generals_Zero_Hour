find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

qt_add_executable(
  c_wdump
  chunk_d.cpp
  chunk_d.h
  wdump.cpp
  wdump.h
  wdump.rc
  wdumpwin.ui
  wdumpwin.cpp
  wdumpwin.h)

set_target_properties(c_wdump PROPERTIES OUTPUT_NAME "wdump")
target_link_libraries(c_wdump PRIVATE Qt6::Widgets c_wwlib c_ww3d2)
