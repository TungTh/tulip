SET(SRCS
  src/main.cpp
  src/PerspectiveItemWidget.cpp
  src/PluginErrorReport.cpp
  src/PluginLoaderDispatcher.cpp
  src/PluginLoaderReporter.cpp
  src/PluginsCenter.cpp
  src/PluginInformationsListItem.cpp
  src/TulipMainWindow.cpp
  src/TulipSplashScreen.cpp
  src/TulipWelcomePage.cpp
  src/TulipPerspectiveProcessHandler.cpp
  src/TulipPerspectiveCrashHandler.cpp
  src/FormPost.cpp
  src/PerspectiveSelectionDialog.cpp
)

QTX_WRAP_UI(UI_HEADERS
  designer/PerspectiveItem.ui
  designer/PluginErrorReport.ui
  designer/PluginsCenter.ui
  designer/TulipMainWindow.ui
  designer/TulipWelcomePage.ui
  designer/PluginInformationsListItem.ui
  designer/TulipPerspectiveCrashHandler.ui
  designer/PerspectiveSelectionDialog.ui
)

QTX_WRAP_CPP(MOC_SRCS
  include/PerspectiveItemWidget.h
  include/PluginErrorReport.h
  include/PluginsCenter.h
  include/TulipMainWindow.h
  include/TulipWelcomePage.h
  include/PluginInformationsListItem.h
  include/TulipPerspectiveProcessHandler.h
  include/TulipPerspectiveCrashHandler.h
  include/FormPost.h
  include/PerspectiveSelectionDialog.h
)

QTX_ADD_RESOURCES(RCC_SRCS resources/TulipAgent.qrc)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../crash_handling)

INCLUDE_DIRECTORIES(include ${CMAKE_CURRENT_BINARY_DIR} ${TulipCoreBuildInclude} ${TulipCoreInclude} ${TulipOGLESInclude} ${TulipGUIInclude})

IF(USE_QT5)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
ENDIF(USE_QT5)

IF(BUILD_PYTHON_COMPONENTS)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_PYTHON_COMPONENTS")
ENDIF(BUILD_PYTHON_COMPONENTS)

IF(WIN32)

STRING(TIMESTAMP CURRENT_YEAR "%Y")

INCLUDE(generate_product_version)
generate_product_version(
   VersionFiles
   NAME "Tulip Agent"
   ICON ${PROJECT_SOURCE_DIR}/library/tulip-gui/resources/icons/tulip.ico
   VERSION_MAJOR ${TulipMajorVersion}
   VERSION_MINOR ${TulipMinorVersion}
   VERSION_PATCH ${TulipReleaseVersion}
   COMPANY_NAME "LaBRI, University of Bordeaux"
   COMPANY_COPYRIGHT "(C) Copyright ${CURRENT_YEAR} David Auber and the Tulip development Team"
   ORIGINAL_FILENAME "tulip.exe"
)

SET(SRCS ${SRCS} ${VersionFiles})

ENDIF(WIN32)

ADD_EXECUTABLE(tulip ${SRCS} ${UI_HEADERS} ${MOC_SRCS} ${RCC_SRCS})

TARGET_LINK_LIBRARIES(tulip crash_handling ${LibTulipCoreName} ${LibTulipOGLESName} ${LibTulipGUIName} ${QT_LIBRARIES})

INSTALL(TARGETS tulip
  RUNTIME DESTINATION ${TulipBinInstallDir} COMPONENT tulip_app
  LIBRARY DESTINATION ${TulipLibInstallDir} COMPONENT tulip_app
  ARCHIVE DESTINATION ${TulipLibInstallDir} COMPONENT tulip_app)
