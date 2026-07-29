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
    Common/GameEngine/Source/GameLogic/Object/Update/ProductionExitUpdate/SpawnPointProductionExitUpdate.cpp
    Common/GameEngineDevice/Source/MiniAudioDevice/MiniAudioManager.cpp
    Common/GameEngineDevice/Source/SDL3Device/Common/SDL3GameEngine.cpp
    Common/GameEngineDevice/Source/SDL3Device/Common/SDL3IMEManager.cpp
    Common/GameEngineDevice/Source/SDL3Device/GameClient/SDL3Keyboard.cpp
    Common/GameEngineDevice/Source/SDL3Device/GameClient/SDL3Mouse.cpp
    Common/GameEngineDevice/Source/W3DDevice/Common/Thing/W3DThingFactory.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DDebrisDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DDefaultDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DOverlordTankDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DPoliceCarDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DProjectileStreamDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DRopeDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DScienceModelDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DSupplyDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DTankDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DTankTruckDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DTracerDraw.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DCheckBox.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DComboBox.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DHorizontalSlider.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DListBox.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DProgressBar.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DRadioButton.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DTabControl.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DTextEntry.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DVerticalSlider.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/GUICallbacks/W3DControlBar.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/GUICallbacks/W3DMOTD.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/W3DGameWindow.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/GUI/W3DGameWindowManager.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/W3DDisplayString.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/W3DInGameUI.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/W3DRoadBuffer.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/W3DSnow.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/W3DStatusCircle.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/W3DTerrainBackground.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/W3DTerrainTracks.cpp
    Common/GameEngineDevice/Source/W3DDevice/GameClient/W3DWebBrowser.cpp
    Common/GameEngine/Source/Common/Audio/GameSounds.cpp
    Common/GameEngine/Source/Common/DamageFX.cpp
    Common/GameEngine/Source/Common/INI/INIAiData.cpp
    Common/GameEngine/Source/Common/INI/INICommandSet.cpp
    Common/GameEngine/Source/Common/INI/INIObject.cpp
    Common/GameEngine/Source/Common/INI/INIParticleSys.cpp
    Common/GameEngine/Source/Common/INI/INISpecialPower.cpp
    Common/GameEngine/Source/Common/RTS/Handicap.cpp
    Common/GameEngine/Source/Common/RTS/MissionStats.cpp
    Common/GameEngine/Source/Common/RTS/ResourceGatheringManager.cpp
    Common/GameEngine/Source/Common/System/CDManager.cpp
    Common/GameEngine/Source/Common/System/Compression.cpp
    Common/GameEngine/Source/Common/System/QuickTrig.cpp
    Common/GameEngine/Source/Common/System/Snapshot.cpp
    Common/GameEngine/Source/Common/System/XferLoad.cpp
    Common/GameEngine/Source/Common/TerrainTypes.cpp
    Common/GameEngine/Source/Common/Thing/DrawModule.cpp
    Common/GameEngine/Source/GameClient/GameClientDispatch.cpp
    Common/GameEngine/Source/GameClient/GUI/ControlBar/ControlBarBeacon.cpp
    Common/GameEngine/Source/GameClient/GUI/Gadget/GadgetHorizontalSlider.cpp
    Common/GameEngine/Source/GameClient/GUI/Gadget/GadgetProgressBar.cpp
    Common/GameEngine/Source/GameClient/GUI/Gadget/GadgetRadioButton.cpp
    Common/GameEngine/Source/GameClient/GUI/Gadget/GadgetStaticText.cpp
    Common/GameEngine/Source/GameClient/GUI/Gadget/GadgetTabControl.cpp
    Common/GameEngine/Source/GameClient/GUI/Gadget/GadgetVerticalSlider.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/ExtendedMessageBox.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/GeneralsExpPoints.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/IMECandidate.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/DownloadMenu.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/EstablishConnectionsWindow.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/GameInfoWindow.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/NetworkDirectConnect.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/PopupLadderSelect.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/WOLCustomScoreScreen.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/WOLLocaleSelectPopup.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/MessageBox.cpp
    Common/GameEngine/Source/GameClient/GUI/GUICallbacks/ReplayControls.cpp
    Common/GameEngine/Source/GameClient/Snow.cpp
    Common/GameEngine/Source/GameClient/System/RayEffect.cpp
    Common/GameEngine/Source/GameLogic/AI/AIDock.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/BehaviorModule.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/OverchargeBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/POWTruckBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/PrisonBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/PropagandaCenterBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Behavior/SupplyWarehouseCripplingBehavior.cpp
    Common/GameEngine/Source/GameLogic/Object/Body/HighlanderBody.cpp
    Common/GameEngine/Source/GameLogic/Object/Body/StructureBody.cpp
    Common/GameEngine/Source/GameLogic/Object/Collide/CollideModule.cpp
    Common/GameEngine/Source/GameLogic/Object/Collide/CrateCollide/HealCrateCollide.cpp
    Common/GameEngine/Source/GameLogic/Object/Collide/CrateCollide/MoneyCrateCollide.cpp
    Common/GameEngine/Source/GameLogic/Object/Collide/CrateCollide/ShroudCrateCollide.cpp
    Common/GameEngine/Source/GameLogic/Object/Collide/CrateCollide/UnitCrateCollide.cpp
    Common/GameEngine/Source/GameLogic/Object/Collide/CrateCollide/VeterancyCrateCollide.cpp
    Common/GameEngine/Source/GameLogic/Object/Collide/SquishCollide.cpp
    Common/GameEngine/Source/GameLogic/Object/Contain/HealContain.cpp
    Common/GameEngine/Source/GameLogic/Object/Contain/RailedTransportContain.cpp
    Common/GameEngine/Source/GameLogic/Object/Create/CreateModule.cpp
    Common/GameEngine/Source/GameLogic/Object/Create/PreorderCreate.cpp
    Common/GameEngine/Source/GameLogic/Object/Create/SpecialPowerCreate.cpp
    Common/GameEngine/Source/GameLogic/Object/Create/SupplyCenterCreate.cpp
    Common/GameEngine/Source/GameLogic/Object/Create/SupplyWarehouseCreate.cpp
    Common/GameEngine/Source/GameLogic/Object/Create/VeterancyGainCreate.cpp
    Common/GameEngine/Source/GameLogic/Object/Damage/BoneFXDamage.cpp
    Common/GameEngine/Source/GameLogic/Object/Damage/DamageModule.cpp
    Common/GameEngine/Source/GameLogic/Object/Destroy/DestroyModule.cpp
    Common/GameEngine/Source/GameLogic/Object/Die/DamDie.cpp
    Common/GameEngine/Source/GameLogic/Object/Die/EjectPilotDie.cpp
    Common/GameEngine/Source/GameLogic/Object/Die/SpecialPowerCompletionDie.cpp
    Common/GameEngine/Source/GameLogic/Object/SpecialPower/CashBountyPower.cpp
    Common/GameEngine/Source/GameLogic/Object/SpecialPower/CashHackSpecialPower.cpp
    Common/GameEngine/Source/GameLogic/Object/SpecialPower/CleanupAreaPower.cpp
    Common/GameEngine/Source/GameLogic/Object/SpecialPower/DefectorSpecialPower.cpp
    Common/GameEngine/Source/GameLogic/Object/SpecialPower/SpyVisionSpecialPower.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/AIUpdate/POWTruckAIUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/AIUpdate/RailedTransportAIUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/AutoFindHealingUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/BaseRenerateUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/CheckpointUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/CleanupHazardUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/DockUpdate/PrisonDockUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/DockUpdate/RepairDockUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/DynamicGeometryInfoUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/EnemyNearUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/FloatUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/HeightDieUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/NeutronMissileSlowDeathUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/NeutronMissileUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/PilotFindVehicleUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/PowerPlantUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/ProneUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/RadarUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/RadiusDecalUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/TensileFormationUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Update/WaveGuideUpdate.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/ActiveShroudUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/CommandSetUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/CostModifierUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/ExperienceScalarUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/LocomotorSetUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/MaxHealthUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/ObjectCreationUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/PowerPlantUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/UnpauseSpecialPowerUpgrade.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/UpgradeModule.cpp
    Common/GameEngine/Source/GameLogic/Object/Upgrade/WeaponSetUpgrade.cpp
    Common/GameEngine/Source/GameNetwork/DisconnectManager.cpp
    Common/GameEngine/Source/GameNetwork/FileTransfer.cpp
    Common/GameEngine/Source/GameNetwork/FrameData.cpp
    Common/GameEngine/Source/GameNetwork/FrameMetrics.cpp
    Common/GameEngine/Source/GameNetwork/GameMessageParser.cpp
    Common/GameEngine/Source/GameNetwork/GameSpyOverlay.cpp
    Common/GameEngine/Source/GameNetwork/User.cpp)

list(TRANSFORM SAGE_STUB_UNCOMMONIZED_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/")
set_source_files_properties(
  ${SAGE_STUB_UNCOMMONIZED_SOURCES}
  TARGET_DIRECTORY c_gameenginestub
  PROPERTIES HEADER_FILE_ONLY ON)
