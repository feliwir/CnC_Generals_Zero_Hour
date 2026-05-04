# Target setup
add_library(g_wwmath)

target_sources(
  g_wwmath
  PRIVATE aabox.cpp
          aabox.h
          aabtreecull.cpp
          aabtreecull.h
          aaplane.h
          cardinalspline.cpp
          cardinalspline.h
          castres.h
          catmullromspline.cpp
          catmullromspline.h
          colmath.cpp
          colmath.h
          colmathaabox.cpp
          colmathaabox.h
          colmathaabtri.cpp
          colmathfrustum.cpp
          colmathfrustum.h
          colmathinlines.h
          colmathline.cpp
          colmathline.h
          colmathobbobb.cpp
          colmathobbox.cpp
          colmathobbtri.cpp
          colmathplane.cpp
          colmathplane.h
          cullsys.cpp
          cullsys.h
          culltype.h
          curve.cpp
          curve.h
          euler.cpp
          euler.h
          frustum.cpp
          frustum.h
          gridcull.cpp
          gridcull.h
          hermitespline.cpp
          hermitespline.h
          lineseg.cpp
          lineseg.h
          lookuptable.cpp
          lookuptable.h
          matrix3.cpp
          matrix3.h
          matrix3d.cpp
          matrix3d.h
          matrix4.cpp
          matrix4.h
          obbox.cpp
          obbox.h
          ode.cpp
          ode.h
          plane.h
          pot.cpp
          pot.h
          quat.cpp
          quat.h
          rect.h
          sphere.h
          tcbspline.cpp
          tcbspline.h
          tri.cpp
          tri.h
          v3_rnd.cpp
          v3_rnd.h
          vector2.h
          vector2i.h
          vector3.h
          vector3i.h
          vector4.h
          vehiclecurve.cpp
          vehiclecurve.h
          vp.cpp
          vp.h
          wwmath.cpp
          wwmath.h
          wwmathids.h)

set_target_properties(g_wwmath PROPERTIES OUTPUT_NAME "WWMath")

target_include_directories(g_wwmath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_include_directories(g_wwmath
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../WWLib)

target_link_libraries(g_wwmath PUBLIC g_library_include g_wwlib g_wwsaveload)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(g_wwmath PRIVATE glm::glm)