ADD_CORE_DIRECTORY_EXCEPT(ConvolutionClustering*.*)

INCLUDE_DIRECTORIES(${TulipCoreBuildInclude} ${TulipCoreInclude} ${CMAKE_CURRENT_BINARY_DIR})

ADD_SUBDIRECTORY(Delaunay)

##----------------------------------------------------------------------------------------------------------------------------
SET(StrengthClustering_SRCS
  StrengthClustering.h
  StrengthClustering.cpp)

ADD_LIBRARY(StrengthClustering-${TulipVersion} SHARED ${StrengthClustering_SRCS})
TARGET_LINK_LIBRARIES(StrengthClustering-${TulipVersion} ${LibTulipCoreName})

##----------------------------------------------------------------------------------------------------------------------------
SET(HierarchicalClustering_SRCS
  HierarchicalClustering.cpp
  HierarchicalClustering.h)

ADD_LIBRARY(HierarchicalClustering-${TulipVersion} SHARED ${HierarchicalClustering_SRCS})
TARGET_LINK_LIBRARIES(HierarchicalClustering-${TulipVersion} ${LibTulipCoreName})

##----------------------------------------------------------------------------------------------------------------------------
IF(NOT BUILD_CORE_ONLY)
QTX_WRAP_UI(ConvolutionClustering_UI_SRCS
  ConvolutionClusteringSetup.ui)

QTX_WRAP_CPP(ConvolutionClustering_MOC_SRCS
  ConvolutionClusteringSetup.h)

SET(ConvolutionClustering_SRCS
  ConvolutionClustering.h
  ConvolutionClustering.cpp
  ConvolutionClusteringSetup.cpp
  ConvolutionClusteringSetup.h)

ADD_LIBRARY(ConvolutionClustering-${TulipVersion} SHARED ${ConvolutionClustering_SRCS} ${ConvolutionClustering_UI_SRCS} ${ConvolutionClustering_MOC_SRCS})
TARGET_LINK_LIBRARIES(ConvolutionClustering-${TulipVersion} ${LibTulipCoreName} ${QT_LIBRARIES})
ENDIF(NOT BUILD_CORE_ONLY)
##----------------------------------------------------------------------------------------------------------------------------
SET(EqualValueClustering_SRCS
  EqualValueClustering.h
  EqualValueClustering.cpp)

ADD_LIBRARY(EqualValueClustering-${TulipVersion} SHARED ${EqualValueClustering_SRCS})
TARGET_LINK_LIBRARIES(EqualValueClustering-${TulipVersion} ${LibTulipCoreName})

##----------------------------------------------------------------------------------------------------------------------------
SET(QuotientClustering_SRCS
    QuotientClustering.cpp
)

ADD_LIBRARY(QuotientClustering-${TulipVersion} SHARED ${QuotientClustering_SRCS})
TARGET_LINK_LIBRARIES(QuotientClustering-${TulipVersion} ${LibTulipCoreName})

##----------------------------------------------------------------------------------------------------------------------------
INSTALL_TULIP_PLUGIN(StrengthClustering-${TulipVersion} ${TulipPluginsInstallDir})
INSTALL_TULIP_PLUGIN(HierarchicalClustering-${TulipVersion} ${TulipPluginsInstallDir})
IF(NOT BUILD_CORE_ONLY)
INSTALL_TULIP_PLUGIN(ConvolutionClustering-${TulipVersion} ${TulipPluginsInstallDir})
ENDIF(NOT BUILD_CORE_ONLY)
INSTALL_TULIP_PLUGIN(EqualValueClustering-${TulipVersion} ${TulipPluginsInstallDir})
INSTALL_TULIP_PLUGIN(QuotientClustering-${TulipVersion} ${TulipPluginsInstallDir})
