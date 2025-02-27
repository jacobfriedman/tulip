/**
 *
 * This file is part of Tulip (http://tulip.labri.fr)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */

#include <tulip/GlTextureManager.h>
#include <tulip/GlMainWidget.h>
#include <tulip/GlGraphComposite.h>
#include <tulip/GlQuantitativeAxis.h>
#include <tulip/GlLabel.h>
#include <tulip/GlLine.h>
#include <tulip/TlpQtTools.h>
#include <tulip/GraphProperty.h>
#include <tulip/TulipViewSettings.h>
#include <tulip/Perspective.h>
#include <tulip/ViewGraphPropertiesSelectionWidget.h>
#include <tulip/WorkspacePanel.h>

#include <QApplication>
#include <QAbstractButton>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QProgressDialog>

#include "ScatterPlot2DView.h"
#include "ScatterPlot2DOptionsWidget.h"
#include "ScatterPlot2D.h"
#include "ScatterPlot2DInteractors.h"
#include "ScatterPlotQuickAccessBar.h"

using namespace std;

const unsigned int OVERVIEWS_SIZE = 512;
const float OFFSET_BETWEEN_PREVIEWS = 16;

namespace tlp {

PLUGIN(ScatterPlot2DView)

class map_pair_string_key_contains
    : public unary_function<pair<pair<string, string>, ScatterPlot2D *>, bool> {

public:
  map_pair_string_key_contains(const string &pairValueToFind) : pairValueToFind(pairValueToFind) {}

  bool operator()(const pair<pair<string, string>, ScatterPlot2D *> &elem) const {
    string pairStringKeyFirst = elem.first.first;
    string pairStringKeySecond = elem.first.second;
    return (pairStringKeyFirst == pairValueToFind) || (pairStringKeySecond == pairValueToFind);
  }

private:
  string pairValueToFind;
};

const string propertiesTypes[] = {"double", "int"};
const unsigned int nbPropertiesTypes = sizeof(propertiesTypes) / sizeof(string);
const vector<string> propertiesTypesFilter(propertiesTypes, propertiesTypes + nbPropertiesTypes);

ScatterPlot2DView::ScatterPlot2DView(const PluginContext *)
    : GlMainView(true), propertiesSelectionWidget(nullptr), optionsWidget(nullptr),
      scatterPlotGraph(nullptr), emptyGraph(nullptr), mainLayer(nullptr), glGraphComposite(nullptr),
      scatterPlotSize(nullptr), matrixComposite(nullptr), axisComposite(nullptr),
      labelsComposite(nullptr), detailedScatterPlot(nullptr),
      detailedScatterPlotPropertyName(make_pair("", "")), center(false), matrixView(true),
      sceneRadiusBak(0.0), zoomFactorBak(0.0), matrixUpdateNeeded(false), newGraphSet(false),
      lastViewWindowWidth(0), lastViewWindowHeight(0), initialized(false),
      edgeAsNodeGraph(nullptr) {}

ScatterPlot2DView::~ScatterPlot2DView() {
  delete propertiesSelectionWidget;
  delete optionsWidget;
  delete glGraphComposite;
  delete matrixComposite;
  delete axisComposite;
  delete emptyGraph;
  delete edgeAsNodeGraph;
}

void ScatterPlot2DView::initGlWidget() {
  mainLayer = getGlMainWidget()->getScene()->getLayer("Main");

  if (mainLayer == nullptr) {
    mainLayer = new GlLayer("Main");
    getGlMainWidget()->getScene()->addExistingLayer(mainLayer);
  }

  cleanupGlScene();

  if (emptyGraph == nullptr) {
    emptyGraph = newGraph();
    glGraphComposite = new GlGraphComposite(emptyGraph);
  }

  mainLayer->addGlEntity(glGraphComposite, "graph");

  if (matrixComposite == nullptr) {
    matrixComposite = new GlComposite();
    mainLayer->addGlEntity(matrixComposite, "matrix composite");
  }

  if (axisComposite == nullptr) {
    axisComposite = new GlComposite();
    mainLayer->addGlEntity(axisComposite, "axis composite");
  }

  if (labelsComposite == nullptr) {
    labelsComposite = new GlComposite();
  }
}

void ScatterPlot2DView::cleanupGlScene() {

  if (axisComposite != nullptr) {
    axisComposite->reset(false);
  }

  if (labelsComposite != nullptr) {
    labelsComposite->reset(true);
  }

  if (matrixComposite != nullptr) {
    matrixComposite->reset(true);
    // labelsComposite was added as a GlEntity of matrixComposite
    // in buildScatterPlotsMatrix() (see below)
    // so it has been deleted by the previous call and then
    // we must ensure to use a new one if needed
    labelsComposite = nullptr;
    scatterPlotsMap.clear();
  }
}

QList<QWidget *> ScatterPlot2DView::configurationWidgets() const {
  return QList<QWidget *>() << propertiesSelectionWidget << optionsWidget;
}

void ScatterPlot2DView::graphicsViewResized(int w, int h) {
  if (initialized) {
    noPropertyMsgBox->setPos(w / 2 - noPropertyMsgBox->sceneBoundingRect().width() / 2,
                             h / 2 - noPropertyMsgBox->sceneBoundingRect().height() / 2);
  }
}

void ScatterPlot2DView::setState(const DataSet &dataSet) {
  if (!initialized) {
    propertiesSelectionWidget = new ViewGraphPropertiesSelectionWidget();
    optionsWidget = new ScatterPlot2DOptionsWidget();
    optionsWidget->setWidgetEnabled(false);
    initialized = true;
    setOverviewVisible(true);
    needQuickAccessBar = true;

    // build QMessageBox indicating the lack of selected properties
    QGraphicsRectItem *qgrItem = new QGraphicsRectItem(0, 0, 1, 1);
    qgrItem->setBrush(Qt::transparent);
    qgrItem->setPen(QPen(Qt::transparent));
    graphicsView()->scene()->addItem(qgrItem);

    QMessageBox *msgBox =
        new QMessageBox(QMessageBox::Warning, "",
                        "<b><font size=\"+1\">"
                        "Select at least two graph properties.</font></b><br/><br/>"
                        "Open the <b>Properties</b> configuration tab<br/>"
                        "to proceed.",
                        QMessageBox::Ok);
    msgBox->setModal(false);
    auto okButton = msgBox->button(QMessageBox::Ok);
    connect(okButton, SIGNAL(released()), this, SLOT(showPropertiesSelectionWidget()));
    // set a specific name before applying style sheet
    msgBox->setObjectName("needConfigurationMessageBox");
    Perspective::setStyleSheet(msgBox);
    noPropertyMsgBox = graphicsView()->scene()->addWidget(msgBox);
    noPropertyMsgBox->setParentItem(qgrItem);
  }

  if (!matrixView)
    switchFromDetailViewToMatrixView();

  Graph *lastGraph = scatterPlotGraph;
  scatterPlotGraph = graph();
  propertiesSelectionWidget->setWidgetParameters(scatterPlotGraph, propertiesTypesFilter);

  if (lastGraph == nullptr || lastGraph != scatterPlotGraph) {
    newGraphSet = true;

    if (lastGraph) {
      lastGraph->removeListener(this);
      lastGraph->getProperty("viewColor")->removeListener(this);
      lastGraph->getProperty("viewLabel")->removeListener(this);
      lastGraph->getProperty("viewSelection")->removeListener(this);
      lastGraph->getProperty("viewSize")->removeListener(this);
      lastGraph->getProperty("viewShape")->removeListener(this);
      lastGraph->getProperty("viewTexture")->removeListener(this);
    }

    initGlWidget();
    detailedScatterPlot = nullptr;

    delete edgeAsNodeGraph;

    if (scatterPlotGraph) {
      edgeAsNodeGraph = tlp::newGraph();
      ColorProperty *edgeAsNodeGraphColor =
          edgeAsNodeGraph->getProperty<ColorProperty>("viewColor");
      ColorProperty *graphColor = scatterPlotGraph->getProperty<ColorProperty>("viewColor");
      BooleanProperty *edgeAsNodeGraphSelection =
          edgeAsNodeGraph->getProperty<BooleanProperty>("viewSelection");
      BooleanProperty *graphSelection =
          scatterPlotGraph->getProperty<BooleanProperty>("viewSelection");
      StringProperty *edgeAsNodeGraphLabel =
          edgeAsNodeGraph->getProperty<StringProperty>("viewLabel");
      StringProperty *graphLabel = scatterPlotGraph->getProperty<StringProperty>("viewLabel");
      edgeToNode.clear();
      nodeToEdge.clear();
      for (auto e : scatterPlotGraph->edges()) {
        node n = edgeToNode[e] = edgeAsNodeGraph->addNode();
        nodeToEdge[n] = e;
        edgeAsNodeGraphColor->setNodeValue(n, graphColor->getEdgeValue(e));
        edgeAsNodeGraphSelection->setNodeValue(n, graphSelection->getEdgeValue(e));
        edgeAsNodeGraphLabel->setNodeValue(n, graphLabel->getEdgeValue(e));
      }
      // This is quite ugly but before listening to the graph we must
      // ensure that its viewMetaGraph property already exist to avoid
      // an event loop when building the Scatterplot2D
      scatterPlotGraph->getRoot()->getProperty<GraphProperty>("viewMetaGraph");
      scatterPlotGraph->addListener(this);
      graphColor->addListener(this);
      graphLabel->addListener(this);
      graphSelection->addListener(this);
      scatterPlotGraph->getProperty("viewSize")->addListener(this);
      scatterPlotGraph->getProperty("viewShape")->addListener(this);
      scatterPlotGraph->getProperty("viewTexture")->addListener(this);

      edgeAsNodeGraphSelection->addListener(this);
      edgeAsNodeGraph->getProperty<IntegerProperty>("viewShape")
          ->setAllNodeValue(NodeShape::Circle);
    } else
      edgeAsNodeGraph = nullptr;

    destroyOverviews();
  }

  if (!scatterPlotGraph) {
    scatterPlotsGenMap.clear();
  } else {
    if (lastGraph != nullptr && scatterPlotGraph != nullptr &&
        (lastGraph->getRoot() != scatterPlotGraph->getRoot())) {
      scatterPlotsGenMap.clear();
    }
  }

  center = true;

  dataSet.get("lastViewWindowWidth", lastViewWindowWidth);
  dataSet.get("lastViewWindowHeight", lastViewWindowHeight);

  bool showedges = false, showlabels = false, scalelabels = false;

  if (dataSet.get("display graph edges", showedges))
    optionsWidget->setDisplayGraphEdges(showedges);

  if (dataSet.get("display node labels", showlabels))
    optionsWidget->setDisplayNodeLabels(showlabels);

  if (dataSet.get("scale labels", scalelabels))
    optionsWidget->setDisplayScaleLabels(scalelabels);

  Color backgroundColor;

  if (dataSet.get("background color", backgroundColor))
    optionsWidget->setBackgroundColor(backgroundColor);

  int minSizeMap = 0;

  if (dataSet.get("min Size Mapping", minSizeMap))
    optionsWidget->setMinSizeMapping(float(minSizeMap));

  int maxSizeMap = 0;

  if (dataSet.get("max Size Mapping", maxSizeMap))
    optionsWidget->setMaxSizeMapping(float(maxSizeMap));

  optionsWidget->configurationChanged();

  DataSet selectedGraphPropertiesDataSet;

  if (dataSet.get("selected graph properties", selectedGraphPropertiesDataSet)) {
    selectedGraphProperties.clear();
    int i = 0;
    ostringstream oss;
    oss << i;

    while (selectedGraphPropertiesDataSet.exists(oss.str())) {
      string propertyName;
      selectedGraphPropertiesDataSet.get(oss.str(), propertyName);
      selectedGraphProperties.push_back(propertyName);
      oss.str("");
      oss << ++i;
    }

    propertiesSelectionWidget->setSelectedProperties(selectedGraphProperties);
    DataSet generatedScatterPlotDataSet;
    dataSet.get("generated scatter plots", generatedScatterPlotDataSet);

    for (size_t j = 0; j < selectedGraphProperties.size(); ++j) {
      for (size_t k = 0; k < selectedGraphProperties.size(); ++k) {
        if (j != k) {
          bool scatterPlotGenerated = false;
          generatedScatterPlotDataSet.get(
              selectedGraphProperties[j] + "_" + selectedGraphProperties[k], scatterPlotGenerated);
          scatterPlotsGenMap[make_pair(selectedGraphProperties[j], selectedGraphProperties[k])] =
              scatterPlotGenerated;
        }
      }
    }
  }

  unsigned nodes = NODE;
  dataSet.get("Nodes/Edges", nodes);
  dataLocation = static_cast<ElementType>(nodes);
  propertiesSelectionWidget->setDataLocation(dataLocation);
  viewConfigurationChanged();

  string detailScatterPlotX = "";
  string detailScatterPlotY = "";
  dataSet.get("detailed scatterplot x dim", detailScatterPlotX);
  dataSet.get("detailed scatterplot y dim", detailScatterPlotY);

  auto scatterPlotIdx = make_pair(detailScatterPlotX, detailScatterPlotY);

  if (!detailScatterPlotX.empty() && !detailScatterPlotY.empty()) {

    if (!scatterPlotsGenMap[scatterPlotIdx]) {
      scatterPlotsMap[scatterPlotIdx]->generateOverview();
      scatterPlotsGenMap[scatterPlotIdx] = true;
    }

    auto *scatterPlot = scatterPlotsMap[scatterPlotIdx];

    if (scatterPlot) {
      switchFromMatrixToDetailView(scatterPlot, true);
    }
  }

  registerTriggers();

  setQuickAccessBarVisible(true);
  GlMainView::setState(dataSet);
}

DataSet ScatterPlot2DView::state() const {
  DataSet dataSet = GlMainView::state();
  DataSet selectedGraphPropertiesDataSet;

  for (size_t i = 0; i < selectedGraphProperties.size(); ++i) {
    ostringstream oss;
    oss << i;
    selectedGraphPropertiesDataSet.set(oss.str(), selectedGraphProperties[i]);
  }

  dataSet.set("selected graph properties", selectedGraphPropertiesDataSet);
  DataSet generatedScatterPlotDataSet;

  for (auto it = scatterPlotsGenMap.begin(); it != scatterPlotsGenMap.end(); ++it) {
    generatedScatterPlotDataSet.set((*it).first.first + "_" + (*it).first.second, (*it).second);
  }

  dataSet.set("generated scatter plots", generatedScatterPlotDataSet);
  dataSet.set("min Size Mapping", int(optionsWidget->getMinSizeMapping().getW()));
  dataSet.set("max Size Mapping", int(optionsWidget->getMaxSizeMapping().getW()));
  dataSet.set("background color", optionsWidget->getBackgroundColor());
  dataSet.set("display graph edges", optionsWidget->displayGraphEdges());
  dataSet.set("display node labels", optionsWidget->displayNodeLabels());
  dataSet.set("scale labels", optionsWidget->displayScaleLabels());
  dataSet.set("lastViewWindowWidth", getGlMainWidget()->width());
  dataSet.set("lastViewWindowHeight", getGlMainWidget()->height());
  dataSet.set("detailed scatterplot x dim", detailedScatterPlotPropertyName.first);
  dataSet.set("detailed scatterplot y dim", detailedScatterPlotPropertyName.second);
  dataSet.set("Nodes/Edges", static_cast<unsigned>(dataLocation));

  return dataSet;
}

void ScatterPlot2DView::showPropertiesSelectionWidget() {
  WorkspacePanel *wp = static_cast<WorkspacePanel *>(graphicsView()->parentWidget());
  wp->showConfigurationTab("Properties");
}

Graph *ScatterPlot2DView::getScatterPlotGraph() {
  return scatterPlotGraph;
}

void ScatterPlot2DView::graphChanged(Graph *g) {
  if (!initialized) {
    setState(DataSet());
    return;
  }
  DataSet ds = getState(g);
  if (ds.empty()) {
    // We copy the value of "Nodes/Edges"
    // in the new state in order to keep
    // the user choice when changing graph
    DataSet oldDs = state();
    unsigned nodes = NODE;
    oldDs.get("Nodes/Edges", nodes);
    ds.set("Nodes/Edges", nodes);
  }
  setState(ds);
}

void ScatterPlot2DView::toggleInteractors(const bool activate) {
  View::toggleInteractors(activate, {InteractorName::ScatterPlot2DInteractorNavigation});
}

void ScatterPlot2DView::computeNodeSizes() {
  if (!scatterPlotSize) {
    scatterPlotSize = new SizeProperty(scatterPlotGraph);
  } else {
    scatterPlotSize->setAllNodeValue(Size(0, 0, 0));
    scatterPlotSize->setAllEdgeValue(Size(0, 0, 0));
  }

  SizeProperty *viewSize = scatterPlotGraph->getProperty<SizeProperty>("viewSize");

  Size &&eltMinSize = viewSize->getMin();
  Size &&eltMaxSize = viewSize->getMax();
  Size &&pointMinSize = optionsWidget->getMinSizeMapping();
  Size &&pointMaxSize = optionsWidget->getMaxSizeMapping();

  Size resizeFactor;
  Size &&deltaSize = eltMaxSize - eltMinSize;

  for (unsigned int i = 0; i < 3; ++i) {
    if (deltaSize[i] != 0) {
      resizeFactor[i] = (pointMaxSize[i] - pointMinSize[i]) / deltaSize[i];
    } else {
      resizeFactor[i] = 0;
    }
  }

  for (auto n : scatterPlotGraph->nodes()) {
    const Size &nodeSize = viewSize->getNodeValue(n);
    Size &&adjustedNodeSize = pointMinSize + resizeFactor * (nodeSize + Size(-1.0f, -1.0f, -1.0f));
    scatterPlotSize->setNodeValue(n, adjustedNodeSize);
  }

  GlGraphInputData *glGraphInputData = glGraphComposite->getInputData();
  glGraphInputData->setElementSize(scatterPlotSize);
}

QuickAccessBar *ScatterPlot2DView::getQuickAccessBarImpl() {
  auto _bar = new ScatterPlotQuickAccessBar(optionsWidget);
  connect(_bar, SIGNAL(settingsChanged()), this, SLOT(applySettings()));
  return _bar;
}

void ScatterPlot2DView::buildScatterPlotsMatrix() {

  dataLocation = propertiesSelectionWidget->getDataLocation();
  Color backgroundColor(optionsWidget->getBackgroundColor());
  getGlMainWidget()->getScene()->setBackgroundColor(backgroundColor);

  Color foregroundColor;
  int bgV = backgroundColor.getV();

  if (bgV < 128) {
    foregroundColor = Color(255, 255, 255);
  } else {
    foregroundColor = Color(0, 0, 0);
  }

  float gridLeft = -(OFFSET_BETWEEN_PREVIEWS / 2.0f);
  float gridBottom = gridLeft;
  float gridRight = selectedGraphProperties.size() * (OVERVIEWS_SIZE) +
                    (selectedGraphProperties.size() - 1.0f) * OFFSET_BETWEEN_PREVIEWS +
                    (OFFSET_BETWEEN_PREVIEWS / 2.0f);
  float gridTop = gridRight;
  float cellSize = OVERVIEWS_SIZE + OFFSET_BETWEEN_PREVIEWS;

  GlSimpleEntity *lastGrid = matrixComposite->findGlEntity("grid");
  matrixComposite->reset(false);
  delete lastGrid;
  labelsComposite->reset(true);

  if (selectedGraphProperties.size() >= 2) {

    GlComposite *grid = new GlComposite(true);
    GlLine *lineV0 = new GlLine();
    lineV0->addPoint(Coord(gridLeft, gridBottom, -1.0f), Color(0, 0, 0, 255));
    lineV0->addPoint(Coord(gridLeft, gridTop - cellSize, -1.0f), Color(0, 0, 0, 255));
    grid->addGlEntity(lineV0, "lineV0");
    GlLine *lineH0 = new GlLine();
    lineH0->addPoint(Coord(gridLeft, gridBottom, -1.0f), Color(0, 0, 0, 255));
    lineH0->addPoint(Coord(gridRight - cellSize, gridBottom, -1.0f), Color(0, 0, 0, 255));
    grid->addGlEntity(lineH0, "lineH0");

    for (unsigned int i = 0; i < selectedGraphProperties.size(); ++i) {
      GlLine *lineV = new GlLine();
      lineV->addPoint(Coord(gridLeft + cellSize * (i + 1), gridBottom, -1.0f), Color(0, 0, 0, 255));
      lineV->addPoint(Coord(gridLeft + cellSize * (i + 1), gridTop - cellSize * (i + 1), -1.0f),
                      Color(0, 0, 0, 255));
      GlLine *lineH = new GlLine();
      lineH->addPoint(Coord(gridLeft, gridBottom + cellSize * (i + 1), -1.0f), Color(0, 0, 0, 255));
      lineH->addPoint(Coord(gridRight - cellSize * (i + 1), gridBottom + cellSize * (i + 1), -1.0f),
                      Color(0, 0, 0, 255));
      stringstream strstr;
      strstr << i + 1;
      grid->addGlEntity(lineV, "lineV" + strstr.str());
      grid->addGlEntity(lineH, "lineH" + strstr.str());
    }

    matrixComposite->addGlEntity(grid, "grid");
    matrixComposite->addGlEntity(labelsComposite, "labels composite");

    for (size_t i = 0; i < selectedGraphProperties.size(); ++i) {

      if (i != selectedGraphProperties.size() - 1) {
        GlLabel *xLabel = new GlLabel(
            Coord(gridLeft + i * cellSize + cellSize / 2.0f, gridBottom - cellSize / 4.0f),
            Size(8.0f * (cellSize / 10.0f), cellSize / 2.0f), foregroundColor);
        xLabel->setText(selectedGraphProperties[i]);
        labelsComposite->addGlEntity(xLabel, selectedGraphProperties[i] + "x label");
      }

      if (i != 0) {
        GlLabel *yLabel =
            new GlLabel(Coord(gridLeft - cellSize / 2.0f, gridTop - i * cellSize - cellSize / 2.0f),
                        Size(8.0f * (cellSize / 10.0f), cellSize / 2.0f), foregroundColor);
        yLabel->setText(selectedGraphProperties[i]);
        labelsComposite->addGlEntity(yLabel, selectedGraphProperties[i] + "y label");
      }

      for (size_t j = i + 1; j < selectedGraphProperties.size(); ++j) {
        pair<string, string> overviewsMapKey =
            make_pair(selectedGraphProperties[i], selectedGraphProperties[j]);
        ScatterPlot2D *scatterOverview = nullptr;
        Coord overviewBlCorner(i * (OVERVIEWS_SIZE + OFFSET_BETWEEN_PREVIEWS),
                               (selectedGraphProperties.size() - j - 1.0f) *
                                   (OVERVIEWS_SIZE + OFFSET_BETWEEN_PREVIEWS));
        map<pair<string, string>, ScatterPlot2D *>::iterator it =
            scatterPlotsMap.find(overviewsMapKey);

        if (it != scatterPlotsMap.end() && it->second) {
          scatterOverview = (it->second);

          if (!scatterOverview)
            continue;

          scatterOverview->setDataLocation(dataLocation);
          scatterOverview->setBLCorner(overviewBlCorner);
          scatterOverview->setUniformBackgroundColor(backgroundColor);
          scatterOverview->setForegroundColor(foregroundColor);
        } else {
          scatterOverview = new ScatterPlot2D(
              scatterPlotGraph, edgeAsNodeGraph, nodeToEdge, selectedGraphProperties[i],
              selectedGraphProperties[j], dataLocation, overviewBlCorner, OVERVIEWS_SIZE,
              backgroundColor, foregroundColor);
          scatterPlotsMap[overviewsMapKey] = scatterOverview;

          if (scatterPlotsGenMap.find(overviewsMapKey) == scatterPlotsGenMap.end()) {
            scatterPlotsGenMap[overviewsMapKey] = false;
          }
        }

        scatterOverview->setDisplayGraphEdges(optionsWidget->displayGraphEdges());
        scatterOverview->setDisplayNodeLabels(optionsWidget->displayNodeLabels());
        scatterOverview->setLabelsScaled(optionsWidget->displayScaleLabels());

        if (!optionsWidget->uniformBackground()) {
          scatterOverview->mapBackgroundColorToCorrelCoeff(true, optionsWidget->getMinusOneColor(),
                                                           optionsWidget->getZeroColor(),
                                                           optionsWidget->getOneColor());
        }

        matrixComposite->addGlEntity(scatterOverview,
                                     selectedGraphProperties[i] + "_" + selectedGraphProperties[j]);

        // add some feedback
        /*if ((i + 1) * (j + 1) % 10 == 0)
          QApplication::processEvents();*/
      }
    }
  }

  if (!detailedScatterPlotPropertyName.first.empty() &&
      !detailedScatterPlotPropertyName.second.empty()) {
    detailedScatterPlot = scatterPlotsMap[detailedScatterPlotPropertyName];
  }

  if (center)
    centerView();
}

void ScatterPlot2DView::propertiesSelected(bool flag) {
  noPropertyMsgBox->setVisible(!flag);
  if (quickAccessBarVisible())
    _quickAccessBar->setEnabled(flag);
  setOverviewVisible(flag);
}

void ScatterPlot2DView::viewConfigurationChanged() {
  getGlMainWidget()->getScene()->setBackgroundColor(optionsWidget->getBackgroundColor());
  bool dataLocationChanged = propertiesSelectionWidget->getDataLocation() != dataLocation;

  if (dataLocationChanged) {
    detailedScatterPlot = nullptr;
    buildScatterPlotsMatrix();
  }

  if (detailedScatterPlot != nullptr) {

    detailedScatterPlot->setXAxisScaleDefined(optionsWidget->useCustomXAxisScale());
    detailedScatterPlot->setXAxisScale(optionsWidget->getXAxisScale());
    detailedScatterPlot->setYAxisScaleDefined(optionsWidget->useCustomYAxisScale());
    detailedScatterPlot->setYAxisScale(optionsWidget->getYAxisScale());
  }

  draw();
  drawOverview(true);
}

void ScatterPlot2DView::draw() {
  GlMainWidget *gl = getGlMainWidget();

  destroyOverviewsIfNeeded();

  if (selectedGraphProperties.size() !=
      propertiesSelectionWidget->getSelectedGraphProperties().size()) {
    center = true;
  }

  selectedGraphProperties = propertiesSelectionWidget->getSelectedGraphProperties();

  if (selectedGraphProperties.size() < 2) {
    destroyOverviews();
    propertiesSelected(false);
    matrixUpdateNeeded = false;
    switchFromDetailViewToMatrixView();
    gl->centerScene();

    return;
  }

  propertiesSelected(true);
  computeNodeSizes();
  buildScatterPlotsMatrix();

  if (!matrixView && detailedScatterPlot != nullptr) {
    gl->makeCurrent();
    detailedScatterPlot->generateOverview();
    axisComposite->reset(false);
    axisComposite->addGlEntity(detailedScatterPlot->getXAxis(), "x axis");
    axisComposite->addGlEntity(detailedScatterPlot->getYAxis(), "y axis");
    matrixUpdateNeeded = true;

    if (newGraphSet) {
      switchFromMatrixToDetailView(detailedScatterPlot, center);
      newGraphSet = false;
    }
  } else if (matrixView) {
    gl->makeCurrent();
    generateScatterPlots();
  } else if (!matrixView && detailedScatterPlot == nullptr) {
    switchFromDetailViewToMatrixView();
    center = true;
  }

  if (center) {
    centerView();
  } else {
    gl->draw();
  }
}

void ScatterPlot2DView::centerView(bool) {
  GlMainWidget *gl = getGlMainWidget();

  if (!gl->isVisible()) {
    if (lastViewWindowWidth != 0 && lastViewWindowHeight != 0) {
      gl->getScene()->adjustSceneToSize(lastViewWindowWidth, lastViewWindowHeight);
    } else {
      gl->centerScene();
    }
  } else {
    gl->getScene()->adjustSceneToSize(gl->width(), gl->height());
  }

  // we apply a zoom factor to preserve a 50 px margin width
  // to ensure the scene will not be drawn under the configuration tabs title
  float glWidth = graphicsView()->width();
  gl->getScene()->zoomFactor((glWidth - 50) / glWidth);
  gl->draw();
  center = false;
}

void ScatterPlot2DView::applySettings() {
  if (propertiesSelectionWidget->configurationChanged() || optionsWidget->configurationChanged()) {
    viewConfigurationChanged();
    if (quickAccessBarVisible())
      _quickAccessBar->reset();
  }
}

void ScatterPlot2DView::destroyOverviewsIfNeeded() {

  vector<string> propertiesToRemove;

  for (size_t i = 0; i < selectedGraphProperties.size(); ++i) {

    if (!scatterPlotGraph || !scatterPlotGraph->existProperty(selectedGraphProperties[i])) {
      propertiesToRemove.push_back(selectedGraphProperties[i]);

      if (detailedScatterPlotPropertyName.first == selectedGraphProperties[i] ||
          detailedScatterPlotPropertyName.second == selectedGraphProperties[i]) {
        detailedScatterPlotPropertyName = make_pair("", "");
      }

      map<pair<string, string>, ScatterPlot2D *>::iterator overviewToDestroyIt;
      overviewToDestroyIt = find_if(scatterPlotsMap.begin(), scatterPlotsMap.end(),
                                    map_pair_string_key_contains(selectedGraphProperties[i]));

      while (overviewToDestroyIt != scatterPlotsMap.end()) {
        if (overviewToDestroyIt->second == detailedScatterPlot) {
          detailedScatterPlot = nullptr;

          if (!matrixView) {
            GlGraphInputData *glGraphInputData = glGraphComposite->getInputData();
            glGraphInputData->setElementLayout(
                scatterPlotGraph->getProperty<LayoutProperty>("viewLayout"));
          }
        }

        delete overviewToDestroyIt->second;
        scatterPlotsGenMap.erase(overviewToDestroyIt->first);
        scatterPlotsMap.erase(overviewToDestroyIt);
        overviewToDestroyIt = find_if(scatterPlotsMap.begin(), scatterPlotsMap.end(),
                                      map_pair_string_key_contains(selectedGraphProperties[i]));
      }
    }
  }

  for (size_t i = 0; i < propertiesToRemove.size(); ++i) {
    selectedGraphProperties.erase(remove(selectedGraphProperties.begin(),
                                         selectedGraphProperties.end(), propertiesToRemove[i]),
                                  selectedGraphProperties.end());
  }

  if (!propertiesToRemove.empty()) {
    propertiesSelectionWidget->setSelectedProperties(selectedGraphProperties);
  }
}

void ScatterPlot2DView::destroyOverviews() {
  for (auto &it : scatterPlotsMap) {
    matrixComposite->deleteGlEntity(it.second);
    delete it.second;
  }

  scatterPlotsMap.clear();
  detailedScatterPlot = nullptr;
  GlSimpleEntity *grid = matrixComposite->findGlEntity("grid");
  matrixComposite->deleteGlEntity(grid);
  delete grid;
  labelsComposite->reset(true);
  mainLayer->addGlEntity(glGraphComposite, "graph");
}

void ScatterPlot2DView::generateScatterPlots() {

  if (selectedGraphProperties.empty())
    return;

  GlLabel *coeffLabel = nullptr;

  if (matrixView) {
    mainLayer->deleteGlEntity(matrixComposite);
  } else {
    mainLayer->deleteGlEntity(axisComposite);
    mainLayer->addGlEntity(glGraphComposite, "graph");
    coeffLabel = dynamic_cast<GlLabel *>(mainLayer->findGlEntity("coeffLabel"));
    mainLayer->deleteGlEntity("coeffLabel");
  }

  unsigned int nbOverviews =
      (selectedGraphProperties.size() - 1) * selectedGraphProperties.size() / 2;
  unsigned currentStep = 0;

  double sceneRadiusBak = getGlMainWidget()->getScene()->getGraphCamera().getSceneRadius();
  double zoomFactorBak = getGlMainWidget()->getScene()->getGraphCamera().getZoomFactor();
  Coord eyesBak = getGlMainWidget()->getScene()->getGraphCamera().getEyes();
  Coord centerBak = getGlMainWidget()->getScene()->getGraphCamera().getCenter();
  Coord upBak = getGlMainWidget()->getScene()->getGraphCamera().getUp();

  {
    QProgressDialog progress(Perspective::instance()->mainWindow());
    progress.setWindowTitle("Computing scatter plot overview for: ");
    progress.setCancelButton(nullptr);
    progress.setRange(0, nbOverviews);
    progress.setMinimumWidth(400);
    progress.setWindowModality(Qt::WindowModal);
    progress.setValue(0);

    for (size_t i = 0; i < selectedGraphProperties.size() - 1; ++i) {
      for (size_t j = 0; j < selectedGraphProperties.size(); ++j) {
        ScatterPlot2D *overview =
            scatterPlotsMap[make_pair(selectedGraphProperties[i], selectedGraphProperties[j])];

        if (overview) {
          progress.setLabelText(QString("%1 - %2")
                                    .arg(selectedGraphProperties[i].c_str())
                                    .arg(selectedGraphProperties[j].c_str()));

          overview->generateOverview();
          scatterPlotsGenMap[make_pair(selectedGraphProperties[i], selectedGraphProperties[j])] =
              true;

          progress.setValue(++currentStep);
        }
      }
    }
  }

  if (matrixView) {
    mainLayer->addGlEntity(matrixComposite, "matrix composite");
  } else {
    mainLayer->addGlEntity(axisComposite, "axis composite");

    if (coeffLabel != nullptr) {
      mainLayer->addGlEntity(coeffLabel, "coeffLabel");
    }

    mainLayer->addGlEntity(detailedScatterPlot->getGlGraphComposite(), "graph");
  }

  getGlMainWidget()->getScene()->getGraphCamera().setSceneRadius(sceneRadiusBak);
  getGlMainWidget()->getScene()->getGraphCamera().setZoomFactor(zoomFactorBak);
  getGlMainWidget()->getScene()->getGraphCamera().setEyes(eyesBak);
  getGlMainWidget()->getScene()->getGraphCamera().setCenter(centerBak);
  getGlMainWidget()->getScene()->getGraphCamera().setUp(upBak);

  getGlMainWidget()->draw();
}

void ScatterPlot2DView::generateScatterPlot(ScatterPlot2D *scatterPlot) {
  scatterPlot->generateOverview();
  scatterPlotsGenMap[make_pair(scatterPlot->getXDim(), scatterPlot->getYDim())] = true;
}

void ScatterPlot2DView::switchFromMatrixToDetailView(ScatterPlot2D *scatterPlot, bool recenter) {

  sceneRadiusBak = getGlMainWidget()->getScene()->getGraphCamera().getSceneRadius();
  zoomFactorBak = getGlMainWidget()->getScene()->getGraphCamera().getZoomFactor();
  eyesBak = getGlMainWidget()->getScene()->getGraphCamera().getEyes();
  centerBak = getGlMainWidget()->getScene()->getGraphCamera().getCenter();
  upBak = getGlMainWidget()->getScene()->getGraphCamera().getUp();

  mainLayer->deleteGlEntity(matrixComposite);
  GlQuantitativeAxis *xAxis = scatterPlot->getXAxis();
  GlQuantitativeAxis *yAxis = scatterPlot->getYAxis();
  axisComposite->addGlEntity(xAxis, "x axis");
  axisComposite->addGlEntity(yAxis, "y axis");
  mainLayer->addGlEntity(axisComposite, "axis composite");
  GlLabel *coeffLabel = new GlLabel(
      Coord(xAxis->getAxisBaseCoord().getX() + (1.0f / 2.0f) * xAxis->getAxisLength(),
            yAxis->getAxisBaseCoord().getY() - 260),
      Size(xAxis->getAxisLength() / 2.0f, yAxis->getLabelHeight()), xAxis->getAxisColor());
  ostringstream oss;
  oss << "correlation coefficient = " << scatterPlot->getCorrelationCoefficient();
  coeffLabel->setText(oss.str());
  mainLayer->addGlEntity(coeffLabel, "coeffLabel");
  mainLayer->addGlEntity(scatterPlot->getGlGraphComposite(), "graph");
  toggleInteractors(true);
  matrixView = false;
  detailedScatterPlot = scatterPlot;
  detailedScatterPlotPropertyName = make_pair(scatterPlot->getXDim(), scatterPlot->getYDim());
  propertiesSelectionWidget->setWidgetEnabled(false);
  optionsWidget->setWidgetEnabled(true);
  optionsWidget->useCustomXAxisScale(detailedScatterPlot->getXAxisScaleDefined());
  optionsWidget->setXAxisScale(detailedScatterPlot->getXAxisScale());
  optionsWidget->useCustomYAxisScale(detailedScatterPlot->getYAxisScaleDefined());
  optionsWidget->setYAxisScale(detailedScatterPlot->getYAxisScale());
  optionsWidget->setInitXAxisScale(detailedScatterPlot->getInitXAxisScale());
  optionsWidget->setInitYAxisScale(detailedScatterPlot->getInitYAxisScale());
  optionsWidget->configurationChanged();

  if (recenter)
    centerView();
}

void ScatterPlot2DView::switchFromDetailViewToMatrixView() {

  axisComposite->reset(false);
  mainLayer->deleteGlEntity("coeffLabel");

  if (matrixUpdateNeeded) {
    generateScatterPlots();
    matrixUpdateNeeded = false;
  }

  mainLayer->addGlEntity(glGraphComposite, "graph");
  mainLayer->addGlEntity(matrixComposite, "matrix composite");
  GlScene *scene = getGlMainWidget()->getScene();
  Camera &cam = scene->getGraphCamera();
  cam.setSceneRadius(sceneRadiusBak);
  cam.setZoomFactor(zoomFactorBak);
  cam.setEyes(eyesBak);
  cam.setCenter(centerBak);
  cam.setUp(upBak);
  scene->setBackgroundColor(optionsWidget->getBackgroundColor());
  matrixView = true;
  detailedScatterPlot = nullptr;
  detailedScatterPlotPropertyName = make_pair("", "");
  propertiesSelectionWidget->setWidgetEnabled(true);
  optionsWidget->setWidgetEnabled(false);
  optionsWidget->resetAxisScale();
  toggleInteractors(false);
  // select the navigator interactor
  // allowing to choose one of the detailed views
  if (!interactors().empty())
    setCurrentInteractor(interactors()[0]);
  getGlMainWidget()->draw();
}

void ScatterPlot2DView::refresh() {
  getGlMainWidget()->redraw();
}

void ScatterPlot2DView::init() {
  emit drawNeeded();
}

BoundingBox ScatterPlot2DView::getMatrixBoundingBox() {
  GlBoundingBoxSceneVisitor glBBSV(nullptr);
  matrixComposite->acceptVisitor(&glBBSV);
  return glBBSV.getBoundingBox();
}

std::vector<ScatterPlot2D *> ScatterPlot2DView::getSelectedScatterPlots() const {
  vector<ScatterPlot2D *> ret;
  map<pair<string, string>, ScatterPlot2D *>::const_iterator it;

  for (it = scatterPlotsMap.begin(); it != scatterPlotsMap.end(); ++it) {
    // a scatter plot is selected if non null
    // and if the property on the x axis is before the property on the y axis
    // in the selectedGraphProperties vector
    if (!it->second)
      continue;

    // properties on x and y axis
    const string &xProp = (it->first).first;
    const string &yProp = (it->first).second;
    // position in the selectedGraphProperties of the property on the x axis
    int xPos = -1;
    bool valid = false;

    for (unsigned int i = 0; i < selectedGraphProperties.size(); ++i) {
      const string &prop = selectedGraphProperties[i];

      if (prop == xProp) {
        xPos = i;
        continue;
      }

      if (prop == yProp) {
        if (xPos != -1)
          valid = true;

        break;
      }
    }

    if (valid)
      ret.push_back(it->second);
  }

  return ret;
}

void ScatterPlot2DView::interactorsInstalled(const QList<tlp::Interactor *> &) {
  toggleInteractors(detailedScatterPlot != nullptr);
}

void ScatterPlot2DView::registerTriggers() {
  for (auto obs : triggers()) {
    removeRedrawTrigger(obs);
  }

  if (graph()) {
    addRedrawTrigger(graph());
    for (auto prop : getScatterPlotGraph()->getObjectProperties()) {
      addRedrawTrigger(prop);
    }
  }
}

void ScatterPlot2DView::treatEvent(const Event &message) {
  const GraphEvent *graphEvent = dynamic_cast<const GraphEvent *>(&message);

  if (graphEvent) {
    if (graphEvent->getType() == GraphEvent::TLP_ADD_EDGE)
      addEdge(graphEvent->getGraph(), graphEvent->getEdge());

    if (graphEvent->getType() == GraphEvent::TLP_DEL_NODE)
      delNode(graphEvent->getGraph(), graphEvent->getNode());

    if (graphEvent->getType() == GraphEvent::TLP_DEL_EDGE)
      delEdge(graphEvent->getGraph(), graphEvent->getEdge());
  }

  const PropertyEvent *propertyEvent = dynamic_cast<const PropertyEvent *>(&message);

  if (propertyEvent) {
    if (propertyEvent->getType() == PropertyEvent::TLP_AFTER_SET_NODE_VALUE)
      afterSetNodeValue(propertyEvent->getProperty(), propertyEvent->getNode());

    if (propertyEvent->getType() == PropertyEvent::TLP_AFTER_SET_EDGE_VALUE)
      afterSetEdgeValue(propertyEvent->getProperty(), propertyEvent->getEdge());

    if (propertyEvent->getType() == PropertyEvent::TLP_AFTER_SET_ALL_NODE_VALUE)
      afterSetAllNodeValue(propertyEvent->getProperty());

    if (propertyEvent->getType() == PropertyEvent::TLP_AFTER_SET_ALL_EDGE_VALUE)
      afterSetAllEdgeValue(propertyEvent->getProperty());
  }
}

void ScatterPlot2DView::afterSetNodeValue(PropertyInterface *p, const node n) {
  if (p->getGraph() == edgeAsNodeGraph && p->getName() == "viewSelection") {
    BooleanProperty *edgeAsNodeGraphSelection = static_cast<BooleanProperty *>(p);
    BooleanProperty *viewSelection =
        scatterPlotGraph->getProperty<BooleanProperty>("viewSelection");
    viewSelection->removeListener(this);
    viewSelection->setEdgeValue(nodeToEdge[n], edgeAsNodeGraphSelection->getNodeValue(n));
    viewSelection->addListener(this);
    return;
  }
}

void ScatterPlot2DView::afterSetEdgeValue(PropertyInterface *p, const edge e) {
  if (edgeToNode.find(e) == edgeToNode.end())
    return;

  if (p->getName() == "viewColor") {
    ColorProperty *edgeAsNodeGraphColors = edgeAsNodeGraph->getProperty<ColorProperty>("viewColor");
    ColorProperty *viewColor = static_cast<ColorProperty *>(p);
    edgeAsNodeGraphColors->setNodeValue(edgeToNode[e], viewColor->getEdgeValue(e));
  } else if (p->getName() == "viewLabel") {
    StringProperty *edgeAsNodeGraphLabels =
        edgeAsNodeGraph->getProperty<StringProperty>("viewLabel");
    StringProperty *viewLabel = static_cast<StringProperty *>(p);
    edgeAsNodeGraphLabels->setNodeValue(edgeToNode[e], viewLabel->getEdgeValue(e));
  } else if (p->getName() == "viewSelection") {
    BooleanProperty *edgeAsNodeGraphSelection =
        edgeAsNodeGraph->getProperty<BooleanProperty>("viewSelection");
    BooleanProperty *viewSelection = static_cast<BooleanProperty *>(p);
    edgeAsNodeGraphSelection->removeListener(this);

    if (edgeAsNodeGraphSelection->getNodeValue(edgeToNode[e]) != viewSelection->getEdgeValue(e))
      edgeAsNodeGraphSelection->setNodeValue(edgeToNode[e], viewSelection->getEdgeValue(e));

    edgeAsNodeGraphSelection->addListener(this);
  }
}

void ScatterPlot2DView::afterSetAllNodeValue(PropertyInterface *p) {
  if (p->getName() == "viewSelection") {
    if (p->getGraph() == edgeAsNodeGraph) {
      BooleanProperty *edgeAsNodeGraphSelection = static_cast<BooleanProperty *>(p);
      BooleanProperty *viewSelection =
          scatterPlotGraph->getProperty<BooleanProperty>("viewSelection");
      viewSelection->setAllEdgeValue(
          edgeAsNodeGraphSelection->getNodeValue(edgeAsNodeGraph->getOneNode()));
    }
  }
}

void ScatterPlot2DView::afterSetAllEdgeValue(PropertyInterface *p) {

  if (p->getName() == "viewColor") {
    ColorProperty *edgeAsNodeGraphColors = edgeAsNodeGraph->getProperty<ColorProperty>("viewColor");
    ColorProperty *viewColor = static_cast<ColorProperty *>(p);
    edgeAsNodeGraphColors->setAllNodeValue(viewColor->getEdgeDefaultValue());
  } else if (p->getName() == "viewLabel") {
    StringProperty *edgeAsNodeGraphLabels =
        edgeAsNodeGraph->getProperty<StringProperty>("viewLabel");
    StringProperty *viewLabel = static_cast<StringProperty *>(p);
    edgeAsNodeGraphLabels->setAllNodeValue(viewLabel->getEdgeDefaultValue());
  } else if (p->getName() == "viewSelection") {
    BooleanProperty *edgeAsNodeGraphSelection =
        edgeAsNodeGraph->getProperty<BooleanProperty>("viewSelection");
    BooleanProperty *viewSelection = static_cast<BooleanProperty *>(p);
    for (auto e : scatterPlotGraph->edges()) {
      if (edgeAsNodeGraphSelection->getNodeValue(edgeToNode[e]) != viewSelection->getEdgeValue(e)) {
        edgeAsNodeGraphSelection->setNodeValue(edgeToNode[e], viewSelection->getEdgeValue(e));
      }
    }
  }
}

void ScatterPlot2DView::addEdge(Graph *, const edge e) {
  edgeToNode[e] = edgeAsNodeGraph->addNode();
}

void ScatterPlot2DView::delNode(Graph *, const node) {}

void ScatterPlot2DView::delEdge(Graph *, const edge e) {
  edgeAsNodeGraph->delNode(edgeToNode[e]);
  edgeToNode.erase(e);
}

unsigned int ScatterPlot2DView::getMappedId(unsigned int id) {
  if (dataLocation == EDGE)
    return nodeToEdge[node(id)].id;

  return id;
}
} // namespace tlp
