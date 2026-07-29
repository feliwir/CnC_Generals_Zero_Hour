# Qt6 port of the Zero Hour WorldBuilder.  The MFC sources in src/ & include/
# are ported to Qt6 in place; files are added here as they are ported.

# GuiPrivate was introduced starting with 6.10
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OPTIONAL_COMPONENTS GuiPrivate)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

qt_add_executable(
  z_worldbuilder
  include/addplayerdialog.h
  include/AutoEdgeOutTool.h
  include/BlendEdgeTool.h
  include/BlendMaterial.h
  include/BrushTool.h
  include/CUndoable.h
  include/DrawObject.h
  include/EyedropperTool.h
  include/FeatherOptions.h
  include/FeatherTool.h
  include/HandScrollTool.h
  include/FloodFillTool.h
  include/MainFrm.h
  include/MapSettings.h
  include/MoundOptions.h
  include/MoundTool.h
  include/ObjectOptions.h
  include/ObjectPreview.h
  include/ObjectTool.h
  include/PointerTool.h
  include/PolygonTool.h
  include/RoadTool.h
  include/SplashScreen.h
  include/TerrainMaterial.h
  include/TileTool.h
  include/Tool.h
  include/WBHeightMap.h
  include/WHeightMapEdit.h
  include/WaterOptions.h
  include/WaterTool.h
  include/WorldBuilder.h
  include/WorldBuilderDoc.h
  include/brushoptions.h
  include/mapobjectprops.h
  include/wbview.h
  include/wbview3d.h
  src/addplayerdialog.cpp
  src/AutoEdgeOutTool.cpp
  src/BlendEdgeTool.cpp
  src/BlendMaterial.cpp
  src/BrushTool.cpp
  src/CUndoable.cpp
  src/DrawObject.cpp
  src/EyedropperTool.cpp
  src/FeatherOptions.cpp
  src/FeatherTool.cpp
  src/HandScrollTool.cpp
  src/FloodFillTool.cpp
  src/MainFrm.cpp
  src/MapSettings.cpp
  src/MoundOptions.cpp
  src/MoundTool.cpp
  src/ObjectOptions.cpp
  src/ObjectPreview.cpp
  src/ObjectTool.cpp
  src/PointerTool.cpp
  src/PolygonTool.cpp
  src/RoadTool.cpp
  src/SplashScreen.cpp
  src/TerrainMaterial.cpp
  src/TileTool.cpp
  src/Tool.cpp
  src/WBHeightMap.cpp
  src/WHeightMapEdit.cpp
  src/WaterOptions.cpp
  src/WaterTool.cpp
  src/WorldBuilder.cpp
  src/WorldBuilderDoc.cpp
  src/brushoptions.cpp
  src/mapobjectprops.cpp
  src/placeholderview.cpp
  src/wbview.cpp
  src/wbview3d.cpp)

# The splash artwork (IDB_WORLDBUILDERSPLASH in the original .rc).
qt_add_resources(z_worldbuilder "splash" PREFIX "/" BASE res FILES
                 res/WorldBuilder.bmp)

# Toolbar icons: sliced from the original IDR_MAINFRAME toolbar bitmap
# (res/Toolbar.bmp, 16x15 buttons in .rc BUTTON order) and converted to PNG
# with the classic button-face gray (192,192,192) keyed to alpha, since a
# palettized BMP with no transparency isn't a usable Qt icon source.
# handscroll_tool.png has no original toolbar slice (Hand Scroll wasn't a
# toolbar button in the MFC tool, only reachable via spacebar); it's rendered
# from its existing cursor (IDC_HAND_SCROLL.cur) instead, for the button this
# Qt port added.
qt_add_resources(z_worldbuilder "icons" PREFIX "/icons" BASE res/icons FILES
                 res/icons/file_new.png
                 res/icons/file_open.png
                 res/icons/file_save.png
                 res/icons/edit_cut.png
                 res/icons/edit_copy.png
                 res/icons/edit_paste.png
                 res/icons/ruler_tool.png
                 res/icons/pointer_tool.png
                 res/icons/lock_horizontal.png
                 res/icons/brush_tool.png
                 res/icons/mound_tool.png
                 res/icons/dig_tool.png
                 res/icons/feather_tool.png
                 res/icons/meshmold_tool.png
                 res/icons/water_tool.png
                 res/icons/tile_tool.png
                 res/icons/bigtile_tool.png
                 res/icons/eyedropper_tool.png
                 res/icons/floodfill_tool.png
                 res/icons/autoedgeout_tool.png
                 res/icons/autoedgein_tool.png
                 res/icons/blendedge_tool.png
                 res/icons/object_tool.png
                 res/icons/road_tool.png
                 res/icons/grove_tool.png
                 res/icons/ramp_tool.png
                 res/icons/scorch_tool.png
                 res/icons/fence_tool.png
                 res/icons/buildlist_tool.png
                 res/icons/waypoint_tool.png
                 res/icons/polygon_tool.png
                 res/icons/border_tool.png
                 res/icons/script_edit.png
                 res/icons/team_edit.png
                 res/icons/app_about.png
                 res/icons/handscroll_tool.png)

# Tool cursors from the original resources.
qt_add_resources(z_worldbuilder "cursors" PREFIX "/cursors" BASE res FILES
                 res/cross.cur
                 res/IDC_HAND_SCROLL.cur
                 res/IDC_AUTO_EDGE_OUT.cur
                 res/IDC_BLEND_EDGE.cur
                 res/IDC_CLIFF.cur
                 res/IDC_EYEDROPPER.cur
                 res/IDC_FLOOD_FILL.cur
                 res/IDC_MOVE_POINTER.cur
                 res/IDC_POINTER.cur
                 res/IDC_ROTATE.cur
                 res/TileTool.cur
                 res/IDC_ROAD.cur
                 res/IDC_WATER.cur)

set_target_properties(z_worldbuilder PROPERTIES OUTPUT_NAME "WorldBuilder")

target_include_directories(z_worldbuilder
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
                                   ${CMAKE_SOURCE_DIR}/Common/Tools/WorldBuilder/include)

target_link_libraries(z_worldbuilder PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui
                                             z_gameengine z_gameenginedevice c_wwqt6)
if(TARGET Qt6::GuiPrivate)
  target_link_libraries(z_worldbuilder PRIVATE Qt6::GuiPrivate)
endif()
