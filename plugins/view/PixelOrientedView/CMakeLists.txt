<<<<<<< HEAD
INCLUDE_DIRECTORIES(${TulipCoreInclude} ${TulipCoreBuildInclude} ${TulipOGLESInclude} ${TulipGUIInclude} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${GLEW_INCLUDE_DIR})
=======
INCLUDE_DIRECTORIES(${TulipCoreBuildInclude} ${TulipCoreInclude} ${TulipOGLInclude} ${TulipGUIInclude} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${GLEW_INCLUDE_DIR})
>>>>>>> 530af02... ensure Tulip build include folders are searched before the source include ones : prevent possible bad compilation surprises when accidentally running the cmake command in the tulip root source folder

SET(LIB_SRCS
    PixelOrientedView.cpp
    PixelOrientedOverview.cpp
    PixelOrientedViewNavigator.cpp
    PixelOrientedInteractors.cpp
    PixelOrientedOptionsWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/ViewGraphPropertiesSelectionWidget.cpp
    POLIB/FishEyesScreen.cpp
    POLIB/FishEyesScreenFurnas.cpp
    POLIB/HilbertLayout.cpp
    POLIB/HSIColorMapping.cpp
    POLIB/HSIColorSpace.cpp
    POLIB/LinearMappingColor.cpp
    POLIB/LogarithmicMappingColor.cpp
    POLIB/PixelOrientedMediator.cpp
    POLIB/SpiralLayout.cpp
    POLIB/SquareLayout.cpp
    POLIB/TulipGraphDimension.cpp
    POLIB/TulipNodeColorMapping.cpp
    POLIB/TulipNodeMetricSorter.cpp
    POLIB/UniformDeformationScreen.cpp
    POLIB/ZOrderLayout.cpp
)

QTX_WRAP_UI(LIB_UI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/ViewGraphPropertiesSelectionWidget.ui
    PixelOrientedOptionsWidget.ui
)

QTX_WRAP_CPP(LIB_MOC_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/ViewGraphPropertiesSelectionWidget.h
    PixelOrientedView.h
    PixelOrientedOptionsWidget.h
)

QTX_ADD_RESOURCES(RCC_SRCS PixelOrientedView.qrc)

ADD_LIBRARY(PixelOrientedView-${TulipVersion} SHARED ${LIB_SRCS} ${LIB_UI_HEADERS} ${LIB_MOC_SRCS} ${RCC_SRCS})
TARGET_LINK_LIBRARIES(PixelOrientedView-${TulipVersion} ${LibTulipCoreName} ${LibTulipOGLESName} ${LibTulipGUIName} ${OPENGL_gl_LIBRARY})

INSTALL_TULIP_PLUGIN(PixelOrientedView-${TulipVersion} ${TulipViewPluginsInstallDir})
