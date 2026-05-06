# Target setup
add_library(g_wwdownload)

target_sources(
  g_wwdownload
  PRIVATE Download.cpp
          Download.h
          DownloadDebug.h
          downloaddefs.h
          FTP.CPP
          ftp.h
          ftpdefs.h
          Registry.h
          registry.cpp
          urlBuilder.cpp
          urlBuilder.h)

set_target_properties(g_wwdownload PROPERTIES OUTPUT_NAME "WWDownload")

target_include_directories(g_wwdownload PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(g_wwdownload PUBLIC g_wwlib)
