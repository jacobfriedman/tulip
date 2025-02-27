
SET(SRCS
  InteractorConfigWidget.cpp
  ColorScaleConfigDialog.cpp
  CopyPropertyDialog.cpp
  CSVGraphImport.cpp
  CSVGraphMappingConfigurationWidget.cpp
  CSVImportConfigurationWidget.cpp
  CSVImportWizard.cpp
  CSVParserConfigurationWidget.cpp
  CSVParser.cpp
  DoubleStringsListSelectionWidget.cpp
  DownloadManager.cpp
  DragHandle.cpp
  GlCompositeHierarchyManager.cpp
  GlMainView.cpp
  GlMainWidget.cpp
  GlOffscreenRenderer.cpp
  GraphPropertiesSelectionWidget.cpp
  ItemsListWidget.cpp
  MouseBoxZoomer.cpp
  MouseEdgeBendEditor.cpp
  MouseEdgeBuilder.cpp
  MouseInteractors.cpp
  MouseNodeBuilder.cpp
  MouseSelectionEditor.cpp
  MouseSelector.cpp
  MouseShowElementInfo.cpp
  NodeLinkDiagramComponent.cpp
  NodeLinkDiagramComponentInteractor.cpp
  ShapeDialog.cpp
  Perspective.cpp
  PluginManager.cpp
  PropertyCreationDialog.cpp
  QtGlSceneZoomAndPanAnimator.cpp
  QuaZIPFacade.cpp
  RenamePropertyDialog.cpp
  SimplePluginProgressWidget.cpp
  SimpleStringsListSelectionWidget.cpp
  RangeSlider.cpp
  StringsListSelectionWidget.cpp
  StringsListSelectionDialog.cpp
  TextureFileDialog.cpp
  TlpQtTools.cpp
  TulipProject.cpp
  TulipSettings.cpp
  ParameterListModel.cpp
  TulipComboBox.cpp
  TulipItemDelegate.cpp
  TulipMetaTypes.cpp
  TulipItemEditorCreators.cpp
  ColorButton.cpp
  TulipModel.cpp
  ColorScaleButton.cpp
  View.cpp
  GlMainWidgetGraphicsItem.cpp
  ViewWidget.cpp
  InteractorComposite.cpp
  GLInteractor.cpp
  WorkspacePanel.cpp
  Interactor.cpp
  ProcessingAnimationItem.cpp
  Workspace.cpp
  PlaceHolderWidget.cpp
  GraphHierarchiesModel.cpp
  CoordEditor.cpp
  GlOverviewGraphicsItem.cpp
  SceneConfigWidget.cpp
  QuickAccessBar.cpp
  TulipFont.cpp
  TulipFontDialog.cpp
  ScrollPopupButton.cpp
  WorkspaceExposeWidget.cpp
  GraphTableItemDelegate.cpp
  StringEditor.cpp
  VectorEditor.cpp
  GlyphRenderer.cpp
  ClearableLineEdit.cpp
  CaptionItem.cpp
  CaptionGraphicsItem.cpp
  CaptionGraphicsSubItems.cpp
  GraphModel.cpp
  GraphElementModel.cpp
  SnapshotDialog.cpp
  TulipMimes.cpp
  SceneLayersModel.cpp
  SceneLayersConfigWidget.cpp
  TreeViewComboBox.cpp
  GlSimpleEntityItemModel.cpp
  PluginModel.cpp
  GraphNeedsSavingObserver.cpp
  ColorScalesManager.cpp
  ScientificDoubleSpinBox.cpp
  AboutTulipPage.cpp
  TulipFontIconDialog.cpp
  TulipFontIconEngine.cpp
  ViewActionsManager.cpp
  ViewGraphPropertiesSelectionWidget.cpp
  ViewToolTipAndUrlManager.cpp
  PythonVersionChecker.cpp
)

QTX_SET_INCLUDES_AND_DEFINITIONS()

ADD_DEFINITIONS(-DQT_DLL)
ADD_DEFINITIONS(-DQT_THREAD_SUPPORT)

IF(TULIP_BUILD_PYTHON_COMPONENTS)
  SET(PYTHON_PART_SRCS PythonVersionChecker.cpp)
  ADD_DEFINITIONS("-DTULIP_BUILD_PYTHON_COMPONENTS")
  SET(TLPGUI_PYTHON_COMPILE_DEFINITIONS TLP_PYTHON="${PYTHON_VERSION}" SIP_VERSION="${SIP_VERSION_STR}")
  IF(MSYS2_PYTHON)
    SET(TLPGUI_PYTHON_COMPILE_DEFINITIONS ${TLPGUI_PYTHON_COMPILE_DEFINITIONS} MSYS2_PYTHON=1 PYTHON_HOME_PATH="${PYTHON_HOME_PATH}/../")
  ENDIF(MSYS2_PYTHON)
  SET_SOURCE_FILES_PROPERTIES(PythonVersionChecker.cpp
                              PROPERTIES COMPILE_DEFINITIONS "${TLPGUI_PYTHON_COMPILE_DEFINITIONS}")
  SET_SOURCE_FILES_PROPERTIES(AboutTulipPage.cpp
                              PROPERTIES COMPILE_DEFINITIONS "${TLPGUI_PYTHON_COMPILE_DEFINITIONS}")
ENDIF(TULIP_BUILD_PYTHON_COMPONENTS)

QTX_WRAP_UI(UI_HEADERS
  ../designer/AboutTulipPage.ui
  ../designer/ColorScaleConfigDialog.ui
  ../designer/CoordEditor.ui
  ../designer/CopyPropertyDialog.ui
  ../designer/CSVGraphMappingConfigurationWidget.ui
  ../designer/CSVImportConfigurationWidget.ui
  ../designer/CSVImportWizard.ui
  ../designer/CSVParserConfigurationWidget.ui
  ../designer/CSVPropertyDialog.ui
  ../designer/DoubleStringsListSelectionWidget.ui
  ../designer/ElementInformationWidget.ui
  ../designer/GridOptionsWidget.ui
  ../designer/ManageGraphEltTooltipDialog.ui
  ../designer/ShapeDialog.ui
  ../designer/PropertyCreationDialog.ui
  ../designer/QuickAccessBar.ui
  ../designer/SceneConfigWidget.ui
  ../designer/SceneLayersConfigWidget.ui
  ../designer/SimplePluginProgressWidget.ui
  ../designer/SimpleStringsListSelectionWidget.ui
  ../designer/SnapshotDialog.ui
  ../designer/StringsListSelectionDialog.ui
  ../designer/TextureFileDialog.ui
  ../designer/TulipFontDialog.ui
  ../designer/TulipFontIconDialog.ui
  ../designer/VectorEditor.ui
  ../designer/Workspace.ui
  ../designer/WorkspacePanel.ui
  ../designer/InteractorConfigWidget.ui
  ../designer/ViewGraphPropertiesSelectionWidget.ui
)

QTX_WRAP_CPP(MOC_SRCS
  ../include/tulip/AboutTulipPage.h
  ../include/tulip/CaptionGraphicsItem.h
  ../include/tulip/CaptionGraphicsSubItems.h
  ../include/tulip/CaptionItem.h
  ../include/tulip/ColorButton.h
  ../include/tulip/ColorScaleButton.h
  ../include/tulip/ColorScaleConfigDialog.h
  ../include/tulip/CoordEditor.h
  ../include/tulip/CopyPropertyDialog.h
  ../include/tulip/CSVGraphMappingConfigurationWidget.h
  ../include/tulip/CSVImportConfigurationWidget.h
  ../include/tulip/CSVImportWizard.h
  ../include/tulip/CSVParserConfigurationWidget.h
  ../include/tulip/DoubleStringsListSelectionWidget.h
  ../include/tulip/DownloadManager.h
  ../include/tulip/DragHandle.h
  ../include/tulip/GLInteractor.h
  ../include/tulip/GlMainView.h
  ../include/tulip/GlMainWidget.h
  ../include/tulip/GlMainWidgetGraphicsItem.h
  ../include/tulip/GlOverviewGraphicsItem.h
  ../include/tulip/GlSimpleEntityItemModel.h
  ../include/tulip/GraphHierarchiesModel.h
  ../include/tulip/GraphNeedsSavingObserver.h
  ../include/tulip/GraphTableItemDelegate.h
  ../include/tulip/Interactor.h
  ../include/tulip/InteractorComposite.h
  ../include/tulip/ItemsListWidget.h
  ../include/tulip/NodeLinkDiagramComponent.h
  ../include/tulip/Perspective.h
  ../include/tulip/PlaceHolderWidget.h
  ../include/tulip/ProcessingAnimationItem.h
  ../include/tulip/PropertyCreationDialog.h
  ../include/tulip/QtGlSceneZoomAndPanAnimator.h
  ../include/tulip/QuickAccessBar.h
  ../include/tulip/SceneConfigWidget.h
  ../include/tulip/SceneLayersModel.h
  ../include/tulip/SceneLayersConfigWidget.h
  ../include/tulip/ScrollPopupButton.h
  ../include/tulip/SimplePluginProgressWidget.h
  ../include/tulip/SimpleStringsListSelectionWidget.h
  ../include/tulip/SnapshotDialog.h
  ../include/tulip/RangeSlider.h
  ../include/tulip/MouseShowElementInfo.h
  ../include/tulip/StringsListSelectionDialog.h
  ../include/tulip/TextureFileDialog.h
  ../include/tulip/TreeViewComboBox.h
  ../include/tulip/TulipFont.h
  ../include/tulip/TulipFontDialog.h
  ../include/tulip/TulipItemDelegate.h
  ../include/tulip/TulipMimes.h
  ../include/tulip/TulipModel.h
  ../include/tulip/TulipProject.h
  ../include/tulip/TulipSettings.h
  ../include/tulip/VectorEditor.h
  ../include/tulip/View.h
  ../include/tulip/ViewWidget.h
  ../include/tulip/WorkspacePanel.h
  ../include/tulip/Workspace.h
  ../include/tulip/WorkspaceExposeWidget.h
  ../include/tulip/TulipFontIconDialog.h
  ../include/tulip/ShapeDialog.h
  ../include/tulip/ViewActionsManager.h
  ../include/tulip/ViewToolTipAndUrlManager.h
  ../include/tulip/ViewGraphPropertiesSelectionWidget.h
)

QTX_ADD_RESOURCES(RCC_SRCS ../resources/TulipGUIResource.qrc)

INCLUDE_DIRECTORIES(${TulipGUIBuildInclude} ${TulipCoreBuildInclude} ${TulipCoreInclude} ${TulipOGLInclude} ${TulipGUIInclude} ${CMAKE_CURRENT_BINARY_DIR} ${ZLIB_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${QuazipInclude} ${OGDFInclude} ${OGDFBuildInclude})
ADD_LIBRARY(${LibTulipGUIName} SHARED ${SRCS} ${MOC_SRCS} ${RCC_SRCS} ${UI_HEADERS})
TARGET_LINK_LIBRARIES(${LibTulipGUIName} ${QT_LIBRARIES} ${LibTulipCoreName} ${LibTulipOGLName} ${QuazipLibrary})

INSTALL(TARGETS ${LibTulipGUIName}
  RUNTIME DESTINATION ${TulipBinInstallDir} COMPONENT tulip_gui
  LIBRARY DESTINATION ${TulipLibInstallDir} COMPONENT tulip_gui
  ARCHIVE DESTINATION ${TulipLibInstallDir} COMPONENT tulip_gui)

INSTALL(FILES ../resources/icons/tulip.ico DESTINATION ${TulipBitmapInstallDir})
INSTALL(FILES ../resources/icons/tulip-file-icon.ico DESTINATION ${TulipBitmapInstallDir})
