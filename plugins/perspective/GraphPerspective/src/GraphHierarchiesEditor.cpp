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
#include "GraphHierarchiesEditor.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QGraphicsEffect>
#include <QMenu>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QTextDocument>
#include <QToolButton>
#include <QSortFilterProxyModel>
#include <QMessageBox>

#include <tulip/BooleanProperty.h>
#include <tulip/Perspective.h>
#include <tulip/TulipMetaTypes.h>
#include <tulip/FontIconManager.h>

#include "GraphPerspective.h"
#include "ui_GraphHierarchiesEditor.h"
#include <tulip/GraphHierarchiesModel.h>

CustomTreeView::CustomTreeView(QWidget *parent) : QTreeView(parent) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
  header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
  connect(this, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(resizeFirstColumnToContent()));
  connect(this, SIGNAL(expanded(const QModelIndex &)), this, SLOT(resizeFirstColumnToContent()));
}

int CustomTreeView::sizeHintForColumn(int col) const {
  if (!model() || col > 0)
    return -1;

  ensurePolished();
  int hint = 0;
  QModelIndex index = model()->index(0, col);

  while (index.isValid()) {
    if (viewport()->rect().contains(visualRect(index))) {
      hint = qMax(hint, visualRect(index).x() + itemDelegate(index)->sizeHint(viewOptions(), index).width());
    }

    index = indexBelow(index);
  }

  return qMin(hint, viewport()->rect().width());
}

void CustomTreeView::scrollContentsBy(int dx, int dy) {
  if (dy != 0 && dx == 0) {
    resizeFirstColumnToContent();
  }

  QTreeView::scrollContentsBy(dx, dy);
}

void CustomTreeView::setModel(QAbstractItemModel *m) {
  if (model()) {
    disconnect(model(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(resizeFirstColumnToContent()));
    disconnect(model(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(resizeFirstColumnToContent()));
  }

  connect(m, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(resizeFirstColumnToContent()));
  connect(m, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(resizeFirstColumnToContent()));
  QTreeView::setModel(m);
  resizeFirstColumnToContent();
}

void CustomTreeView::resizeFirstColumnToContent() {
  resizeColumnToContents(0);
}

static QColor menuIconColor = QColor("#404244");

GraphHierarchiesEditor::GraphHierarchiesEditor(QWidget *parent)
    : QWidget(parent), _ui(new Ui::GraphHierarchiesEditorData), _contextGraph(nullptr), _model(nullptr) {
  _ui->setupUi(this);

  _ui->actionCreate_panel->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::plusbox, menuIconColor));
  _ui->actionExport->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::export_, menuIconColor));
  _ui->actionSave_to_file->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::fileexport, menuIconColor));
  _ui->actionRename->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::renamebox, menuIconColor));
  _ui->actionDelete_graph->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::delete_, menuIconColor));
  _ui->actionDelete_All->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::delete_, menuIconColor));

  QToolButton *linkButton = new QToolButton();
  linkButton->setObjectName("linkButton");
  linkButton->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::linkvariant, Qt::white, 0.8, QPointF(-1, -1)));
  linkButton->setToolTip("Click here to disable the synchronization with "
                         "workspace active panel.\nWhen synchronization is "
                         "enabled, the graph currently displayed\nin the "
                         "active panel, becomes the current one in the Graphs "
                         "panel.");
  linkButton->setIconSize(QSize(23, 23));
  linkButton->setMinimumSize(25, 25);
  linkButton->setMaximumSize(25, 25);
  linkButton->setCheckable(true);
  linkButton->setChecked(true);
  _ui->header->insertWidget(linkButton);
  _linkButton = linkButton;
  connect(linkButton, SIGNAL(toggled(bool)), this, SLOT(toggleSynchronization(bool)));
  _ui->hierarchiesTree->installEventFilter(this);

  connect(_ui->hierarchiesTree, SIGNAL(clicked(const QModelIndex &)), this, SLOT(clicked(const QModelIndex &)));
}

bool GraphHierarchiesEditor::synchronized() const {
  return _linkButton->isChecked();
}

void GraphHierarchiesEditor::setModel(tlp::GraphHierarchiesModel *model) {
  _model = model;
  QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(_ui->hierarchiesTree);
  proxyModel->setSourceModel(model);
  proxyModel->setDynamicSortFilter(false);
  _ui->hierarchiesTree->setModel(proxyModel);
  _ui->hierarchiesTree->header()->resizeSection(0, 100);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  _ui->hierarchiesTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
#else
  _ui->hierarchiesTree->header()->setResizeMode(0, QHeaderView::Interactive);
#endif
  connect(_ui->hierarchiesTree->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this,
          SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));
}

GraphHierarchiesEditor::~GraphHierarchiesEditor() {
  delete _ui;
}

void GraphHierarchiesEditor::contextMenuRequested(const QPoint &p) {
  _contextIndex = _ui->hierarchiesTree->indexAt(p);

  if (_contextIndex.isValid()) {
    _contextGraph = _contextIndex.data(tlp::GraphHierarchiesModel::GraphRole).value<tlp::Graph *>();
    QMenu menu;
    menu.addAction(_ui->actionCreate_panel);
    menu.addSeparator();
    menu.addAction(_ui->actionExport);
    menu.addAction(_ui->actionSave_to_file);
    menu.addSeparator();
    menu.addAction(_ui->actionRename);
    menu.addSeparator();
    menu.addAction(_ui->actionAdd_sub_graph);
    menu.addAction(_ui->actionCreate_induced_sub_graph);
    menu.addAction(_ui->actionClone_subgraph);
    if (_contextGraph->getRoot() != _contextGraph) {
      menu.addAction(_ui->actionClone_sibling);
      menu.addAction(_ui->actionClone_sibling_with_properties);
    }
    menu.addSeparator();

    if (_contextGraph->getRoot() != _contextGraph) {
      menu.addAction(_ui->actionDelete_graph);
    }

    menu.addAction(_ui->actionDelete_All);
    menu.exec(_ui->hierarchiesTree->viewport()->mapToGlobal(p));
    _contextIndex = QModelIndex();
    _contextGraph = nullptr;
  }
}

void GraphHierarchiesEditor::clicked(const QModelIndex &index) {
  if (!index.isValid() || index.internalPointer() == nullptr)
    return;

  _contextGraph = index.data(tlp::TulipModel::GraphRole).value<tlp::Graph *>();
  _model->setCurrentGraph(_contextGraph);
  _contextGraph = nullptr;
}

void GraphHierarchiesEditor::doubleClicked(const QModelIndex &index) {
  if (!index.isValid() || index.internalPointer() == nullptr)
    return;

  _contextGraph = index.data(tlp::TulipModel::GraphRole).value<tlp::Graph *>();
  _model->setCurrentGraph(_contextGraph);
  createPanel();
  _contextGraph = nullptr;
}

void GraphHierarchiesEditor::currentChanged(const QModelIndex &index, const QModelIndex &previous) {
  if (synchronized() && index.isValid() && index.internalPointer()) {
    if (index == previous)
      return;

    _contextGraph = index.data(tlp::TulipModel::GraphRole).value<tlp::Graph *>();
    disconnect(_ui->hierarchiesTree->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this,
               SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));
    _model->setCurrentGraph(_contextGraph);
    connect(_ui->hierarchiesTree->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this,
            SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));
    _contextGraph = nullptr;
  }
}

void GraphHierarchiesEditor::addSubGraph() {
  if (_contextGraph == nullptr)
    return;

  _contextGraph->push();
  _contextGraph->addSubGraph("empty sub-graph");
}

void GraphHierarchiesEditor::cloneSubGraph() {
  if (_contextGraph == nullptr)
    return;

  _contextGraph->push();
  std::string sgName("clone sub-graph of ");
  _contextGraph->addCloneSubGraph(sgName + _contextGraph->getName());
}

void GraphHierarchiesEditor::cloneSibling() {
  if (_contextGraph == nullptr)
    return;

  _contextGraph->push();
  std::string sgName("clone sibling of ");
  _contextGraph->addCloneSubGraph(sgName + _contextGraph->getName(), true);
}

void GraphHierarchiesEditor::cloneSiblingWithProperties() {
  if (_contextGraph == nullptr)
    return;

  _contextGraph->push();
  std::string sgName("clone sibling of ");
  _contextGraph->addCloneSubGraph(sgName + _contextGraph->getName(), true, true);
}

void GraphHierarchiesEditor::addInducedSubGraph() {
  if (_contextGraph == nullptr)
    return;

  GraphPerspective *persp = GraphPerspective::typedInstance<GraphPerspective>();
  persp->createSubGraph(_contextGraph);
}

void GraphHierarchiesEditor::delGraph() {
  if (_contextGraph == nullptr && !_ui->hierarchiesTree->selectionModel()->selectedRows(0).empty()) {
    _contextGraph = _ui->hierarchiesTree->selectionModel()->selectedRows(0)[0].data(tlp::TulipModel::GraphRole).value<tlp::Graph *>();
  }

  if (_contextGraph == nullptr)
    return;

  GraphPerspective::typedInstance<GraphPerspective>()->closePanelsForGraph(_contextGraph);
  _contextGraph->push();

  if (_contextGraph->getRoot() == _contextGraph) {
    delete _contextGraph;
    _model->setCurrentGraph(nullptr);
  } else {
    tlp::Graph *sg = _contextGraph->getSuperGraph();
    _contextGraph->getSuperGraph()->delSubGraph(_contextGraph);
    _model->setCurrentGraph(sg);
  }

  _contextGraph = nullptr;
}

void GraphHierarchiesEditor::delAllGraph() {
  if (_contextGraph == nullptr && !_ui->hierarchiesTree->selectionModel()->selectedRows(0).empty()) {
    _contextGraph = _ui->hierarchiesTree->selectionModel()->selectedRows(0)[0].data(tlp::TulipModel::GraphRole).value<tlp::Graph *>();
  }

  if (_contextGraph == nullptr)
    return;

  if (_contextGraph->getRoot() == _contextGraph) {
    if (QMessageBox::question(parentWidget(), "Delete a whole hierarchy",
                              "You are going to delete a complete graph hierarchy. This operation cannot be undone. Do you really want to continue?",
                              QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok) {
      GraphPerspective::typedInstance<GraphPerspective>()->closePanelsForGraph(_contextGraph);
      delete _contextGraph;
      _model->setCurrentGraph(nullptr);
    }
  } else {
    _contextGraph->push();
    GraphPerspective::typedInstance<GraphPerspective>()->closePanelsForGraph(_contextGraph);
    tlp::Graph *sg = _contextGraph->getSuperGraph();
    _contextGraph->getSuperGraph()->delAllSubGraphs(_contextGraph);
    _model->setCurrentGraph(sg);
  }

  _contextGraph = nullptr;
}

void GraphHierarchiesEditor::createPanel() {
  tlp::Graph *g = _contextGraph;

  if (g == nullptr) {
    g = _model->currentGraph();

    if (g == nullptr)
      return;
  }

  tlp::Perspective::typedInstance<GraphPerspective>()->createPanel(g);
}

void GraphHierarchiesEditor::exportGraph() {
  tlp::Perspective::typedInstance<GraphPerspective>()->exportGraph(_contextGraph);
}

void GraphHierarchiesEditor::renameGraph() {
  if (_contextIndex.isValid() && _ui->hierarchiesTree->selectionModel()->selectedRows(0).size() == 1) {
    // Qt bug workaround
    // there is a potential pb when calling
    // _ui->hierarchiesTree->model()->index(_contextIndex.row(), 0,
    // _contextIndex.parent())
    // to get the index of the name to edit
    // because the call to _contextIndex.parent() may cause a crash
    // same pb if using _contextIndex.sibling(__contextIndex.row(), 0) instead
    // It seems safer to use an index build from the selectionModel()
    _ui->hierarchiesTree->edit(_ui->hierarchiesTree->selectionModel()->selectedRows(0)[0]);
  }
}

void GraphHierarchiesEditor::saveGraphHierarchyInTlpFile() {
  tlp::Perspective::typedInstance<GraphPerspective>()->saveGraphHierarchyInTlpFile(_contextGraph);
}

void GraphHierarchiesEditor::toggleSynchronization(bool f) {
  if (f) {
    _linkButton->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::linkvariant, Qt::white, 0.8, QPointF(-1, -1)));
    _linkButton->setToolTip("Click here to disable the synchronization with "
                            "workspace active panel.\nWhen synchronization is "
                            "enabled, the graph currently displayed\nin the "
                            "active panel, becomes the current one in the "
                            "Graphs panel.");
  } else {
    _linkButton->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::linkvariantoff, Qt::white, 0.8));
    _linkButton->setToolTip("Click here to enable the synchronization with "
                            "workspace active panel.\nWhen synchronization is "
                            "enabled, the graph currently displayed\nin the "
                            "active panel, becomes the current one in the "
                            "Graphs panel.");
  }

  emit changeSynchronization(f);
}

void GraphHierarchiesEditor::setSynchronizeButtonVisible(bool f) {
  _linkButton->setVisible(f);
}
