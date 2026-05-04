# Target setup
add_library(g_wwsaveload)

target_sources(
  g_wwsaveload
  PRIVATE definition.cpp
          definition.h
          definitionclassids.h
          definitionfactory.cpp
          definitionfactory.h
          definitionfactorymgr.cpp
          definitionfactorymgr.h
          definitionmgr.cpp
          definitionmgr.h
          editable.h
          parameter.cpp
          parameter.h
          parameterlist.h
          parametertypes.h
          persist.h
          persistfactory.cpp
          persistfactory.h
          pointerremap.cpp
          pointerremap.h
          postloadable.h
          saveload.cpp
          saveload.h
          saveloadids.h
          saveloadstatus.cpp
          saveloadstatus.h
          saveloadsubsystem.cpp
          saveloadsubsystem.h
          simpledefinitionfactory.h
          simpleparameter.h
          twiddler.cpp
          twiddler.h
          wwsaveload.cpp
          wwsaveload.h)

set_target_properties(g_wwsaveload PROPERTIES OUTPUT_NAME "WWSaveLoad")

target_include_directories(g_wwsaveload PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(g_wwsaveload
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../WWMath)

target_link_libraries(g_wwsaveload PUBLIC g_library_include g_wwlib)
