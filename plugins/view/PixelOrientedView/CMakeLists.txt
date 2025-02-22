INCLUDE_DIRECTORIES(${TulipCoreBuildInclude} ${TulipCoreInclude} ${TulipOGLInclude} ${TulipGUIInclude} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${GLEW_INCLUDE_DIR})

SET(LIB_SRCS
    PixelOrientedView.cpp
    PixelOrientedOverview.cpp
    PixelOrientedViewNavigator.cpp
    PixelOrientedInteractors.cpp
    PixelOrientedOptionsWidget.cpp
    PixelOrientedViewQuickAccessBar.cpp
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
    PixelOrientedOptionsWidget.ui
)

QTX_WRAP_CPP(LIB_MOC_SRCS
    PixelOrientedView.h
    PixelOrientedOptionsWidget.h
    PixelOrientedViewQuickAccessBar.h
)

QTX_ADD_RESOURCES(RCC_SRCS PixelOrientedView.qrc)

ADD_LIBRARY(PixelOrientedView-${TulipVersion} SHARED ${LIB_SRCS} ${LIB_UI_HEADERS} ${LIB_MOC_SRCS} ${RCC_SRCS})
TARGET_LINK_LIBRARIES(PixelOrientedView-${TulipVersion} ${LibTulipCoreName} ${LibTulipOGLName} ${LibTulipGUIName} ${OPENGL_gl_LIBRARY})

TULIP_INSTALL_PLUGIN(PixelOrientedView-${TulipVersion} ${TulipViewPluginsInstallDir})
