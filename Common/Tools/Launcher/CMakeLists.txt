find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 QuickDialogs2 Network Svg LinguistTools)

set(CMAKE_AUTOMOC ON)

# Our .qml files live in the qml/ subdirectory; NEW makes Qt generate the
# per-directory qmldir files itself instead of warning about them (QTP0004,
# introduced in Qt 6.8).
if(QT_KNOWN_POLICY_QTP0004)
  qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(
  c_launcher
  Main.cpp
  Installation.h
  Installation.cpp
  InstallationModel.h
  InstallationModel.cpp
  InstallationFinder.h
  InstallationFinder.cpp
  GameCatalog.h
  GameCatalogModel.h
  GameCatalogModel.cpp
  LauncherConfig.h
  LauncherConfig.cpp
  SteamCmdManager.h
  SteamCmdManager.cpp
  GameLauncher.h
  GameLauncher.cpp)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(
  c_launcher
  URI SAGELauncher
  VERSION 1.0
  RESOURCE_PREFIX /qt/qml
  NO_IMPORT_SCAN
  QML_FILES qml/Main.qml qml/GameCard.qml qml/Theme.qml qml/SteamLoginDialog.qml qml/TitleBar.qml qml/AppButton.qml qml/AppSettingsDialog.qml
  RESOURCES assets/covers/generals.jpg
            assets/covers/zerohour.jpg
            assets/covers/bfme1.jpg
            assets/covers/bfme2.jpg
            assets/covers/rotwk.jpg
            assets/covers/placeholder.svg
            assets/icons/steam-icon.svg
            assets/icons/steam-wordmark.svg
            assets/icons/refresh.svg
            assets/icons/play.svg)

qt_add_translations(
  c_launcher
  TS_FILES translations/launcher_de.ts
  RESOURCE_PREFIX "/i18n"
  SOURCES qml/Main.qml qml/GameCard.qml qml/SteamLoginDialog.qml qml/AppSettingsDialog.qml SteamCmdManager.cpp GameLauncher.cpp)

set_target_properties(c_launcher PROPERTIES OUTPUT_NAME "Launcher")
target_include_directories(c_launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(c_launcher PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::QuickDialogs2 Qt6::Network Qt6::Svg)
