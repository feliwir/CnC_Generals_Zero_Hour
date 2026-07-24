# A compiled build of the Common engine and device sources, for tests.
#
# c_gameengine and c_gameenginedevice are INTERFACE source lists: they carry
# sources but produce no archive, so each game compiles them into its own
# g_*/z_* libraries under RTS_GAME_GEN / RTS_GAME_ZH. Tests under Common/Tests
# have no game to borrow from, so this target links the interface libraries --
# pulling in their sources, include paths and dependencies -- and compiles them
# once, game-agnostically.
add_library(c_gameenginestub STATIC gamestub.cpp)

# PRIVATE, not PUBLIC: an INTERFACE source list propagates its sources to every
# consumer, so a PUBLIC link would make each test executable recompile the whole
# engine itself. Linking privately keeps the sources here, and the include paths
# are re-exported below so consumers can still find the headers.
target_link_libraries(c_gameenginestub PRIVATE c_gameenginedevice)

# The interface libraries no longer export their sources (see the
# SAGE_GAME_SOURCES note in Common/GameEngine/CMakeLists.txt), so pull the
# lists in explicitly for the game-agnostic stub build.
target_sources(c_gameenginestub PRIVATE
  $<TARGET_PROPERTY:c_gameengine,SAGE_GAME_SOURCES>
  $<TARGET_PROPERTY:c_gameenginedevice,SAGE_GAME_SOURCES>)

target_include_directories(c_gameenginestub PUBLIC
  $<TARGET_PROPERTY:c_gameenginedevice,INTERFACE_INCLUDE_DIRECTORIES>)

# A handful of Common sources still include headers that live only in the game
# trees (Common/Player.h, GameLogic/Weapon.h, GameLogic/Object.h,
# GameLogic/ExperienceTracker.h, Common/MessageStream.h). They are gameplay
# code that no Common test exercises, so drop them from this target rather than
# block the whole library on finishing their commonization. HEADER_FILE_ONLY
# excludes a source from compilation for one target without disturbing the
# shared interface list. Remove entries here as their headers move to Common.
set(SAGE_STUB_UNCOMMONIZED_SOURCES
    Common/GameEngine/Source/Common/INI/INIMapCache.cpp
    Common/GameEngine/Source/GameNetwork/Connection.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/SubObjectsUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Helper/ObjectWeaponStatusHelper.cpp
    Common/GameEngine/Source/GameLogic/Object/Body/BodyModule.cpp
    Common/GameEngine/Source/GameNetwork/IPEnumeration.cpp
    Common/GameEngine/Source/GameNetwork/FrameDataManager.cpp
    Common/GameEngine/Source/GameNetwork/Transport.cpp
    Common/GameEngine/Source/GameClient/Drawable/Update/SwayClientUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Contain/CaveContain.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/MapSelectMenu.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/FlammableUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/HijackerUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/FireSpreadUpdate.cpp
    Common/GameEngine/Source/Common/RTS/Science.cpp
    Common/GameEngine/Source/GameClient/MessageStream/HotKey.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/TechBuildingBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Collide/FireWeaponCollide.cpp
    Common/GameEngine/Source/GameLogic/Object/Die/DieModule.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/StatusBitsUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Map/SidesList.cpp
    Common/GameEngine/Source/GameLogic/Map/PolygonTrigger.cpp
    Common/GameEngine/Source/GameLogic/Map/TerrainLogic.cpp
    Common/GameEngine/Source/GameNetwork/NetPacket.cpp
    Common/GameEngine/Source/GameNetwork/NetworkUtil.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/WorldHeightMap.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/HeightMap.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/BaseHeightMap.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/FlatHeightMap.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/TileData.cpp
    Common/GameEngine/Source/GameClient/FXList.cpp
    Common/GameEngine/Source/GameClient/Display.cpp
    Common/GameEngine/Source/GameClient/GraphDraw.cpp
    Common/GameEngine/Source/GameClient/Credits.cpp
    Common/GameEngine/Source/GameClient/RadiusDecal.cpp
    Common/GameEngine/Source/GameClient/View.cpp
    Common/GameEngine/Source/GameNetwork/NetCommandMsg.cpp
    Common/GameEngine/Source/GameNetwork/NetCommandList.cpp
    Common/GameEngine/Source/GameNetwork/LANGameInfo.cpp
    Common/GameEngine/Source/GameNetwork/NetCommandWrapperList.cpp
    Common/GameEngine/Source/GameNetwork/NetCommandRef.cpp
    Common/GameEngine/Source/Common/StatsCollector.cpp
    Common/GameEngine/Source/Common/CRCDebug.cpp
    Common/GameEngine/Source/GameClient/GUI/GameWindowGlobal.cpp
    Common/GameEngine/Source/GameClient/GUI/AnimateWindowManager.cpp
    Common/GameEngine/Source/GameClient/GUI/WindowLayout.cpp
    Common/GameEngine/Source/GameClient/GUI/GameWindowManager.cpp
    Common/GameEngine/Source/GameClient/GUI/WindowVideoManager.cpp
    Common/GameEngine/Source/GameClient/GUI/WinInstanceData.cpp
    Common/GameEngine/Source/GameClient/GUI/GameWindowTransitions.cpp
    Common/GameEngine/Source/GameClient/GUI/ProcessAnimateWindow.cpp
    Common/GameEngine/Source/GameClient/System/Anim2D.cpp
    Common/GameEngine/Source/GameClient/Terrain/TerrainRoads.cpp
    Common/GameEngine/Source/Common/Thing/Module.cpp
    Common/GameEngine/Source/Common/Thing/Thing.cpp
    Common/GameEngine/Source/Common/INI/INIControlBarScheme.cpp
    Common/GameEngine/Source/Common/INI/INITerrainBridge.cpp
    Common/GameEngine/Source/Common/INI/INITerrainRoad.cpp
    Common/GameEngine/Source/Common/INI/INITerrain.cpp
    Common/GameEngine/Source/GameClient/Drawable/Update/AnimatedParticleSysBoneClientUpdate.cpp
    Common/GameEngine/Source/GameClient/Drawable/Update/BeaconClientUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/SimpleObjectIterator.cpp
    Common/GameEngine/Source/GameLogic/Object/ObjectTypes.cpp
    Common/GameEngine/Source/GameLogic/Object/GhostObject.cpp
    Common/GameEngine/Source/GameLogic/Object/ExperienceTracker.cpp
    Common/GameEngine/Source/GameLogic/System/CaveSystem.cpp
    Common/GameEngine/Source/GameLogic/System/RankInfo.cpp
    Common/GameEngine/Source/GameLogic/AI/Squad.cpp
    Common/GameEngine/Source/Common/RTS/ScoreKeeper.cpp
    Common/GameEngine/Source/Common/RTS/PlayerList.cpp
    Common/GameEngine/Source/Common/RTS/Energy.cpp
    Common/GameEngine/Source/GameClient/GUI/Shell/ShellMenuScheme.cpp
    Common/GameEngine/Source/GameClient/GUI/ControlBar/ControlBarResizer.cpp
    Common/GameEngine/Source/GameClient/GUI/ControlBar/ControlBarUnderConstruction.cpp
    Common/GameEngine/Source/GameClient/GUI/ControlBar/ControlBarObserver.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/InGamePopupMessage.cpp
    Common/GameEngine/Source/GameClient/GUI/EstablishConnectionsMenu/EstablishConnectionsMenu.cpp
    Common/GameEngine/Source/GameClient/GUI/Gadget/GadgetComboBox.cpp
    Common/GameEngine/Source/GameClient/GUI/Gadget/GadgetListBox.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/WeaponBonusUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/RadarUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Body/ImmortalBody.cpp
    Common/GameEngine/Source/GameLogic/Object/Body/InactiveBody.cpp
    Common/GameEngine/Source/GameLogic/Object/Die/CrushDie.cpp
    Common/GameEngine/Source/GameLogic/Object/Die/KeepObjectDie.cpp
    Common/GameEngine/Source/GameLogic/Object/Die/UpgradeDie.cpp
    Common/GameEngine/Source/GameLogic/Object/Die/CreateCrateDie.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/InstantDeathBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/BridgeTowerBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/FireWeaponWhenDamagedBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/BridgeScaffoldBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Helper/ObjectSMCHelper.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/DeletionUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/ToppleUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/LifetimeUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/UpdateModule.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/AutoDepositUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/DynamicShroudClearingRangeUpdate.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/LanMapSelectMenu.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/PopupReplay.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/DifficultySelect.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/AIUpdate/WanderAIUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/DockUpdate/SupplyWarehouseDockUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/ProductionExitUpdate/DefaultProductionExitUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/ProductionExitUpdate/QueueProductionExitUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/ProductionExitUpdate/SpawnPointProductionExitUpdate.cpp)

list(TRANSFORM SAGE_STUB_UNCOMMONIZED_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/")
set_source_files_properties(
  ${SAGE_STUB_UNCOMMONIZED_SOURCES}
  TARGET_DIRECTORY c_gameenginestub
  PROPERTIES HEADER_FILE_ONLY ON)
