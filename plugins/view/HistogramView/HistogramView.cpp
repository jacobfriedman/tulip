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

#include <tulip/GlQuantitativeAxis.h>
#include <tulip/Interactor.h>
#include <tulip/GlMainWidget.h>
#include <tulip/GlLabel.h>
#include <tulip/GlRect.h>

#include <tulip/Perspective.h>
#include <tulip/TlpQtTools.h>
#include <tulip/QuickAccessBar.h>
#include <tulip/TulipViewSettings.h>
#include <tulip/ViewGraphPropertiesSelectionWidget.h>
#include <tulip/WorkspacePanel.h>

#include <QHelpEvent>
#include <QApplication>
#include <QToolTip>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QAbstractButton>
#include <QMessageBox>

#include "HistogramView.h"
#include "HistogramInteractors.h"
#include "HistoOptionsWidget.h"

using namespace std;

const unsigned int OVERVIEW_SIZE = 512;

const string propertiesTypes[] = {"double", "int"};
const unsigned int nbPropertiesTypes = sizeof(propertiesTypes) / sizeof(string);
const vector<string> propertiesTypesFilter(propertiesTypes, propertiesTypes + nbPropertiesTypes);

template <typename T>
std::string getStringFromNumber(T number, unsigned int precision = 5) {
  std::ostringstream oss;
  oss.precision(precision);
  oss << number;
  return oss.str();
}

namespace tlp {

PLUGIN(HistogramView)

HistogramView::HistogramView(const PluginContext *)
    : GlMainView(true), propertiesSelectionWidget(nullptr), histoOptionsWidget(nullptr),
      xAxisDetail(nullptr), yAxisDetail(nullptr), _histoGraph(nullptr), emptyGraph(nullptr),
      emptyGlGraphComposite(nullptr), histogramsComposite(nullptr), labelsComposite(nullptr),
      axisComposite(nullptr), smallMultiplesView(true), mainLayer(nullptr),
      detailedHistogram(nullptr), sceneRadiusBak(0), zoomFactorBak(0), emptyRect(nullptr),
      emptyRect2(nullptr), isConstruct(false), lastNbHistograms(0), dataLocation(NODE),
      needUpdateHistogram(false), edgeAsNodeGraph(nullptr) {}

HistogramView::~HistogramView() {
  if (isConstruct) {
    if (currentInteractor() != nullptr)
      currentInteractor()->uninstall();

    delete propertiesSelectionWidget;
    delete histoOptionsWidget;
    delete emptyGlGraphComposite;
    delete histogramsComposite;
    delete labelsComposite;
    delete emptyGraph;
    delete axisComposite;
    delete edgeAsNodeGraph;
  }
}

QList<QWidget *> HistogramView::configurationWidgets() const {
  return QList<QWidget *>() << propertiesSelectionWidget << histoOptionsWidget;
}

void HistogramView::initGlWidget() {
  mainLayer = getGlMainWidget()->getScene()->getLayer("Main");

  if (mainLayer == nullptr) {
    mainLayer = new GlLayer("Main");
    getGlMainWidget()->getScene()->addExistingLayer(mainLayer);
  }

  cleanupGlScene();

  if (emptyGlGraphComposite == nullptr) {
    emptyGraph = newGraph();
    emptyGlGraphComposite = new GlGraphComposite(emptyGraph);
  }

  mainLayer->addGlEntity(emptyGlGraphComposite, "graph");

  if (histogramsComposite == nullptr) {
    histogramsComposite = new GlComposite();
    mainLayer->addGlEntity(histogramsComposite, "overviews composite");
  }

  if (labelsComposite == nullptr) {
    labelsComposite = new GlComposite();
    mainLayer->addGlEntity(labelsComposite, "labels composite");
  }

  if (axisComposite == nullptr) {
    axisComposite = new GlComposite();
  }
}

void HistogramView::cleanupGlScene() {
  if (!smallMultiplesView && detailedHistogram != nullptr) {
    mainLayer->deleteGlEntity(detailedHistogram->getBinsComposite());
  }

  if (axisComposite != nullptr) {
    axisComposite->reset(false);
  }

  if (labelsComposite != nullptr) {
    labelsComposite->reset(true);
  }

  if (histogramsComposite != nullptr) {
    histogramsComposite->reset(true);
    histogramsMap.clear();
  }
}

QuickAccessBar *HistogramView::getQuickAccessBarImpl() {
  auto _bar = new QuickAccessBarImpl(
      nullptr, QuickAccessBarImpl::QuickAccessButtons(
                   QuickAccessBarImpl::SCREENSHOT | QuickAccessBarImpl::BACKGROUNDCOLOR |
                   QuickAccessBarImpl::SHOWLABELS | QuickAccessBarImpl::LABELSSCALED |
                   QuickAccessBarImpl::SHOWEDGES | QuickAccessBarImpl::NODECOLOR |
                   QuickAccessBarImpl::EDGECOLOR | QuickAccessBarImpl::NODEBORDERCOLOR |
                   QuickAccessBarImpl::LABELCOLOR));
  return _bar;
}

void HistogramView::graphicsViewResized(int w, int h) {
  if (isConstruct) {
    noPropertyMsgBox->setPos(w / 2 - noPropertyMsgBox->sceneBoundingRect().width() / 2,
                             h / 2 - noPropertyMsgBox->sceneBoundingRect().height() / 2);
  }
}

void HistogramView::setState(const DataSet &dataSet) {
  GlMainWidget *gl = getGlMainWidget();

  if (!isConstruct) {
    isConstruct = true;
    gl->installEventFilter(this);
    propertiesSelectionWidget = new ViewGraphPropertiesSelectionWidget();
    histoOptionsWidget = new HistoOptionsWidget();
    propertiesSelectionWidget->setWidgetEnabled(true);
    histoOptionsWidget->setWidgetEnabled(false);

    // build QMessageBox indicating the lack of selected properties
    QGraphicsRectItem *qgrItem = new QGraphicsRectItem(0, 0, 1, 1);
    qgrItem->setBrush(Qt::transparent);
    qgrItem->setPen(QPen(Qt::transparent));
    graphicsView()->scene()->addItem(qgrItem);

    QMessageBox *msgBox = new QMessageBox(QMessageBox::Warning, "",
                                          "<b><font size=\"+1\">"
                                          "No graph properties selected.</font></b><br/><br/>"
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

  Graph *lastGraph = _histoGraph;
  _histoGraph = graph();
  destroyHistogramsIfNeeded();

  if (lastGraph == nullptr || lastGraph != _histoGraph) {
    if (lastGraph) {
      lastGraph->removeListener(this);
      lastGraph->getProperty("viewColor")->removeListener(this);
      lastGraph->getProperty("viewLabel")->removeListener(this);
      lastGraph->getProperty("viewSize")->removeListener(this);
      lastGraph->getProperty("viewShape")->removeListener(this);
      lastGraph->getProperty("viewSelection")->removeListener(this);
      lastGraph->getProperty("viewTexture")->removeListener(this);
    }

    initGlWidget();
    detailedHistogram = nullptr;

    delete edgeAsNodeGraph;

    if (_histoGraph) {
      edgeAsNodeGraph = tlp::newGraph();
      edgeToNode.clear();
      nodeToEdge.clear();
      for (auto e : _histoGraph->edges()) {
        nodeToEdge[edgeToNode[e] = edgeAsNodeGraph->addNode()] = e;
        edgeAsNodeGraph->getProperty<ColorProperty>("viewColor")
            ->setNodeValue(edgeToNode[e],
                           _histoGraph->getProperty<ColorProperty>("viewColor")->getEdgeValue(e));
        edgeAsNodeGraph->getProperty<BooleanProperty>("viewSelection")
            ->setNodeValue(
                edgeToNode[e],
                _histoGraph->getProperty<BooleanProperty>("viewSelection")->getEdgeValue(e));
        edgeAsNodeGraph->getProperty<StringProperty>("viewLabel")
            ->setNodeValue(edgeToNode[e],
                           _histoGraph->getProperty<StringProperty>("viewLabel")->getEdgeValue(e));
      }
      edgeAsNodeGraph->getProperty<IntegerProperty>("viewShape")
          ->setAllNodeValue(NodeShape::Circle);
      edgeAsNodeGraph->getProperty<BooleanProperty>("viewSelection")->addListener(this);
      _histoGraph->addListener(this);
      _histoGraph->getProperty("viewColor")->addListener(this);
      _histoGraph->getProperty("viewLabel")->addListener(this);
      _histoGraph->getProperty("viewSize")->addListener(this);
      _histoGraph->getProperty("viewShape")->addListener(this);
      _histoGraph->getProperty("viewSelection")->addListener(this);
      _histoGraph->getProperty("viewTexture")->addListener(this);
    }
  }

  propertiesSelectionWidget->setWidgetParameters(graph(), propertiesTypesFilter);

  dataSet.get("histo detailed name", detailedHistogramPropertyName);
  Color backgroundColor;

  if (dataSet.get("backgroundColor", backgroundColor)) {
    histoOptionsWidget->setBackgroundColor(backgroundColor);
  }

  unordered_map<string, DataSet> histogramParametersMap;
  DataSet histogramParameters;
  int i = 0;
  stringstream ss;
  ss << i;

  while (dataSet.get("histo" + ss.str(), histogramParameters)) {
    string propertyName;
    histogramParameters.get("property name", propertyName);
    selectedProperties.push_back(propertyName);
    histogramParametersMap[propertyName] = histogramParameters;

    ss.str("");
    ss << ++i;
  }

  propertiesSelectionWidget->setSelectedProperties(selectedProperties);

  if (!selectedProperties.empty()) {
    buildHistograms();

    for (size_t j = 0; j < selectedProperties.size(); ++j) {
      unsigned int nbHistogramBins = 0;

      Histogram *histo = histogramsMap[selectedProperties[j]];

      if (histogramParametersMap[selectedProperties[j]].get("nb histogram bins", nbHistogramBins)) {
        histo->setLayoutUpdateNeeded();
        histo->setNbHistogramBins(nbHistogramBins);
      }

      unsigned int nbXGraduations = 0;

      if (histogramParametersMap[selectedProperties[j]].get("x axis nb graduations",
                                                            nbXGraduations)) {
        histo->setLayoutUpdateNeeded();
        histo->setNbXGraduations(nbXGraduations);
      }

      unsigned int yAxisIncrementStep = 0;

      if (histogramParametersMap[selectedProperties[j]].get("y axis increment step",
                                                            yAxisIncrementStep)) {
        histo->setLayoutUpdateNeeded();
        histo->setYAxisIncrementStep(yAxisIncrementStep);
      }

      bool cumulativeFrequenciesHisto = false;

      if (histogramParametersMap[selectedProperties[j]].get("cumulative frequencies histogram",
                                                            cumulativeFrequenciesHisto)) {
        histo->setLayoutUpdateNeeded();
        histo->setCumulativeHistogram(cumulativeFrequenciesHisto);
        histo->setLastCumulativeHistogram(cumulativeFrequenciesHisto);
      }

      bool uniformQuantification = false;

      if (histogramParametersMap[selectedProperties[j]].get("uniform quantification",
                                                            uniformQuantification)) {
        histo->setLayoutUpdateNeeded();
        histo->setUniformQuantification(uniformQuantification);
      }

      bool xAxisLogScale = false;

      if (histogramParametersMap[selectedProperties[j]].get("x axis logscale", xAxisLogScale)) {
        histo->setLayoutUpdateNeeded();
        histo->setXAxisLogScale(xAxisLogScale);
      }

      bool yAxisLogScale = false;

      if (histogramParametersMap[selectedProperties[j]].get("y axis logscale", yAxisLogScale)) {
        histo->setLayoutUpdateNeeded();
        histo->setYAxisLogScale(yAxisLogScale);
      }

      bool useCustomAxisScale = false;

      if (histogramParametersMap[selectedProperties[j]].get("x axis custom scale",
                                                            useCustomAxisScale)) {
        histo->setLayoutUpdateNeeded();
        histo->setXAxisScaleDefined(useCustomAxisScale);

        if (useCustomAxisScale) {
          std::pair<double, double> axisScale(0, 0);
          histogramParametersMap[selectedProperties[j]].get("x axis scale min", axisScale.first);
          histogramParametersMap[selectedProperties[j]].get("x axis scale max", axisScale.second);
          histo->setXAxisScale(axisScale);
        }
      }

      if (histogramParametersMap[selectedProperties[j]].get("y axis custom scale",
                                                            useCustomAxisScale)) {
        histo->setLayoutUpdateNeeded();
        histo->setYAxisScaleDefined(useCustomAxisScale);

        if (useCustomAxisScale) {
          std::pair<double, double> axisScale(0, 0);
          histogramParametersMap[selectedProperties[j]].get("y axis scale min", axisScale.first);
          histogramParametersMap[selectedProperties[j]].get("y axis scale max", axisScale.second);
          histo->setXAxisScale(axisScale);
        }
      }
    }
  }

  unsigned nodes = NODE;
  dataSet.get("Nodes/Edges", nodes);
  dataLocation = static_cast<ElementType>(nodes);
  propertiesSelectionWidget->setDataLocation(dataLocation);
  viewConfigurationChanged();

  registerTriggers();

  if (!detailedHistogramPropertyName.empty()) {
    Histogram *histo = histogramsMap[detailedHistogramPropertyName];
    histo->update();
    switchFromSmallMultiplesToDetailedView(histo);
  }

  setQuickAccessBarVisible(true);
  GlMainView::setState(dataSet);
}

DataSet HistogramView::state() const {
  DataSet dataSet = GlMainView::state();
  dataSet.set("Nodes/Edges", static_cast<unsigned>(dataLocation));

  unsigned int i = 0;
  for (auto &prop : selectedProperties) {
    std::stringstream ss;
    ss << i++;
    DataSet histogramParameters;
    auto histo = histogramsMap.find(prop)->second;
    histogramParameters.set("property name", prop);
    histogramParameters.set("nb histogram bins", histo->getNbHistogramBins());
    histogramParameters.set("x axis nb graduations", histo->getNbXGraduations());
    histogramParameters.set("y axis increment step", histo->getYAxisIncrementStep());
    histogramParameters.set("cumulative frequencies histogram",
                            histo->cumulativeFrequenciesHistogram());
    histogramParameters.set("uniform quantification", histo->uniformQuantificationHistogram());
    histogramParameters.set("x axis logscale", histo->xAxisLogScaleSet());
    histogramParameters.set("y axis logscale", histo->yAxisLogScaleSet());
    bool customScale = histo->getXAxisScaleDefined();
    histogramParameters.set("x axis custom scale", customScale);

    if (customScale) {
      const std::pair<double, double> &scale = histo->getXAxisScale();
      histogramParameters.set("x axis scale min", scale.first);
      histogramParameters.set("x axis scale max", scale.second);
    }

    customScale = histo->getYAxisScaleDefined();
    histogramParameters.set("y axis custom scale", customScale);

    if (customScale) {
      const std::pair<double, double> &scale = histo->getYAxisScale();
      histogramParameters.set("y axis scale min", scale.first);
      histogramParameters.set("y axis scale max", scale.second);
    }

    dataSet.set("histo" + ss.str(), histogramParameters);
  }

  dataSet.set("backgroundColor", getGlMainWidget()->getScene()->getBackgroundColor());
  string histoDetailedNamed = "";

  if (detailedHistogram != nullptr) {
    histoDetailedNamed = detailedHistogram->getPropertyName();
  }

  dataSet.set("histo detailed name", histoDetailedNamed);

  return dataSet;
}

void HistogramView::showPropertiesSelectionWidget() {
  WorkspacePanel *wp = static_cast<WorkspacePanel *>(graphicsView()->parentWidget());
  wp->showConfigurationTab("Properties");
}

bool HistogramView::eventFilter(QObject *object, QEvent *event) {
  if (xAxisDetail != nullptr && event->type() == QEvent::ToolTip &&
      !detailedHistogram->uniformQuantificationHistogram()) {
    GlMainWidget *glw = getGlMainWidget();
    QHelpEvent *he = static_cast<QHelpEvent *>(event);
    int x = glw->width() - he->x();
    int y = he->y();
    Coord screenCoords(x, y, 0);
    Coord sceneCoords(glw->getScene()->getLayer("Main")->getCamera().viewportTo3DWorld(
        glw->screenToViewport(screenCoords)));
    BoundingBox xAxisBB = xAxisDetail->getBoundingBox();

    if (sceneCoords.getX() > xAxisBB[0][0] && sceneCoords.getX() < xAxisBB[1][0] &&
        sceneCoords.getY() > xAxisBB[0][1] && sceneCoords.getY() < xAxisBB[1][1]) {
      double val = xAxisDetail->getValueForAxisPoint(sceneCoords);
      string valStr(getStringFromNumber(val));
      QToolTip::showText(he->globalPos(), tlp::tlpStringToQString(valStr));
    }

    return true;
  }

  return GlMainView::eventFilter(object, event);
}

void HistogramView::viewConfigurationChanged() {
  getGlMainWidget()->getScene()->setBackgroundColor(histoOptionsWidget->getBackgroundColor());
  bool dataLocationChanged = propertiesSelectionWidget->getDataLocation() != dataLocation;

  if (dataLocationChanged) {
    histogramsComposite->reset(true);
    axisComposite->reset(false);
    histogramsMap.clear();
    detailedHistogram = nullptr;
  }

  buildHistograms();

  if (detailedHistogram != nullptr && lastNbHistograms != 0 && !dataLocationChanged) {

    detailedHistogram->setNbHistogramBins(histoOptionsWidget->getNbOfHistogramBins());
    detailedHistogram->setNbXGraduations(histoOptionsWidget->getNbXGraduations());
    detailedHistogram->setYAxisIncrementStep(histoOptionsWidget->getYAxisIncrementStep());
    detailedHistogram->setXAxisLogScale(histoOptionsWidget->xAxisLogScaleSet());
    detailedHistogram->setYAxisLogScale(histoOptionsWidget->yAxisLogScaleSet());
    detailedHistogram->setCumulativeHistogram(histoOptionsWidget->cumulativeFrequenciesHisto());
    detailedHistogram->setUniformQuantification(histoOptionsWidget->uniformQuantification());
    detailedHistogram->setDisplayGraphEdges(histoOptionsWidget->showGraphEdges());
    detailedHistogram->setXAxisScaleDefined(histoOptionsWidget->useCustomXAxisScale());
    detailedHistogram->setXAxisScale(histoOptionsWidget->getXAxisScale());
    detailedHistogram->setYAxisScaleDefined(histoOptionsWidget->useCustomYAxisScale());
    detailedHistogram->setYAxisScale(histoOptionsWidget->getYAxisScale());
    detailedHistogram->setLayoutUpdateNeeded();
    detailedHistogram->update();
    histoOptionsWidget->setBinWidth(detailedHistogram->getHistogramBinsWidth());
    histoOptionsWidget->setYAxisIncrementStep(detailedHistogram->getYAxisIncrementStep());
  }

  updateHistograms(detailedHistogram);
  draw();
  drawOverview(true);
}

void HistogramView::propertiesSelected(bool flag) {
  noPropertyMsgBox->setVisible(!flag);
  toggleInteractors(flag);
  if (quickAccessBarVisible())
    _quickAccessBar->setEnabled(flag);
  setOverviewVisible(flag);
}

void HistogramView::draw() {
  GlMainWidget *gl = getGlMainWidget();

  if (selectedProperties.empty()) {
    if (!interactors().empty()) {
      setCurrentInteractor(interactors().front());
    }

    if (!smallMultiplesView) {
      switchFromDetailedViewToSmallMultiples();
    }

    propertiesSelected(false);
    gl->centerScene();

    lastNbHistograms = 0;
    return;
  }

  propertiesSelected(true);

  if (detailedHistogram != nullptr) {
    needUpdateHistogram = true;
    detailedHistogram->update();
    updateDetailedHistogramAxis();
  } else {
    updateHistograms();
  }

  if (!smallMultiplesView && detailedHistogram != nullptr) {
    switchFromSmallMultiplesToDetailedView(detailedHistogram);
  }

  if (!smallMultiplesView &&
      (detailedHistogram == nullptr || (selectedProperties.size() > 1 && lastNbHistograms == 1))) {
    switchFromDetailedViewToSmallMultiples();
  }

  if (selectedProperties.size() == 1) {
    switchFromSmallMultiplesToDetailedView(histogramsMap[selectedProperties[0]]);
    propertiesSelectionWidget->setWidgetEnabled(true);
  }

  if (lastNbHistograms != selectedProperties.size()) {
    centerView();
    lastNbHistograms = selectedProperties.size();
  } else
    gl->draw();
}

void HistogramView::refresh() {
  getGlMainWidget()->redraw();
}

void HistogramView::graphChanged(Graph *g) {
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
  drawOverview();
  if (currentInteractor())
    // force interactor refresh
    // needed by statistics interactor
    currentInteractor()->install(graphicsView());
}

void HistogramView::buildHistograms() {

  getGlMainWidget()->makeCurrent();

  histogramsComposite->reset(false);
  labelsComposite->reset(true);

  selectedProperties = propertiesSelectionWidget->getSelectedGraphProperties();
  dataLocation = propertiesSelectionWidget->getDataLocation();

  if (selectedProperties.empty()) {
    return;
  }

  float spaceBetweenOverviews = OVERVIEW_SIZE / 10.f;
  float labelHeight = OVERVIEW_SIZE / 6.0f;

  float squareRoot = sqrt(float(selectedProperties.size()));
  const unsigned int N =
      uint(squareRoot) + (fmod(float(selectedProperties.size()), squareRoot) == 0.f ? 0u : 1u);

  Color backgroundColor(histoOptionsWidget->getBackgroundColor());
  getGlMainWidget()->getScene()->setBackgroundColor(backgroundColor);

  Color foregroundColor;
  int bgV = backgroundColor.getV();

  if (bgV < 128) {
    foregroundColor = Color(255, 255, 255);
  } else {
    foregroundColor = Color(0, 0, 0);
  }

  vector<GlLabel *> propertiesLabels;
  float minSize = FLT_MIN;

  // disable user input
  // before allowing some display feedback
  tlp::disableQtUserInput();

  for (size_t i = 0; i < selectedProperties.size(); ++i) {

    unsigned int row = i / N;
    unsigned int col = i % N;

    Coord overviewBLCorner(
        col * (OVERVIEW_SIZE + spaceBetweenOverviews),
        -(labelHeight + row * (OVERVIEW_SIZE + spaceBetweenOverviews + labelHeight)), 0);
    ostringstream oss;
    oss << "histogram overview for property " << selectedProperties[i];

    if (histogramsMap.find(selectedProperties[i]) == histogramsMap.end()) {
      Histogram *histoOverview = new Histogram(
          _histoGraph, edgeAsNodeGraph, edgeToNode, selectedProperties[i], dataLocation,
          overviewBLCorner, OVERVIEW_SIZE, backgroundColor, foregroundColor);
      histogramsMap[selectedProperties[i]] = histoOverview;
    } else {
      histogramsMap[selectedProperties[i]]->setDataLocation(dataLocation);
      histogramsMap[selectedProperties[i]]->setBLCorner(overviewBLCorner);
      histogramsMap[selectedProperties[i]]->setBackgroundColor(backgroundColor);
      histogramsMap[selectedProperties[i]]->setTextColor(foregroundColor);
    }

    histogramsComposite->addGlEntity(histogramsMap[selectedProperties[i]], oss.str());

    GlLabel *propertyLabel =
        new GlLabel(Coord(overviewBLCorner.getX() + OVERVIEW_SIZE / 2,
                          overviewBLCorner.getY() - labelHeight / 2, 0),
                    Size((8.f / 10.f) * OVERVIEW_SIZE, labelHeight), foregroundColor);
    propertyLabel->setText(selectedProperties[i]);
    propertiesLabels.push_back(propertyLabel);

    if (i == 0)
      minSize = propertyLabel->getHeightAfterScale();
    else {
      if (minSize > propertyLabel->getHeightAfterScale())
        minSize = propertyLabel->getHeightAfterScale();
    }

    labelsComposite->addGlEntity(propertyLabel, selectedProperties[i] + " label");

    if (selectedProperties.size() == 1 ||
        (detailedHistogramPropertyName == selectedProperties[i])) {
      detailedHistogram = histogramsMap[selectedProperties[i]];
    }

    // add some feedback
    if (i % 10 == 0)
      QApplication::processEvents();
  }

  // re-enable user input
  tlp::enableQtUserInput();

  for (auto label : propertiesLabels) {
    label->setSize(Size(label->getSize()[0], minSize));
  }
}

void HistogramView::updateHistograms(Histogram *detailOverview) {
  needUpdateHistogram = false;
  getGlMainWidget()->makeCurrent();

  for (auto &prop : selectedProperties) {
    auto histo = histogramsMap[prop];
    if (histo != detailOverview) {
      histo->setUpdateNeeded();
      histo->update();
    }
  }
}

vector<Histogram *> HistogramView::getHistograms() const {
  vector<Histogram *> ret;

  for (auto &prop : selectedProperties) {
    ret.push_back(histogramsMap.find(prop)->second);
  }

  return ret;
}

void HistogramView::destroyHistogramsIfNeeded() {
  vector<string> propertiesToRemove;

  for (auto &prop : selectedProperties) {
    if (!_histoGraph || !_histoGraph->existProperty(prop)) {
      auto it = histogramsMap.find(prop);
      if (it->second == detailedHistogram) {
        if (!smallMultiplesView) {
          mainLayer->deleteGlEntity(detailedHistogram->getBinsComposite());
        }

        detailedHistogram = nullptr;
      }

      propertiesToRemove.push_back(prop);
      delete it->second;
      histogramsMap.erase(it);
    }
  }

  for (auto &prop : propertiesToRemove) {
    selectedProperties.erase(remove(selectedProperties.begin(), selectedProperties.end(), prop),
                             selectedProperties.end());
  }
}

void HistogramView::switchFromSmallMultiplesToDetailedView(Histogram *histogramToDetail) {
  if (!histogramToDetail)
    return;

  if (smallMultiplesView) {
    sceneRadiusBak = getGlMainWidget()->getScene()->getGraphCamera().getSceneRadius();
    zoomFactorBak = getGlMainWidget()->getScene()->getGraphCamera().getZoomFactor();
    eyesBak = getGlMainWidget()->getScene()->getGraphCamera().getEyes();
    centerBak = getGlMainWidget()->getScene()->getGraphCamera().getCenter();
    upBak = getGlMainWidget()->getScene()->getGraphCamera().getUp();
  }

  mainLayer->deleteGlEntity(histogramsComposite);
  mainLayer->deleteGlEntity(labelsComposite);

  if (detailedHistogram)
    _histoGraph->getProperty(detailedHistogram->getPropertyName())->removeListener(this);

  detailedHistogram = histogramToDetail;
  detailedHistogramPropertyName = detailedHistogram->getPropertyName();
  _histoGraph->getProperty(detailedHistogramPropertyName)->addListener(this);

  updateDetailedHistogramAxis();

  mainLayer->addGlEntity(axisComposite, "axis composite");
  mainLayer->addGlEntity(histogramToDetail->getBinsComposite(), "bins composite");

  float offset = detailedHistogram->getYAxis()->getMaxLabelWidth() + 90;
  Coord brCoord(detailedHistogram->getYAxis()->getAxisBaseCoord() - Coord(offset, 0, 0));
  Coord tlCoord(detailedHistogram->getYAxis()->getAxisBaseCoord() - Coord(offset + 65, 0, 0) +
                Coord(0, detailedHistogram->getYAxis()->getAxisLength()));
  delete emptyRect;
  emptyRect = new GlRect(tlCoord, brCoord, Color(0, 0, 0, 0), Color(0, 0, 0, 0));

  float offset2 = (detailedHistogram->getXAxis()->getAxisGradsWidth() / 2.) +
                  detailedHistogram->getXAxis()->getLabelHeight();
  Coord tlCoord2(detailedHistogram->getXAxis()->getAxisBaseCoord() - Coord(0, offset2, 0));
  Coord brCoord2(detailedHistogram->getXAxis()->getAxisBaseCoord() +
                 Coord(detailedHistogram->getXAxis()->getAxisLength(), 0, 0) -
                 Coord(0, offset2 + 60, 0));
  delete emptyRect2;
  emptyRect2 = new GlRect(tlCoord2, brCoord2, Color(0, 0, 0, 0), Color(0, 0, 0, 0));

  mainLayer->addGlEntity(emptyRect, "emptyRect");
  mainLayer->addGlEntity(emptyRect2, "emptyRect2");

  mainLayer->addGlEntity(histogramToDetail->getGlGraphComposite(), "graph");

  toggleInteractors(true);

  if (smallMultiplesView)
    centerView();

  smallMultiplesView = false;

  if (selectedProperties.size() > 1) {
    propertiesSelectionWidget->setWidgetEnabled(false);
  }

  histoOptionsWidget->setWidgetEnabled(true);

  histoOptionsWidget->enableShowGraphEdgesCB(dataLocation == NODE);
  histoOptionsWidget->setUniformQuantification(detailedHistogram->uniformQuantificationHistogram());
  histoOptionsWidget->setNbOfHistogramBins(detailedHistogram->getNbHistogramBins());
  histoOptionsWidget->setBinWidth(detailedHistogram->getHistogramBinsWidth());
  histoOptionsWidget->setYAxisIncrementStep(detailedHistogram->getYAxisIncrementStep());
  histoOptionsWidget->setYAxisLogScale(detailedHistogram->yAxisLogScaleSet());
  histoOptionsWidget->setNbXGraduations(detailedHistogram->getNbXGraduations());
  histoOptionsWidget->setXAxisLogScale(detailedHistogram->xAxisLogScaleSet());
  histoOptionsWidget->setCumulativeFrequenciesHistogram(
      detailedHistogram->cumulativeFrequenciesHistogram());
  histoOptionsWidget->setShowGraphEdges(detailedHistogram->displayGraphEdges());
  histoOptionsWidget->useCustomXAxisScale(detailedHistogram->getXAxisScaleDefined());
  histoOptionsWidget->setXAxisScale(detailedHistogram->getXAxisScale());
  histoOptionsWidget->useCustomYAxisScale(detailedHistogram->getYAxisScaleDefined());
  histoOptionsWidget->setYAxisScale(detailedHistogram->getYAxisScale());
  histoOptionsWidget->setInitXAxisScale(detailedHistogram->getInitXAxisScale());
  histoOptionsWidget->setInitYAxisScale(detailedHistogram->getInitYAxisScale());

  getGlMainWidget()->draw();
}

void HistogramView::switchFromDetailedViewToSmallMultiples() {

  if (needUpdateHistogram)
    updateHistograms();

  mainLayer->addGlEntity(emptyGlGraphComposite, "graph");

  mainLayer->deleteGlEntity(axisComposite);
  mainLayer->deleteGlEntity(emptyRect);
  mainLayer->deleteGlEntity(emptyRect2);
  delete emptyRect;
  delete emptyRect2;
  emptyRect = emptyRect2 = nullptr;

  if (detailedHistogram != nullptr) {
    mainLayer->deleteGlEntity(detailedHistogram->getBinsComposite());
  }

  detailedHistogram = nullptr;
  detailedHistogramPropertyName = "";
  GlMainWidget *gl = getGlMainWidget();
  xAxisDetail = nullptr;
  yAxisDetail = nullptr;
  mainLayer->addGlEntity(histogramsComposite, "overviews composite");
  mainLayer->addGlEntity(labelsComposite, "labels composite");
  Camera &cam = gl->getScene()->getGraphCamera();
  cam.setSceneRadius(sceneRadiusBak);
  cam.setZoomFactor(zoomFactorBak);
  cam.setEyes(eyesBak);
  cam.setCenter(centerBak);
  cam.setUp(upBak);

  smallMultiplesView = true;

  toggleInteractors(false);
  propertiesSelectionWidget->setWidgetEnabled(true);
  histoOptionsWidget->setWidgetEnabled(false);
  histoOptionsWidget->resetAxisScale();

  gl->draw();
}

void HistogramView::toggleInteractors(const bool activate) {
  View::toggleInteractors(activate, {InteractorName::HistogramInteractorNavigation});
}

void HistogramView::updateDetailedHistogramAxis() {
  GlQuantitativeAxis *xAxis = detailedHistogram->getXAxis();
  GlQuantitativeAxis *yAxis = detailedHistogram->getYAxis();
  xAxis->addCaption(GlAxis::BELOW, 100, false, 300, 155, detailedHistogram->getPropertyName());
  yAxis->addCaption(GlAxis::LEFT, 100, false, 300, 155,
                    (dataLocation == NODE ? "number of nodes" : "number of edges"));

  if (xAxis->getCaptionHeight() > yAxis->getCaptionHeight())
    xAxis->setCaptionHeight(yAxis->getCaptionHeight(), false);
  else
    yAxis->setCaptionHeight(xAxis->getCaptionHeight(), false);

  axisComposite->reset(false);
  axisComposite->addGlEntity(xAxis, "x axis");
  axisComposite->addGlEntity(yAxis, "y axis");

  if (xAxis->getSpaceBetweenAxisGrads() > yAxis->getSpaceBetweenAxisGrads())
    xAxis->setGradsLabelsHeight(yAxis->getSpaceBetweenAxisGrads() / 2.);
  else
    yAxis->setGradsLabelsHeight(xAxis->getSpaceBetweenAxisGrads() / 2.);

  xAxisDetail = xAxis;
  yAxisDetail = yAxis;
}

BoundingBox HistogramView::getSmallMultiplesBoundingBox() const {
  GlBoundingBoxSceneVisitor glBBSV(nullptr);
  histogramsComposite->acceptVisitor(&glBBSV);
  labelsComposite->acceptVisitor(&glBBSV);
  return glBBSV.getBoundingBox();
}

void HistogramView::registerTriggers() {
  for (auto obs : triggers()) {
    removeRedrawTrigger(obs);
  }

  if (graph()) {
    addRedrawTrigger(graph());
    for (auto prop : graph()->getObjectProperties()) {
      addRedrawTrigger(prop);
    }
  }
}

void HistogramView::interactorsInstalled(const QList<tlp::Interactor *> &) {
  toggleInteractors(detailedHistogram != nullptr);
}

void HistogramView::applySettings() {
  if (propertiesSelectionWidget->configurationChanged() ||
      histoOptionsWidget->configurationChanged())
    viewConfigurationChanged();
}

void HistogramView::treatEvent(const Event &message) {
  if (typeid(message) == typeid(GraphEvent)) {
    const GraphEvent *graphEvent = dynamic_cast<const GraphEvent *>(&message);

    if (graphEvent) {
      if (graphEvent->getType() == GraphEvent::TLP_ADD_NODE)
        addNode(graphEvent->getGraph(), graphEvent->getNode());

      if (graphEvent->getType() == GraphEvent::TLP_ADD_EDGE)
        addEdge(graphEvent->getGraph(), graphEvent->getEdge());

      if (graphEvent->getType() == GraphEvent::TLP_DEL_NODE)
        delNode(graphEvent->getGraph(), graphEvent->getNode());

      if (graphEvent->getType() == GraphEvent::TLP_DEL_EDGE)
        delEdge(graphEvent->getGraph(), graphEvent->getEdge());
    }
  }

  if (typeid(message) == typeid(PropertyEvent)) {
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
}

void HistogramView::afterSetNodeValue(PropertyInterface *p, const node n) {
  if (p->getGraph() == edgeAsNodeGraph && p->getName() == "viewSelection") {
    BooleanProperty *edgeAsNodeGraphSelection = static_cast<BooleanProperty *>(p);
    BooleanProperty *viewSelection = _histoGraph->getProperty<BooleanProperty>("viewSelection");
    viewSelection->removeListener(this);
    viewSelection->setEdgeValue(nodeToEdge[n], edgeAsNodeGraphSelection->getNodeValue(n));
    viewSelection->addListener(this);
    setUpdateNeeded();
    return;
  }

  afterSetAllNodeValue(p);
}

void HistogramView::afterSetEdgeValue(PropertyInterface *p, const edge e) {
  if (edgeToNode.find(e) == edgeToNode.end())
    return;

  if (p->getName() == "viewColor") {
    ColorProperty *edgeAsNodeGraphColors = edgeAsNodeGraph->getProperty<ColorProperty>("viewColor");
    ColorProperty *viewColor = static_cast<ColorProperty *>(p);
    edgeAsNodeGraphColors->setNodeValue(edgeToNode[e], viewColor->getEdgeValue(e));
    setUpdateNeeded();
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
    setUpdateNeeded();
  }
}

void HistogramView::afterSetAllNodeValue(PropertyInterface *p) {
  if (detailedHistogram && p->getName() == detailedHistogram->getPropertyName()) {
    setLayoutUpdateNeeded();
  } else if (p->getName() == "viewSize") {
    setSizesUpdateNeeded();
  } else if (p->getName() == "viewSelection") {
    if (p->getGraph() == edgeAsNodeGraph) {
      BooleanProperty *edgeAsNodeGraphSelection = static_cast<BooleanProperty *>(p);
      BooleanProperty *viewSelection = _histoGraph->getProperty<BooleanProperty>("viewSelection");
      viewSelection->setAllEdgeValue(
          edgeAsNodeGraphSelection->getNodeValue(edgeAsNodeGraph->getOneNode()));
    }

    setUpdateNeeded();
  } else if (p->getName() == "viewColor" || p->getName() == "viewShape" ||
             p->getName() == "viewTexture") {
    setUpdateNeeded();
  }
}

void HistogramView::afterSetAllEdgeValue(PropertyInterface *p) {

  if (detailedHistogram && p->getName() == detailedHistogram->getPropertyName()) {
    setLayoutUpdateNeeded();
  }

  if (p->getName() == "viewColor") {
    ColorProperty *edgeAsNodeGraphColors = edgeAsNodeGraph->getProperty<ColorProperty>("viewColor");
    ColorProperty *viewColor = static_cast<ColorProperty *>(p);
    edgeAsNodeGraphColors->setAllNodeValue(viewColor->getEdgeDefaultValue());
    setUpdateNeeded();
  } else if (p->getName() == "viewLabel") {
    StringProperty *edgeAsNodeGraphLabels =
        edgeAsNodeGraph->getProperty<StringProperty>("viewLabel");
    StringProperty *viewLabel = static_cast<StringProperty *>(p);
    edgeAsNodeGraphLabels->setAllNodeValue(viewLabel->getEdgeDefaultValue());
  } else if (p->getName() == "viewSelection") {
    BooleanProperty *edgeAsNodeGraphSelection =
        edgeAsNodeGraph->getProperty<BooleanProperty>("viewSelection");
    BooleanProperty *viewSelection = static_cast<BooleanProperty *>(p);
    for (auto e : _histoGraph->edges()) {
      if (edgeAsNodeGraphSelection->getNodeValue(edgeToNode[e]) != viewSelection->getEdgeValue(e)) {
        edgeAsNodeGraphSelection->setNodeValue(edgeToNode[e], viewSelection->getEdgeValue(e));
      }
    }

    setUpdateNeeded();
  }
}

void HistogramView::addNode(Graph *, const node) {
  setLayoutUpdateNeeded();
  setSizesUpdateNeeded();
}

void HistogramView::addEdge(Graph *, const edge e) {
  edgeToNode[e] = edgeAsNodeGraph->addNode();
  setLayoutUpdateNeeded();
  setSizesUpdateNeeded();
}

void HistogramView::delNode(Graph *, const node) {
  setLayoutUpdateNeeded();
  setSizesUpdateNeeded();
}

void HistogramView::delEdge(Graph *, const edge e) {
  edgeAsNodeGraph->delNode(edgeToNode[e]);
  edgeToNode.erase(e);
  setLayoutUpdateNeeded();
  setSizesUpdateNeeded();
}

unsigned int HistogramView::getMappedId(unsigned int id) {
  if (dataLocation == EDGE)
    return nodeToEdge[node(id)].id;

  return id;
}
} // namespace tlp
