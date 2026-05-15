# Target setup
add_library(c_wwdownload)

target_sources(
  c_wwdownload
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

set_target_properties(c_wwdownload PROPERTIES OUTPUT_NAME "WWDownload")

target_include_directories(c_wwdownload PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(c_wwdownload PUBLIC c_wwlib)
