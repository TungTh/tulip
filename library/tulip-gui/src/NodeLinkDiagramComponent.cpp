/**
 *
 * This file is part of Tulip (www.tulip-software.org)
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
#include "ui_GridOptionsWidget.h"

#include <QGraphicsView>
#include <QActionGroup>
#include <QDialog>
#include <QMenu>
#include <QHelpEvent>
#include <QToolTip>
#include <QString>
#include <QEvent>

#include <tulip/GlGrid.h>
#include <tulip/GlLayer.h>
#include <tulip/GlGraph.h>
#include <tulip/DrawingTools.h>
#include <tulip/TulipItemDelegate.h>
#include <tulip/ParameterListModel.h>
#include <tulip/GlMainWidget.h>
#include <tulip/GlGraphInputData.h>
#include <tulip/GlRect2D.h>
#include <tulip/GlOverviewGraphicsItem.h>
#include <tulip/Interactor.h>
#include <tulip/TulipMetaTypes.h>
#include <tulip/QtGlSceneZoomAndPanAnimator.h>
#include <tulip/TlpTools.h>
#include <tulip/TlpQtTools.h>
#include <tulip/NodeLinkDiagramComponent.h>
#include <tulip/GraphModel.h>
#include <tulip/NumericProperty.h>
#include <tulip/OpenGlConfigManager.h>

using namespace tlp;
using namespace std;

const string NodeLinkDiagramComponent::viewName("Node Link Diagram view");

NodeLinkDiagramComponent::NodeLinkDiagramComponent(const tlp::PluginContext *)
    : _grid(nullptr), _gridOptions(nullptr), _hasHulls(false), _tooltips(false), grid_ui(nullptr) {
}

NodeLinkDiagramComponent::~NodeLinkDiagramComponent() {
  delete grid_ui;
}

void NodeLinkDiagramComponent::updateGrid() {
  delete _grid;
  _grid = nullptr;

  if (_gridOptions == nullptr)
    return;

  DataSet gridData = static_cast<ParameterListModel *>(_gridOptions->findChild<QTableView *>()->model())->parametersValues();
  StringCollection gridMode;
  gridData.get<StringCollection>("Grid mode", gridMode);
  int mode = gridMode.getCurrent();

  if (mode == 0)
    return;

  Coord margins;
  Size gridSize;
  Color gridColor;
  bool onX = true, onY = true, onZ = true;
  gridData.get<Coord>("Margins", margins);
  gridData.get<Size>("Grid size", gridSize);
  gridData.get<Color>("Grid color", gridColor);
  gridData.get<bool>("X grid", onX);
  gridData.get<bool>("Y grid", onY);
  gridData.get<bool>("Z grid", onZ);

  GlGraphInputData &inputData = getGlMainWidget()->getScene()->getMainGlGraph()->getInputData();
  BoundingBox graphBB = computeBoundingBox(graph(), inputData.getElementLayout(), inputData.getElementSize(), inputData.getElementRotation());
  Coord bottomLeft = Coord(graphBB[0] - margins);
  Coord topRight = Coord(graphBB[1] + margins);

  if (mode == 1) {
    for (int i = 0; i < 3; ++i)
      gridSize[i] = abs(topRight[i] - bottomLeft[i]) / gridSize[i];
  }

  bool displays[3];
  displays[0] = onX;
  displays[1] = onY;
  displays[2] = onZ;

  _grid = new GlGrid(bottomLeft, topRight, gridSize, gridColor, displays);
  getGlMainWidget()->getScene()->getMainLayer()->addGlEntity(_grid, "Node Link Diagram Component grid");
}

void NodeLinkDiagramComponent::draw() {
  updateGrid();
  GlMainView::draw();
}

void NodeLinkDiagramComponent::setupWidget() {
  GlMainView::setupWidget();
  graphicsView()->installEventFilter(this); // Handle tooltip events
}

bool NodeLinkDiagramComponent::eventFilter(QObject *obj, QEvent *event) {
  if (_tooltips == true && event->type() == QEvent::ToolTip) {
    SelectedEntity type;
    QHelpEvent *he = static_cast<QHelpEvent *>(event);
    GlMainWidget *gl = getGlMainWidget();

    if (gl->pickNodesEdges(he->x(), he->y(), type)) {
      // try to show the viewLabel if any
      StringProperty *labels = graph()->getProperty<StringProperty>("viewLabel");
      std::string label;
      QString ttip;
      node tmpNode = type.getNode();

      if (tmpNode.isValid()) {
        label = labels->getNodeValue(tmpNode);

        if (!label.empty())
          ttip = tlpStringToQString(label) + " (";

        ttip += QString("node #") + QString::number(tmpNode.id);

        if (!label.empty())
          ttip += ")";

        QToolTip::showText(he->globalPos(), ttip, gl);
        return true;
      } else {
        edge tmpEdge = type.getEdge();

        if (tmpEdge.isValid()) {
          label = labels->getEdgeValue(tmpEdge);

          if (!label.empty())
            ttip = tlpStringToQString(label) + "(";

          ttip += QString("edge #") + QString::number(tmpEdge.id);

          if (!label.empty())
            ttip += ")";

          QToolTip::showText(he->globalPos(), ttip, gl);
          return true;
        }
      }
    } else { // be sure to hide the tooltip if the mouse cursor is not under a node or an edge
      QToolTip::hideText();
      event->ignore();
    }
  }

  // standard event processing
  return GlMainView::eventFilter(obj, event);
}

void NodeLinkDiagramComponent::setState(const tlp::DataSet &data) {
  ParameterDescriptionList gridParameters;
  gridParameters.add<StringCollection>("Grid mode", "", "No grid;Space divisions;Fixed size", true);
  gridParameters.add<Size>("Grid size", "", "(1,1,1)", false);
  gridParameters.add<Size>("Margins", "", "(0.5,0.5,0.5)", false);
  gridParameters.add<Color>("Grid color", "", "(0,0,0,255)", false);
  gridParameters.add<bool>("X grid", "", "true", false);
  gridParameters.add<bool>("Y grid", "", "true", false);
  gridParameters.add<bool>("Z grid", "", "true", false);
  ParameterListModel *model = new ParameterListModel(gridParameters, nullptr, this);

  grid_ui = new Ui::GridOptionsWidget;
  _gridOptions = new QDialog(graphicsView());
  grid_ui->setupUi(_gridOptions);
  grid_ui->tableView->setModel(model);
  grid_ui->tableView->setItemDelegate(new TulipItemDelegate);
  connect(grid_ui->tableView, SIGNAL(destroyed()), grid_ui->tableView->itemDelegate(), SLOT(deleteLater()));

  setOverviewVisible(true);
  setQuickAccessBarVisible(true);
  GlMainView::setState(data);

  bool keepSPOV = false;
  data.get<bool>("keepScenePointOfViewOnSubgraphChanging", keepSPOV);
  getGlMainWidget()->setKeepScenePointOfViewOnSubgraphChanging(keepSPOV);

  createScene(graph(), data);
  registerTriggers();

  if (overviewItem())
    overviewItem()->setLayerVisible("Foreground", false);
}

void NodeLinkDiagramComponent::graphChanged(tlp::Graph *graph) {
  GlGraph *glGraph = getGlMainWidget()->getScene()->getMainGlGraph();
  Graph *oldGraph = glGraph ? glGraph->getGraph() : nullptr;
  loadGraphOnScene(graph);
  registerTriggers();

  if (oldGraph == nullptr || graph == nullptr || (oldGraph->getRoot() != graph->getRoot()) ||
      getGlMainWidget()->keepScenePointOfViewOnSubgraphChanging() == false)
    centerView();

  emit drawNeeded();
  drawOverview();
}

tlp::DataSet NodeLinkDiagramComponent::state() const {
  DataSet data = sceneData();
  data.set("keepScenePointOfViewOnSubgraphChanging", getGlMainWidget()->keepScenePointOfViewOnSubgraphChanging());
  return data;
}

//==================================================
void NodeLinkDiagramComponent::createScene(Graph *graph, DataSet dataSet) {

  GlScene *scene = getGlMainWidget()->getScene();
  scene->clearLayersList();

  GlLayer *layer = new GlLayer("Main");
  GlLayer *backgroundLayer = new GlLayer("Background", false);
  backgroundLayer->setVisible(false);
  GlLayer *foregroundLayer = new GlLayer("Foreground", false);
  foregroundLayer->setVisible(false);

  std::string dir = TulipBitmapDir;
  GlRect2D *labri = new GlRect2D(35.f, 5.f, 50.f, 50.f, tlp::Color::White);
  labri->setTexture(dir + "logolabri.jpg");
  labri->setStencil(1);
  labri->setVisible(false);
  foregroundLayer->addGlEntity(labri, "labrilogo");

  scene->addExistingLayer(backgroundLayer);
  scene->addExistingLayer(layer);
  scene->addExistingLayer(foregroundLayer);

  scene->addMainGlGraph(graph);
  scene->getMainGlGraph()->getRenderingParameters().setDisplayNodesLabels(true);
  scene->getMainGlGraph()->getRenderingParameters().setInterpolateEdgesColors(false);
  scene->getMainGlGraph()->getRenderingParameters().setNodesStencil(2);
  scene->getMainGlGraph()->getRenderingParameters().setNodesLabelsStencil(1);
  scene->centerScene();

  if (dataSet.exist("Display")) {
    DataSet renderingParameters;
    dataSet.get("Display", renderingParameters);
    GlGraphRenderingParameters &rp = scene->getMainGlGraph()->getRenderingParameters();
    rp.setParameters(renderingParameters);

    string s;

    if (renderingParameters.get("elementsOrderingPropertyName", s) && !s.empty()) {
      rp.setElementsOrderingProperty(dynamic_cast<tlp::NumericProperty *>(graph->getProperty(s)));
    }
  }

  getGlMainWidget()->emitGraphChanged();
}
//==================================================
DataSet NodeLinkDiagramComponent::sceneData() const {
  GlScene *scene = getGlMainWidget()->getScene();
  DataSet outDataSet = GlMainView::state();
  outDataSet.set("Display", scene->getMainGlGraph()->getRenderingParameters().getParameters());

  return outDataSet;
}
//==================================================
void NodeLinkDiagramComponent::loadGraphOnScene(Graph *graph) {
  GlScene *scene = getGlMainWidget()->getScene();

  if (!scene->getLayer("Main")) {
    createScene(graph, DataSet());
    return;
  }

  GlGraph *oldGlGraph = scene->getMainGlGraph();

  if (!oldGlGraph) {
    createScene(graph, DataSet());
    return;
  }

  scene->getMainGlGraph()->setGraph(graph);

  getGlMainWidget()->emitGraphChanged();
}

void NodeLinkDiagramComponent::registerTriggers() {
  clearRedrawTriggers();

  if (graph() == nullptr)
    return;

  addRedrawTrigger(getGlMainWidget()->getScene()->getMainGlGraph()->getGraph());
  std::set<tlp::PropertyInterface *> properties = getGlMainWidget()->getScene()->getMainGlGraph()->getInputData().getProperties();

  for (std::set<tlp::PropertyInterface *>::iterator it = properties.begin(); it != properties.end(); ++it) {
    addRedrawTrigger(*it);
  }
}

void NodeLinkDiagramComponent::setZOrdering(bool f) {
  getGlMainWidget()->getScene()->getMainGlGraph()->getRenderingParameters().setElementsZOrdered(f);
  centerView();
}

void NodeLinkDiagramComponent::showGridControl() {
  if (_gridOptions->exec() == QDialog::Rejected)
    return;

  updateGrid();
  emit drawNeeded();
}

void NodeLinkDiagramComponent::requestChangeGraph(Graph *graph) {
  this->loadGraphOnScene(graph);
  registerTriggers();
  emit graphSet(graph);
  centerView();
  draw();
}

void NodeLinkDiagramComponent::displayToolTips(bool display) {
  _tooltips = display;
}

void NodeLinkDiagramComponent::fillContextMenu(QMenu *menu, const QPointF &point) {

  // Check if a node/edge is under the mouse pointer
  bool result;
  SelectedEntity entity;
  result = getGlMainWidget()->pickNodesEdges(point.x(), getGlMainWidget()->height() - point.y(), entity);

  if (result) {
    menu->addSeparator();
    isNode = entity.getEntityType() == SelectedEntity::NODE_SELECTED;
    itemId = isNode ? entity.getNode().id : entity.getEdge().id;

    menu->addAction((isNode ? trUtf8("Node #") : trUtf8("Edge #")) + QString::number(itemId))->setEnabled(false);

    menu->addSeparator();

    QMenu *selectMenu = menu->addMenu("Select");

    if (isNode) {
      selectMenu->addAction(tr("node"), this, SLOT(addItemToSelection()));
      selectMenu->addAction(tr("predecessor nodes"), this, SLOT(addInNodesToSelection()));
      selectMenu->addAction(tr("successor nodes"), this, SLOT(addOutNodesToSelection()));
      selectMenu->addAction(tr("input edges"), this, SLOT(addInEdgesToSelection()));
      selectMenu->addAction(tr("output edges"), this, SLOT(addOutEdgesToSelection()));
      selectMenu->addAction(tr("node and all its neighbour nodes (including edges)"), this, SLOT(addNodeAndAllNeighbourNodesAndEdges()));
    } else {
      selectMenu->addAction(tr("edge"), this, SLOT(addItemToSelection()));
      selectMenu->addAction(tr("edge extremities"), this, SLOT(addExtremitiesToSelection()));
      selectMenu->addAction(tr("edge and its extremities"), this, SLOT(addEdgeAndExtremitiesToSelection()));
    }

    QMenu *toggleMenu = menu->addMenu("Toggle selection");

    if (isNode) {
      toggleMenu->addAction(tr("of node"), this, SLOT(addRemoveItemToSelection()));
      toggleMenu->addAction(tr("of predecessor nodes"), this, SLOT(addRemoveInNodesToSelection()));
      toggleMenu->addAction(tr("of successor nodes"), this, SLOT(addRemoveOutNodesToSelection()));
      toggleMenu->addAction(tr("of input edges"), this, SLOT(addRemoveInEdgesToSelection()));
      toggleMenu->addAction(tr("of output edges"), this, SLOT(addRemoveOutEdgesToSelection()));
      toggleMenu->addAction(tr("of node and all its neighbour nodes (including edges)"), this, SLOT(addRemoveNodeAndAllNeighbourNodesAndEdges()));
    } else {
      toggleMenu->addAction(tr("of edge"), this, SLOT(addRemoveItemToSelection()));
      toggleMenu->addAction(tr("of edge extremities"), this, SLOT(addRemoveExtremitiesToSelection()));
      toggleMenu->addAction(tr("of edge and its extremities"), this, SLOT(addRemoveEdgeAndExtremitiesToSelection()));
    }

    menu->addAction(tr("Delete"), this, SLOT(deleteItem()));

    QMenu *updateMenu = menu->addMenu("Edit");
    updateMenu->addAction("Color", this, SLOT(editColor()));
    updateMenu->addAction("Label", this, SLOT(editLabel()));
    updateMenu->addAction("Shape", this, SLOT(editShape()));
    updateMenu->addAction("Size", this, SLOT(editSize()));

    if (isNode) {
      Graph *metaGraph = graph()->getNodeMetaInfo(entity.getNode());

      if (metaGraph) {
        menu->addAction(tr("Go inside"), this, SLOT(goInsideItem()));
        menu->addAction(tr("Ungroup"), this, SLOT(ungroupItem()));
      }
    }
  } else {
    GlMainView::fillContextMenu(menu, point);

    QAction *actionTooltips = menu->addAction("Tooltips");
    actionTooltips->setCheckable(true);
    actionTooltips->setChecked(_tooltips);
    connect(actionTooltips, SIGNAL(triggered(bool)), this, SLOT(displayToolTips(bool)));

    QAction *zOrdering = menu->addAction(trUtf8("Use Z ordering"));
    zOrdering->setCheckable(true);
    zOrdering->setChecked(getGlMainWidget()->getScene()->getMainGlGraph()->getRenderingParameters().elementsZOrdered());
    connect(zOrdering, SIGNAL(triggered(bool)), this, SLOT(setZOrdering(bool)));
    menu->addAction(trUtf8("Grid display parameters"), this, SLOT(showGridControl()));
  }
}

void NodeLinkDiagramComponent::addRemoveItemToSelection(bool pushGraph, bool forceSelect) {
  BooleanProperty *elementSelected = graph()->getProperty<BooleanProperty>("viewSelection");
  if (pushGraph) {
    graph()->push();
  }

  // selection add/remove graph item
  if (isNode)
    elementSelected->setNodeValue(node(itemId), forceSelect || !elementSelected->getNodeValue(node(itemId)));
  else
    elementSelected->setEdgeValue(edge(itemId), forceSelect || !elementSelected->getEdgeValue(edge(itemId)));
}

void NodeLinkDiagramComponent::addItemToSelection() {
  addRemoveItemToSelection(true, true);
}

void NodeLinkDiagramComponent::selectItem() {
  BooleanProperty *elementSelected = graph()->getProperty<BooleanProperty>("viewSelection");
  graph()->push();

  // empty selection
  elementSelected->setAllNodeValue(false);
  elementSelected->setAllEdgeValue(false);

  // selection add/remove graph item
  if (isNode)
    elementSelected->setNodeValue(node(itemId), true);
  else
    elementSelected->setEdgeValue(edge(itemId), true);
}

void NodeLinkDiagramComponent::addRemoveInNodesToSelection(bool pushGraph, bool forceSelect) {
  BooleanProperty *elementSelected = graph()->getProperty<BooleanProperty>("viewSelection");
  if (pushGraph) {
    graph()->push();
  }
  node neigh;
  MutableContainer<bool> inNodes;
  for (node neigh : graph()->getInNodes(node(itemId))) {
    if (inNodes.get(neigh.id) == false) {
      elementSelected->setNodeValue(neigh, forceSelect || !elementSelected->getNodeValue(neigh));
      inNodes.set(neigh.id, true);
    }
  }
}

void NodeLinkDiagramComponent::addRemoveOutNodesToSelection(bool pushGraph, bool forceSelect) {
  BooleanProperty *elementSelected = graph()->getProperty<BooleanProperty>("viewSelection");
  if (pushGraph) {
    graph()->push();
  }
  node neigh;
  MutableContainer<bool> outNodes;
  for (node neigh : graph()->getOutNodes(node(itemId))) {
    if (outNodes.get(neigh.id) == false) {
      elementSelected->setNodeValue(neigh, forceSelect || !elementSelected->getNodeValue(neigh));
      outNodes.set(neigh.id, true);
    }
  }
}

void NodeLinkDiagramComponent::addRemoveInEdgesToSelection(bool pushGraph, bool forceSelect) {
  BooleanProperty *elementSelected = graph()->getProperty<BooleanProperty>("viewSelection");
  if (pushGraph) {
    graph()->push();
  }
  edge e;
  forEach(e, graph()->getInEdges(node(itemId))) {
    elementSelected->setEdgeValue(e, forceSelect || !elementSelected->getEdgeValue(e));
  }
}

void NodeLinkDiagramComponent::addRemoveOutEdgesToSelection(bool pushGraph, bool forceSelect) {
  BooleanProperty *elementSelected = graph()->getProperty<BooleanProperty>("viewSelection");
  if (pushGraph) {
    graph()->push();
  }
  edge e;
  forEach(e, graph()->getOutEdges(node(itemId))) {
    elementSelected->setEdgeValue(e, forceSelect || !elementSelected->getEdgeValue(e));
  }
}

void NodeLinkDiagramComponent::addRemoveNodeAndAllNeighbourNodesAndEdges(bool forceSelect) {
  graph()->push();
  addRemoveItemToSelection(false, forceSelect);
  addRemoveInEdgesToSelection(false, forceSelect);
  addRemoveOutEdgesToSelection(false, forceSelect);
  addRemoveInNodesToSelection(false, forceSelect);
  addRemoveOutNodesToSelection(false, forceSelect);
}

void NodeLinkDiagramComponent::addRemoveExtremitiesToSelection(bool pushGraph, bool forceSelect) {
  BooleanProperty *elementSelected = graph()->getProperty<BooleanProperty>("viewSelection");
  if (pushGraph) {
    graph()->push();
  }
  node src = graph()->source(edge(itemId));
  node tgt = graph()->target(edge(itemId));
  elementSelected->setNodeValue(src, forceSelect || !elementSelected->getNodeValue(src));

  if (src != tgt) {
    elementSelected->setNodeValue(tgt, forceSelect || !elementSelected->getNodeValue(tgt));
  }
}

void NodeLinkDiagramComponent::addRemoveEdgeAndExtremitiesToSelection(bool forceSelect) {
  graph()->push();
  addRemoveItemToSelection(false, forceSelect);
  addRemoveExtremitiesToSelection(false, forceSelect);
}

void NodeLinkDiagramComponent::addInNodesToSelection(bool pushGraph) {
  addRemoveInNodesToSelection(pushGraph, true);
}

void NodeLinkDiagramComponent::addOutNodesToSelection(bool pushGraph) {
  addRemoveOutNodesToSelection(pushGraph, true);
}

void NodeLinkDiagramComponent::addInEdgesToSelection(bool pushGraph) {
  addRemoveInEdgesToSelection(pushGraph, true);
}

void NodeLinkDiagramComponent::addOutEdgesToSelection(bool pushGraph) {
  addRemoveOutEdgesToSelection(pushGraph, true);
}

void NodeLinkDiagramComponent::addNodeAndAllNeighbourNodesAndEdges() {
  addRemoveNodeAndAllNeighbourNodesAndEdges(true);
}

void NodeLinkDiagramComponent::addExtremitiesToSelection(bool pushGraph) {
  addRemoveExtremitiesToSelection(pushGraph, true);
}

void NodeLinkDiagramComponent::addEdgeAndExtremitiesToSelection() {
  addRemoveEdgeAndExtremitiesToSelection(true);
}

void NodeLinkDiagramComponent::deleteItem() {
  graph()->push();

  if (isNode)
    graph()->delNode(node(itemId));
  else
    graph()->delEdge(edge(itemId));
}

void NodeLinkDiagramComponent::editValue(PropertyInterface *pi) {
  TulipItemDelegate tid(getGlMainWidget());
  QVariant val = TulipItemDelegate::showEditorDialog(isNode ? NODE : EDGE, pi, graph(), &tid, getGlMainWidget(), itemId);

  // Check if edition has been cancelled
  if (!val.isValid())
    return;

  graph()->push();

  if (isNode)
    GraphModel::setNodeValue(itemId, pi, val);
  else
    GraphModel::setEdgeValue(itemId, pi, val);
}

void NodeLinkDiagramComponent::editColor() {
  editValue(getGlMainWidget()->getScene()->getMainGlGraph()->getInputData().getElementColor());
}

void NodeLinkDiagramComponent::editLabel() {
  editValue(getGlMainWidget()->getScene()->getMainGlGraph()->getInputData().getElementLabel());
}

void NodeLinkDiagramComponent::editShape() {
  editValue(getGlMainWidget()->getScene()->getMainGlGraph()->getInputData().getElementShape());
}

void NodeLinkDiagramComponent::editSize() {
  editValue(getGlMainWidget()->getScene()->getMainGlGraph()->getInputData().getElementSize());
}

const Camera &NodeLinkDiagramComponent::goInsideItem(node meta) {
  Graph *metaGraph = graph()->getNodeMetaInfo(meta);
  Size size = getGlMainWidget()->getScene()->getMainGlGraph()->getInputData().getElementSize()->getNodeValue(meta);
  Coord coord = getGlMainWidget()->getScene()->getMainGlGraph()->getInputData().getElementLayout()->getNodeValue(meta);
  BoundingBox bb;
  bb.expand(coord - size / 2.f);
  bb.expand(coord + size / 2.f);
  QtGlSceneZoomAndPanAnimator zoomAnPan(getGlMainWidget(), bb);
  zoomAnPan.animateZoomAndPan();
  loadGraphOnScene(metaGraph);
  registerTriggers();
  emit graphSet(metaGraph);
  centerView();
  draw();
  return *(getGlMainWidget()->getScene()->getMainLayer()->getCamera());
}

void NodeLinkDiagramComponent::goInsideItem() {
  goInsideItem(node(itemId));
}

void NodeLinkDiagramComponent::ungroupItem() {
  graph()->push();
  graph()->openMetaNode(node(itemId));
}

PLUGIN(NodeLinkDiagramComponent)
