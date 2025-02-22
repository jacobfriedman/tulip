INCLUDE_DIRECTORIES(${TulipCoreBuildInclude} ${TulipCoreInclude} ${TulipOGLInclude} ${TulipGUIInclude} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${GLEW_INCLUDE_DIR})

SET(LIB_SRCS
    HistogramView.cpp
    HistoStatsConfigWidget.cpp
    HistogramMetricMapping.cpp
    HistogramStatistics.cpp    
    Histogram.cpp
    HistogramViewNavigator.cpp
    HistogramInteractors.cpp
    HistoOptionsWidget.cpp
    SizeScaleConfigDialog.cpp
    GlyphScaleConfigDialog.cpp
)

QTX_WRAP_UI(LIB_UI_HEADERS
	HistoStatsConfigWidget.ui
	HistoOptionsWidget.ui
	SizeScaleConfigDialog.ui
	GlyphScaleConfigDialog.ui
)

QTX_WRAP_CPP(LIB_MOC_SRCS
      HistogramView.h
      HistogramStatistics.h
      HistoOptionsWidget.h
      SizeScaleConfigDialog.h
      GlyphScaleConfigDialog.h
      HistoStatsConfigWidget.h
)

QTX_ADD_RESOURCES(LIB_RCC_SRCS
HistogramResource.qrc
)

ADD_LIBRARY(HistogramView-${TulipVersion} SHARED ${LIB_SRCS} ${LIB_UI_HEADERS} ${LIB_MOC_SRCS} ${LIB_RCC_SRCS})
TARGET_LINK_LIBRARIES(HistogramView-${TulipVersion} ${LibTulipCoreName} ${LibTulipOGLName} ${LibTulipGUIName} ${OPENGL_gl_LIBRARY})

TULIP_INSTALL_PLUGIN(HistogramView-${TulipVersion} ${TulipViewPluginsInstallDir})
