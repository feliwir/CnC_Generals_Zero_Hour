add_library(c_compat STATIC d3dx8math.cpp d3dx8tex.cpp d3dx8core.cpp
                            time_compat.cpp)
target_include_directories(c_compat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX)
  target_compile_definitions(c_compat PUBLIC -D_UNIX)
  if(APPLE)
    target_compile_definitions(c_compat PUBLIC -D_MACOSX)
  else()
    target_compile_definitions(c_compat PUBLIC -D_LINUX)
  endif()
  target_link_libraries(c_compat PUBLIC DXVK::D3D8)
elseif(WIN32)
  target_compile_definitions(c_compat PUBLIC -D_WINDOWS)
  target_link_libraries(c_compat PUBLIC d3d8)
endif()

if(NOT SAGE_USE_GLM)
  message(
    FATAL_ERROR
      "GLM is required for the compatibility layer. Please enable SAGE_USE_GLM."
  )
endif()

if(SAGE_USE_GLM)
  find_package(glm CONFIG REQUIRED)
  target_link_libraries(c_compat PRIVATE glm::glm)
  target_compile_definitions(c_compat PRIVATE SAGE_USE_GLM)
endif()
