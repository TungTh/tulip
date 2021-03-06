/**
 *
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

#ifdef BUILD_PYTHON_COMPONENTS
#include "PythonPanel.h"
#include "PythonPluginsIDE.h"
#include <tulip/APIDataBase.h>
#include <tulip/PythonInterpreter.h>
#endif

#include "GraphPerspective.h"

#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDropEvent>
#include <QFileDialog>
#include <QCloseEvent>
#include <QClipboard>
#include <QDropEvent>
#include <QUrl>
#include <QMimeData>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QMessageBox>

#include <tulip/AboutTulipPage.h>
#include <tulip/CSVImportWizard.h>
#include <tulip/ColorScaleConfigDialog.h>
#include <tulip/ColorScaleConfigDialog.h>
#include <tulip/ExportModule.h>
#include <tulip/FontIconManager.h>
#include <tulip/GlGraph.h>
#include <tulip/GlMainView.h>
#include <tulip/GlMainWidget.h>
#include <tulip/Graph.h>
#include <tulip/GraphHierarchiesModel.h>
#include <tulip/GraphModel.h>
#include <tulip/GraphPropertiesModel.h>
#include <tulip/GraphTableItemDelegate.h>
#include <tulip/ImportModule.h>
#include <tulip/PluginLister.h>
#include <tulip/SimplePluginProgressWidget.h>
#include <tulip/TlpQtTools.h>
#include <tulip/TlpTools.h>
#include <tulip/TulipProject.h>
#include <tulip/GraphTools.h>
#include <tulip/ColorScaleConfigDialog.h>
#include <tulip/AboutTulipPage.h>
#include <tulip/ColorScalesManager.h>
#include <tulip/TulipSettings.h>

#include "ui_GraphPerspectiveMainWindow.h"

#include "ExportWizard.h"
#include "GraphHierarchiesEditor.h"
#include "GraphPerspectiveLogger.h"
#include "ImportWizard.h"
#include "PanelSelectionWizard.h"
#include "PreferencesDialog.h"

#include <QDebug>

using namespace tlp;
using namespace std;

static QColor sideBarIconColor = Qt::white;
static QColor menuIconColor = QColor("#404244");

static QIcon getFileNewIcon() {
  QIcon backIcon = FontIconManager::instance()->getMaterialDesignIcon(md::file, menuIconColor);
  QIcon frontIcon = FontIconManager::instance()->getMaterialDesignIcon(md::plus, Qt::white, 0.7, QPointF(0, 10));
  return FontIconManager::stackIcons(backIcon, frontIcon);
}

GraphPerspective::GraphPerspective(const tlp::PluginContext *c)
    : Perspective(c), _ui(nullptr), _graphs(new GraphHierarchiesModel(this)), _recentDocumentsSettingsKey("perspective/recent_files"),
      _logger(nullptr) {
  Q_INIT_RESOURCE(GraphPerspective);

  if (c && ((PerspectiveContext *)c)->parameters.contains("gui_testing")) {
    tlp::setGuiTestingMode(true);
    // we must ensure that choosing a file is relative to
    // the current directory to allow to run the gui tests
    // from any relative unit_test/gui directory
    _lastOpenLocation = QDir::currentPath();
  }
}

void GraphPerspective::reserveDefaultProperties() {
  registerReservedProperty("viewColor");
  registerReservedProperty("viewLabelColor");
  registerReservedProperty("viewLabelBorderColor");
  registerReservedProperty("viewLabelBorderWidth");
  registerReservedProperty("viewSize");
  registerReservedProperty("viewLabel");
  registerReservedProperty("viewLabelPosition");
  registerReservedProperty("viewShape");
  registerReservedProperty("viewRotation");
  registerReservedProperty("viewSelection");
  registerReservedProperty("viewFont");
  registerReservedProperty("viewFontAwesomeIcon");
  registerReservedProperty("viewFontSize");
  registerReservedProperty("viewTexture");
  registerReservedProperty("viewBorderColor");
  registerReservedProperty("viewBorderWidth");
  registerReservedProperty("viewLayout");
  registerReservedProperty("viewSrcAnchorShape");
  registerReservedProperty("viewSrcAnchorSize");
  registerReservedProperty("viewTgtAnchorShape");
  registerReservedProperty("viewTgtAnchorSize");
  registerReservedProperty("viewAnimationFrame");
}

void GraphPerspective::buildRecentDocumentsMenu() {
  _ui->menuOpen_recent_file->clear();

  foreach (const QString &s, TulipSettings::instance().recentDocuments()) {
    if (!QFileInfo(s).exists())
      continue;
    _ui->menuOpen_recent_file->addAction(FontIconManager::instance()->getMaterialDesignIcon(md::archive, menuIconColor), s, this,
                                         SLOT(openRecentFile()));
  }
  _ui->menuOpen_recent_file->addSeparator();

  foreach (const QString &s, TulipSettings::instance().value(_recentDocumentsSettingsKey).toStringList()) {
    if (!QFileInfo(s).exists())
      continue;
    _ui->menuOpen_recent_file->addAction(FontIconManager::instance()->getMaterialDesignIcon(md::file, menuIconColor), s, this,
                                         SLOT(openRecentFile()));
  }
}

void GraphPerspective::addRecentDocument(const QString &path) {
  QStringList recents = TulipSettings::instance().value(_recentDocumentsSettingsKey).toStringList();

  if (recents.contains(path))
    return;

  recents += path;

  if (recents.size() > 10)
    recents.pop_front();

  TulipSettings::instance().setValue(_recentDocumentsSettingsKey, recents);
  TulipSettings::instance().sync();
  buildRecentDocumentsMenu();
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))

void graphPerspectiveLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  if (msg.startsWith("[Python")) {
    // remove quotes around message added by Qt
    QString msgClean = msg.mid(14).mid(2, msg.length() - 17);

    if (msg.startsWith("[PythonStdOut]")) {
      std::cout << QStringToTlpString(msgClean) << std::endl;
    } else {
      std::cerr << QStringToTlpString(msgClean) << std::endl;
    }
  } else {
    std::cerr << QStringToTlpString(msg) << std::endl;
  }

  static_cast<GraphPerspective *>(Perspective::instance())->log(type, context, msg);
}

void GraphPerspective::log(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  _logger->log(type, context, msg);
  _ui->loggerIcon->setPixmap(_logger->icon());
  _ui->loggerMessage->setText(QString::number(_logger->count()));
}

#else

void graphPerspectiveLogger(QtMsgType type, const char *msg) {
  QString qmsg = msg;

  if (qmsg.startsWith("[Python")) {
    // remove quotes around message added by Qt
    QString msgClean = qmsg.mid(14).mid(2, qmsg.length() - 18);

    if (qmsg.startsWith("[PythonStdOut]")) {
      std::cout << QStringToTlpString(msgClean) << std::endl;
    } else {
      std::cerr << QStringToTlpString(msgClean) << std::endl;
    }
  } else {
    std::cerr << QStringToTlpString(qmsg) << std::endl;
  }

  static_cast<GraphPerspective *>(Perspective::instance())->log(type, msg);
}

void GraphPerspective::log(QtMsgType type, const char *msg) {
  _logger->log(type, msg);
  _ui->loggerIcon->setPixmap(_logger->icon());
  _ui->loggerMessage->setText(QString::number(_logger->count()));
}

#endif

GraphPerspective::~GraphPerspective() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  qInstallMessageHandler(0);
#else
  qInstallMsgHandler(0);
#endif

  delete _ui;
}

void GraphPerspective::logCleared() {
  _ui->loggerMessage->setText("");
  _ui->loggerIcon->setPixmap(QPixmap());
}

void GraphPerspective::findPlugins() {
  _ui->algorithmRunner->findPlugins();
}

bool GraphPerspective::eventFilter(QObject *obj, QEvent *ev) {
  if (ev->type() == QEvent::DragEnter) {
    QDragEnterEvent *dragEvent = dynamic_cast<QDragEnterEvent *>(ev);

    if (dragEvent->mimeData()->hasUrls()) {
      dragEvent->accept();
    }
  }

  if (ev->type() == QEvent::Drop) {
    QDropEvent *dropEvent = dynamic_cast<QDropEvent *>(ev);
    foreach (const QUrl &url, dropEvent->mimeData()->urls()) { open(url.toLocalFile()); }
  }

  if (obj == _ui->loggerFrame && ev->type() == QEvent::MouseButtonPress)
    showLogger();

  if (obj == _mainWindow && ev->type() == QEvent::Close) {
    if (_graphs->needsSaving()) {
      QMessageBox::StandardButton answer = QMessageBox::question(_mainWindow, trUtf8("Save"), trUtf8("The project has been modified. Do you "
                                                                                                     "want to save your changes?"),
                                                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel | QMessageBox::Escape);

      if ((answer == QMessageBox::Yes && !save()) || (answer == QMessageBox::Cancel)) {
        ev->ignore();
        return true;
      }
    }
  }

  return false;
}

void GraphPerspective::showLogger() {
  if (_logger->count() == 0)
    return;

  QPoint pos = _mainWindow->mapToGlobal(_ui->loggerFrame->pos());
  pos.setX(pos.x() + _ui->loggerFrame->width());
  pos.setY(std::min<int>(_mainWindow->mapToGlobal(QPoint(0, 0)).y() + mainWindow()->height() - _logger->height(), pos.y()));
  _logger->move(pos);
  // extend the logger frame width until reaching the right side of the main
  // window
  _logger->resize(_mainWindow->mapToGlobal(_mainWindow->pos()).x() + mainWindow()->width() - _mainWindow->mapToGlobal(_logger->pos()).x(),
                  _logger->height());

  _logger->show();
}

void GraphPerspective::redrawPanels(bool center) {
  _ui->workspace->redrawPanels(center);
}

void GraphPerspective::start(tlp::PluginProgress *progress) {
  reserveDefaultProperties();
  _ui = new Ui::GraphPerspectiveMainWindowData;
  _ui->setupUi(_mainWindow);

  _ui->developButton->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::languagepython, sideBarIconColor));
  _ui->workspaceButton->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::televisionguide, sideBarIconColor));
  _ui->importButton->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::import, sideBarIconColor));
  _ui->exportButton->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::export_, sideBarIconColor));
  _ui->csvImportButton->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::tablelarge, sideBarIconColor));
  _ui->undoButton->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::reply, sideBarIconColor));
  _ui->redoButton->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::share, sideBarIconColor));
  _ui->addPanelButton->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::plusbox, sideBarIconColor));
  _ui->actionNewProject->setIcon(getFileNewIcon());
  _ui->actionOpen_Project->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::fileimport, menuIconColor));
  _ui->menuOpen_recent_file->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::clock, menuIconColor));
  _ui->actionSave_Project->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::fileexport, menuIconColor));
  _ui->actionSave_Project_as->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::fileexport, menuIconColor));
  _ui->actionImport->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::import, menuIconColor));
  _ui->actionExport->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::export_, menuIconColor));
  _ui->actionImport_CSV->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::table, menuIconColor));
  _ui->actionNew_graph->setIcon(getFileNewIcon());
  _ui->actionExit->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::closecircle, menuIconColor));
  _ui->actionUndo->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::reply, menuIconColor));
  _ui->actionRedo->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::share, menuIconColor));
  _ui->actionCut->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::contentcut, menuIconColor));
  _ui->actionPaste->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::contentpaste, menuIconColor));
  _ui->actionCopy->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::contentcopy, menuIconColor));
  _ui->actionDelete->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::delete_, menuIconColor));
  _ui->actionSelect_All->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::selectall, menuIconColor));
  _ui->actionInvert_selection->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::selectinverse, menuIconColor));
  _ui->actionCancel_selection->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::selectoff, menuIconColor));
  _ui->actionGroup_elements->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::group, menuIconColor));
  _ui->actionPreferences->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::settings, menuIconColor));
  _ui->actionFull_screen->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::fullscreen, menuIconColor));
  _ui->action_Close_All->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::windowclose, menuIconColor));
  _ui->actionMessages_log->setIcon(tlp::FontIconManager::instance()->getMaterialDesignIcon(tlp::md::information, QColor("#407FB2")));
  _ui->actionFind_plugins->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::magnify, menuIconColor));
  _ui->actionAbout_us->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::star, QColor("#F9AA3A")));
  _ui->actionShowUserDocumentation->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::bookopenvariant, menuIconColor));
  _ui->actionShowDevelDocumentation->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::bookopenvariant, menuIconColor));
  _ui->actionShowPythonDocumentation->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::bookopenvariant, menuIconColor));
  _ui->actionColor_scales_management->setIcon(FontIconManager::instance()->getMaterialDesignIcon(md::palette, menuIconColor));
#ifdef BUILD_PYTHON_COMPONENTS
  _pythonPanel = new PythonPanel();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(_pythonPanel);
  layout->setContentsMargins(0, 0, 0, 0);
  _ui->pythonPanel->setLayout(layout);
  _developFrame = new PythonPluginsIDE();
  layout = new QVBoxLayout();
  layout->addWidget(_developFrame);
  layout->setContentsMargins(0, 0, 0, 0);
  _ui->developFrame->setLayout(layout);
#else
  _ui->pythonButton->setVisible(false);
  _ui->developButton->setVisible(false);
#endif
  currentGraphChanged(nullptr);
  _ui->singleModeButton->setEnabled(false);
  _ui->singleModeButton->hide();
  _ui->workspace->setSingleModeSwitch(_ui->singleModeButton);
  _ui->workspace->setFocusedPanelHighlighting(true);
  _ui->splitModeButton->setEnabled(false);
  _ui->splitModeButton->hide();
  _ui->workspace->setSplitModeSwitch(_ui->splitModeButton);
  _ui->splitHorizontalModeButton->setEnabled(false);
  _ui->splitHorizontalModeButton->hide();
  _ui->workspace->setSplitHorizontalModeSwitch(_ui->splitHorizontalModeButton);
  _ui->split3ModeButton->setEnabled(false);
  _ui->split3ModeButton->hide();
  _ui->workspace->setSplit3ModeSwitch(_ui->split3ModeButton);
  _ui->split32ModeButton->setEnabled(false);
  _ui->split32ModeButton->hide();
  _ui->workspace->setSplit32ModeSwitch(_ui->split32ModeButton);
  _ui->split33ModeButton->setEnabled(false);
  _ui->split33ModeButton->hide();
  _ui->workspace->setSplit33ModeSwitch(_ui->split33ModeButton);
  _ui->gridModeButton->setEnabled(false);
  _ui->gridModeButton->hide();
  _ui->workspace->setGridModeSwitch(_ui->gridModeButton);
  _ui->sixModeButton->setEnabled(false);
  _ui->sixModeButton->hide();
  _ui->workspace->setSixModeSwitch(_ui->sixModeButton);
  _ui->workspace->setPageCountLabel(_ui->pageCountLabel);
  _ui->workspace->setExposeModeSwitch(_ui->exposeModeButton);
  _ui->outputFrame->hide();
  _logger = new GraphPerspectiveLogger(_mainWindow);
  _ui->loggerFrame->installEventFilter(this);
  _mainWindow->installEventFilter(this);
  _mainWindow->setAcceptDrops(true);
  connect(_logger, SIGNAL(cleared()), this, SLOT(logCleared()));

  _colorScalesDialog = new ColorScaleConfigDialog(ColorScalesManager::getLatestColorScale(), mainWindow());

  // redirection of various output
  redirectDebugOutputToQDebug();
  redirectWarningOutputToQWarning();
  redirectErrorOutputToQCritical();

  TulipSettings::instance().synchronizeViewSettings();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  qInstallMessageHandler(graphPerspectiveLogger);
#else
  qInstallMsgHandler(graphPerspectiveLogger);
#endif

  connect(_ui->workspace, SIGNAL(addPanelRequest(tlp::Graph *)), this, SLOT(createPanel(tlp::Graph *)));
  connect(_ui->workspace, SIGNAL(focusedPanelSynchronized()), this, SLOT(focusedPanelSynchronized()));
  connect(_graphs, SIGNAL(currentGraphChanged(tlp::Graph *)), this, SLOT(currentGraphChanged(tlp::Graph *)));
  connect(_graphs, SIGNAL(currentGraphChanged(tlp::Graph *)), _ui->algorithmRunner, SLOT(setGraph(tlp::Graph *)));
  connect(_ui->graphHierarchiesEditor, SIGNAL(changeSynchronization(bool)), this, SLOT(changeSynchronization(bool)));

  // Connect actions
  connect(_ui->actionMessages_log, SIGNAL(triggered()), this, SLOT(showLogger()));
  connect(_ui->actionFull_screen, SIGNAL(triggered(bool)), this, SLOT(showFullScreen(bool)));
  connect(_ui->actionImport, SIGNAL(triggered()), this, SLOT(importGraph()));
  connect(_ui->actionExport, SIGNAL(triggered()), this, SLOT(exportGraph()));
  connect(_ui->actionSave_graph_to_file, SIGNAL(triggered()), this, SLOT(saveGraphHierarchyInTlpFile()));
  connect(_ui->workspace, SIGNAL(panelFocused(tlp::View *)), this, SLOT(panelFocused(tlp::View *)));
  connect(_ui->actionSave_Project, SIGNAL(triggered()), this, SLOT(save()));
  connect(_ui->actionSave_Project_as, SIGNAL(triggered()), this, SLOT(saveAs()));
  connect(_ui->actionOpen_Project, SIGNAL(triggered()), this, SLOT(open()));
  connect(_ui->actionDelete, SIGNAL(triggered()), this, SLOT(deleteSelectedElements()));
  connect(_ui->actionInvert_selection, SIGNAL(triggered()), this, SLOT(invertSelection()));
  connect(_ui->actionCancel_selection, SIGNAL(triggered()), this, SLOT(cancelSelection()));
  connect(_ui->actionMake_selection_a_graph, SIGNAL(triggered()), this, SLOT(make_graph()));
  connect(_ui->actionSelect_All, SIGNAL(triggered()), this, SLOT(selectAll()));
  connect(_ui->actionUndo, SIGNAL(triggered()), this, SLOT(undo()));
  connect(_ui->actionRedo, SIGNAL(triggered()), this, SLOT(redo()));
  connect(_ui->actionCut, SIGNAL(triggered()), this, SLOT(cut()));
  connect(_ui->actionPaste, SIGNAL(triggered()), this, SLOT(paste()));
  connect(_ui->actionCopy, SIGNAL(triggered()), this, SLOT(copy()));
  connect(_ui->actionGroup_elements, SIGNAL(triggered()), this, SLOT(group()));
  connect(_ui->actionCreate_sub_graph, SIGNAL(triggered()), this, SLOT(createSubGraph()));
  connect(_ui->actionClone_sub_graph, SIGNAL(triggered()), this, SLOT(cloneSubGraph()));
  connect(_ui->actionCreate_empty_sub_graph, SIGNAL(triggered()), this, SLOT(addEmptySubGraph()));
  connect(_ui->actionImport_CSV, SIGNAL(triggered()), this, SLOT(CSVImport()));
  connect(_ui->actionFind_plugins, SIGNAL(triggered()), this, SLOT(findPlugins()));
  connect(_ui->actionNew_graph, SIGNAL(triggered()), this, SLOT(addNewGraph()));
  connect(_ui->actionNewProject, SIGNAL(triggered()), this, SLOT(newProject()));
  connect(_ui->actionPreferences, SIGNAL(triggered()), this, SLOT(openPreferences()));
  connect(_ui->searchButton, SIGNAL(clicked(bool)), this, SLOT(setSearchOutput(bool)));
  connect(_ui->workspace, SIGNAL(importGraphRequest()), this, SLOT(importGraph()));
  connect(_ui->workspaceButton, SIGNAL(clicked()), this, SLOT(setWorkspaceMode()));
  connect(_ui->action_Close_All, SIGNAL(triggered()), _ui->workspace, SLOT(closeAll()));
  connect(_ui->addPanelButton, SIGNAL(clicked()), this, SLOT(createPanel()));
  connect(_ui->actionColor_scales_management, SIGNAL(triggered()), this, SLOT(displayColorScalesDialog()));

  // Agent actions
  connect(_ui->actionPlugins_Center, SIGNAL(triggered()), this, SLOT(showPluginsCenter()));
  connect(_ui->actionAbout_us, SIGNAL(triggered()), this, SLOT(showAboutPage()));
  connect(_ui->actionAbout_us, SIGNAL(triggered()), this, SLOT(showAboutTulipPage()));

  if (QFile(tlpStringToQString(tlp::TulipShareDir) + "doc/tulip-user/html/index.html").exists()) {
    connect(_ui->actionShowUserDocumentation, SIGNAL(triggered()), this, SLOT(showUserDocumentation()));
    connect(_ui->actionShowDevelDocumentation, SIGNAL(triggered()), this, SLOT(showDevelDocumentation()));
    connect(_ui->actionShowPythonDocumentation, SIGNAL(triggered()), this, SLOT(showPythonDocumentation()));
  } else {
    _ui->actionShowUserDocumentation->setVisible(false);
    _ui->actionShowDevelDocumentation->setVisible(false);
    _ui->actionShowPythonDocumentation->setVisible(false);
  }

  // Setting initial sizes for splitters
  _ui->mainSplitter->setSizes(QList<int>() << 350 << 850);
  _ui->mainSplitter->setStretchFactor(0, 0);
  _ui->mainSplitter->setStretchFactor(1, 1);
  _ui->mainSplitter->setCollapsible(1, false);

  // Open project with model
  QMap<QString, tlp::Graph *> rootIds;

  if (!_project->projectFile().isEmpty()) {
    rootIds = _graphs->readProject(_project, progress);

    if (rootIds.empty())
      QMessageBox::critical(_mainWindow, QString("Error while loading project ").append(_project->projectFile()), progress->getError().c_str());
  }

  // these ui initializations are needed here
  // in case of a call to showStartPanels in the open method
  _ui->graphHierarchiesEditor->setModel(_graphs);
  _ui->workspace->setModel(_graphs);

  if (!rootIds.empty())
    _ui->workspace->readProject(_project, rootIds, progress);

  _ui->searchPanel->setModel(_graphs);

#ifdef BUILD_PYTHON_COMPONENTS
  connect(_ui->pythonButton, SIGNAL(clicked(bool)), this, SLOT(setPythonPanel(bool)));
  connect(_ui->developButton, SIGNAL(clicked()), this, SLOT(setDevelopMode()));
  _pythonPanel->setModel(_graphs);
  tlp::PluginLister::instance()->addListener(this);
  _developFrame->setProject(_project);

  APIDataBase::getInstance()->loadApiFile(tlpStringToQString(tlp::TulipShareDir) + "/apiFiles/tulip.api");
  APIDataBase::getInstance()->loadApiFile(tlpStringToQString(tlp::TulipShareDir) + "/apiFiles/Python-" +
                                          PythonInterpreter::getInstance()->getPythonVersionStr() + ".api");
  APIDataBase::getInstance()->loadApiFile(tlpStringToQString(tlp::TulipShareDir) + "/apiFiles/tulipogl.api");
  APIDataBase::getInstance()->loadApiFile(tlpStringToQString(tlp::TulipShareDir) + "/apiFiles/tulipgui.api");

  PythonInterpreter::getInstance()->setErrorOutputEnabled(false);

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))

  if (PythonInterpreter::getInstance()->runString("import PyQt4")) {
    APIDataBase::getInstance()->loadApiFile(tlpStringToQString(tlp::TulipShareDir) + "/apiFiles/PyQt4.api");
  }

#endif

  PythonInterpreter::getInstance()->setErrorOutputEnabled(true);
#endif

  if (!_externalFile.isEmpty() && QFileInfo(_externalFile).exists()) {
    open(_externalFile);
  }

  foreach (HeaderFrame *h, _ui->docksSplitter->findChildren<HeaderFrame *>()) {
    connect(h, SIGNAL(expanded(bool)), this, SLOT(refreshDockExpandControls()));
  }

  connect(_ui->sidebarButton, SIGNAL(clicked()), this, SLOT(showHideSideBar()));

#if !defined(__APPLE__) && !defined(_WIN32)
  // Hide plugins center when not on MacOS or Windows
  _ui->pluginsButton->hide();
  _ui->menuHelp->removeAction(_ui->actionPlugins_Center);
#else
  // show the 'Plugins center' menu entry and button only if connected to the Tulip agent
  _ui->pluginsButton->setVisible(checkSocketConnected());
  _ui->actionPlugins_Center->setVisible(checkSocketConnected());
#endif

  // show the 'Find plugins' menu entry only if connected to the Tulip agent
  _ui->actionFind_plugins->setVisible(checkSocketConnected());

  // fill menu with recent documents
  buildRecentDocumentsMenu();

  showTrayMessage("GraphPerspective started");
}

void GraphPerspective::openExternalFile() {
  open(_externalFile);
}

tlp::GraphHierarchiesModel *GraphPerspective::model() const {
  return _graphs;
}

void GraphPerspective::refreshDockExpandControls() {
  QList<HeaderFrame *> expandedHeaders, collapsedHeaders;
  foreach (HeaderFrame *h, _ui->docksSplitter->findChildren<HeaderFrame *>()) {
    h->expandControl()->setEnabled(true);

    if (h->isExpanded())
      expandedHeaders.push_back(h);
    else
      collapsedHeaders.push_back(h);
  }

  if (expandedHeaders.size() == 1)
    expandedHeaders[0]->expandControl()->setEnabled(false);
}

void GraphPerspective::exportGraph(Graph *g) {
  if (g == nullptr)
    g = _graphs->currentGraph();

  if (g == nullptr)
    return;

  static QString exportFile;
  ExportWizard wizard(g, exportFile, _mainWindow);
  wizard.setWindowTitle(QString("Export of graph \"") + g->getName().c_str() + '"');

  if (wizard.exec() != QDialog::Accepted || wizard.algorithm().isNull() || wizard.outputFile().isEmpty())
    return;

  std::ostream *os;
  std::string filename = QStringToTlpString(exportFile = wizard.outputFile());
  std::string exportPluginName = QStringToTlpString(wizard.algorithm());

  if (filename.rfind(".gz") == (filename.length() - 3)) {
    if (exportPluginName != "TLP Export" && exportPluginName != "TLPB Export") {
      QMessageBox::critical(_mainWindow, trUtf8("Format error"), trUtf8("GZip compression is only supported for TLP and TLPB formats."));
      return;
    }

    os = tlp::getOgzstream(filename);
  } else {
    if (exportPluginName == "TLPB Export")
      os = tlp::getOutputFileStream(filename, std::ios::out | std::ios::binary);
    else
      os = tlp::getOutputFileStream(filename);
  }

  if (os->fail()) {
    QMessageBox::critical(_mainWindow, trUtf8("File error"), trUtf8("Cannot open output file for writing: ") + wizard.outputFile());
    delete os;
    return;
  }

  DataSet data = wizard.parameters();
  PluginProgress *prg = progress(NoProgressOption);
  prg->setTitle(exportPluginName);
  // take time before run
  QDateTime start = QDateTime::currentDateTime();
  bool result = tlp::exportGraph(g, *os, exportPluginName, data, prg);
  delete os;

  if (!result) {
    QMessageBox::critical(_mainWindow, trUtf8("Export error"), QString("<i>") + wizard.algorithm() +
                                                                   trUtf8("</i> failed to export graph.<br/><br/><b>") +
                                                                   tlp::tlpStringToQString(prg->getError()) + "</b>");
  } else {
    // display spent time
    if (TulipSettings::instance().isRunningTimeComputed()) {
      std::string moduleAndParams = exportPluginName + " - " + data.toString();
      qDebug() << tlp::tlpStringToQString(moduleAndParams) << ": " << start.msecsTo(QDateTime::currentDateTime()) << "ms";
    }
    addRecentDocument(wizard.outputFile());
  }
  delete prg;
}

void GraphPerspective::saveGraphHierarchyInTlpFile(Graph *g) {
  if (g == nullptr)
    g = _graphs->currentGraph();

  if (g == nullptr)
    return;

  static QString savedFile;
  QString filter("TLP format (*.tlp *.tlp.gz);;TLPB format (*.tlpb *.tlpb.gz)");
  QString filename = QFileDialog::getSaveFileName(_mainWindow, tr("Save graph hierarchy in tlp/tlpb file"), savedFile, filter);

  if (!filename.isEmpty()) {
    bool result = tlp::saveGraph(g, tlp::QStringToTlpString(filename));
    if (!result)
      QMessageBox::critical(_mainWindow, trUtf8("Save error"), trUtf8("Failed to save graph hierarchy"));
    else {
      savedFile = filename;
      addRecentDocument(filename);
    }
  }
}

void GraphPerspective::importGraph(const std::string &module, DataSet &data) {
  Graph *g;

  if (!module.empty()) {
    PluginProgress *prg = progress(IsStoppable | IsCancellable);
    prg->setTitle(module);
    // take time before run
    QDateTime start = QDateTime::currentDateTime();
    g = tlp::importGraph(module, data, prg);

    if (g == nullptr) {
      QMessageBox::critical(_mainWindow, trUtf8("Import error"), QString("<i>") + tlp::tlpStringToQString(module) +
                                                                     trUtf8("</i> failed to import data.<br/><br/><b>") +
                                                                     tlp::tlpStringToQString(prg->getError()) + "</b>");
      delete prg;
      return;
    }

    delete prg;

    // display spent time
    if (TulipSettings::instance().isRunningTimeComputed()) {
      std::string moduleAndParams = module + " import - " + data.toString();
      qDebug() << tlp::tlpStringToQString(moduleAndParams) << ": " << start.msecsTo(QDateTime::currentDateTime()) << "ms";
    }

    if (g->getName().empty()) {
      QString n = tlp::tlpStringToQString(module) + " - " + tlp::tlpStringToQString(data.toString());
      n.replace(QRegExp("[\\w]*::"), ""); // remove words before "::"
      g->setName(tlp::QStringToTlpString(n));
    }
  } else {
    g = tlp::newGraph();
  }

  _graphs->addGraph(g);
  std::string fileName;

  if (data.get("file::filename", fileName))
    // set current directory to the directory of the loaded file
    // to ensure a correct loading of the associated texture files if any
    QDir::setCurrent(QFileInfo(tlpStringToQString(fileName)).absolutePath());

  applyRandomLayout(g);
  showStartPanels(g);
}

void GraphPerspective::importGraph() {
  ImportWizard wizard(_mainWindow);

  if (wizard.exec() == QDialog::Accepted) {
    DataSet data = wizard.parameters();
    importGraph(tlp::QStringToTlpString(wizard.algorithm()), data);
  }
}

void GraphPerspective::createPanel(tlp::Graph *g) {
  if (_graphs->empty())
    return;

  PanelSelectionWizard wizard(_graphs, _mainWindow);

  if (g != nullptr)
    wizard.setSelectedGraph(g);
  else
    wizard.setSelectedGraph(_graphs->currentGraph());

  int result = wizard.exec();

  if (result == QDialog::Accepted && wizard.panel() != nullptr) {
    // expose mode is not safe to add a new panel
    // so hide it if needed
    _ui->workspace->hideExposeMode();
    _ui->workspace->addPanel(wizard.panel());
    _ui->workspace->setActivePanel(wizard.panel());
    wizard.panel()->applySettings();
  }
}

void GraphPerspective::panelFocused(tlp::View *view) {
  disconnect(this, SLOT(focusedPanelGraphSet(tlp::Graph *)));

  if (!_ui->graphHierarchiesEditor->synchronized())
    return;

  connect(view, SIGNAL(graphSet(tlp::Graph *)), this, SLOT(focusedPanelGraphSet(tlp::Graph *)));
  focusedPanelGraphSet(view->graph());
}

void GraphPerspective::changeSynchronization(bool s) {
  _ui->workspace->setFocusedPanelHighlighting(s);
}

void GraphPerspective::focusedPanelGraphSet(Graph *g) {
  _graphs->setCurrentGraph(g);
}

void GraphPerspective::focusedPanelSynchronized() {
  _ui->workspace->setGraphForFocusedPanel(_graphs->currentGraph());
}

bool GraphPerspective::save() {
  return saveAs(_project->projectFile());
}

bool GraphPerspective::saveAs(const QString &path) {
  if (path.isEmpty()) {
    QString path = QFileDialog::getSaveFileName(_mainWindow, trUtf8("Save project"), QString(), "Tulip Project (*.tlpx)");

    if (!path.isEmpty()) {
      if (!path.endsWith(".tlpx"))
        path += ".tlpx";

      return saveAs(path);
    }

    return false;
  }

  SimplePluginProgressDialog progress(_mainWindow);
  progress.showPreview(false);
  progress.show();
  QMap<Graph *, QString> rootIds = _graphs->writeProject(_project, &progress);
  _ui->workspace->writeProject(_project, rootIds, &progress);
  _project->write(path, &progress);
  TulipSettings::instance().addToRecentDocuments(path);

  return true;
}

void GraphPerspective::open(QString fileName) {
  QMap<std::string, std::string> modules;
  std::list<std::string> imports = PluginLister::instance()->availablePlugins<ImportModule>();

  std::string filters("Tulip project (*.tlpx);;");
  std::string filterAny("Any supported format (");

  for (std::list<std::string>::const_iterator it = imports.begin(); it != imports.end(); ++it) {
    ImportModule *m = PluginLister::instance()->getPluginObject<ImportModule>(*it, nullptr);
    std::list<std::string> fileExtension(m->fileExtensions());

    std::string currentFilter;

    for (std::list<std::string>::const_iterator listIt = fileExtension.begin(); listIt != fileExtension.end(); ++listIt) {
      if (listIt->empty())
        continue;

      filterAny += "*." + *listIt + " ";
      currentFilter += "*." + *listIt + " ";

      modules[*listIt] = *it;
    }

    if (!currentFilter.empty())
      filters += *it + "(" + currentFilter + ");;";

    delete m;
  }

  filterAny += " *.tlpx);;";

  filters += "All files (*)";
  filters.insert(0, filterAny);

  if (fileName.isNull()) // If open() was called without a parameter, open the
    // file dialog
    fileName = QFileDialog::getOpenFileName(_mainWindow, tr("Open graph"), _lastOpenLocation, filters.c_str());

  if (!fileName.isEmpty()) {
    QFileInfo fileInfo(fileName);

    // we must ensure that choosing a file is relative to
    // the current directory to allow to run the gui tests
    // from any relative unit_test/gui directory
    if (!tlp::inGuiTestingMode())
      _lastOpenLocation = fileInfo.absolutePath();

    foreach (const std::string &extension, modules.keys()) {
      if (fileName.endsWith(".tlpx")) {
        openProjectFile(fileName);
        TulipSettings::instance().addToRecentDocuments(fileInfo.absoluteFilePath());
        break;
      } else if (fileName.endsWith(QString::fromStdString(extension))) {
        DataSet params;
        params.set("file::filename", QStringToTlpString(fileName));
        addRecentDocument(fileName);
        importGraph(modules[extension], params);
        break;
      }
    }
  }
}

void GraphPerspective::openProjectFile(const QString &path) {
  if (_graphs->empty()) {
    PluginProgress *prg = progress(NoProgressOption);
    _project->openProjectFile(path, prg);
    QMap<QString, tlp::Graph *> rootIds = _graphs->readProject(_project, prg);
    _ui->workspace->readProject(_project, rootIds, prg);
#ifdef BUILD_PYTHON_COMPONENTS
    _developFrame->setProject(_project);
#endif

    for (QMap<QString, tlp::Graph *>::iterator it = rootIds.begin(); it != rootIds.end(); ++it) {
      it.value()->setAttribute("file", QStringToTlpString(path));
    }

    delete prg;
  } else {
    Perspective::openProjectFile(path);
  }
}

void GraphPerspective::deleteSelectedElements() {
  Observable::holdObservers();
  tlp::Graph *graph = _graphs->currentGraph();
  tlp::BooleanProperty *selection = graph->getProperty<BooleanProperty>("viewSelection");

  graph->push();
  tlp::Iterator<edge> *itEdges = selection->getEdgesEqualTo(true, graph);
  graph->delEdges(itEdges, false);
  delete itEdges;

  tlp::Iterator<node> *itNodes = selection->getNodesEqualTo(true, graph);
  graph->delNodes(itNodes, false);
  delete itNodes;

  Observable::unholdObservers();
}

void GraphPerspective::invertSelection() {
  Observable::holdObservers();
  tlp::Graph *graph = _graphs->currentGraph();
  tlp::BooleanProperty *selection = graph->getProperty<BooleanProperty>("viewSelection");
  graph->push();
  selection->reverse();
  Observable::unholdObservers();
}

void GraphPerspective::cancelSelection() {
  Observable::holdObservers();
  tlp::Graph *graph = _graphs->currentGraph();
  tlp::BooleanProperty *selection = graph->getProperty<BooleanProperty>("viewSelection");
  graph->push();

  for (node n : selection->getNodesEqualTo(true)) {
    selection->setNodeValue(n, false);
  }

  for (edge e : selection->getEdgesEqualTo(true)) {
    selection->setEdgeValue(e, false);
  }
  Observable::unholdObservers();
}

void GraphPerspective::selectAll() {
  Observable::holdObservers();
  tlp::Graph *graph = _graphs->currentGraph();
  tlp::BooleanProperty *selection = graph->getProperty<BooleanProperty>("viewSelection");
  graph->push();

  for (node n : graph->getNodes()) {
    selection->setNodeValue(n, true);
  }

  for (edge e : graph->getEdges()) {
    selection->setEdgeValue(e, true);
  }

  Observable::unholdObservers();
}

void GraphPerspective::undo() {
  Observable::holdObservers();
  tlp::Graph *graph = _graphs->currentGraph();

  if (graph != nullptr)
    graph->pop();

  Observable::unholdObservers();

  foreach (View *v, _ui->workspace->panels()) {
    if (v->graph() == graph)
      v->undoCallback();
  }
}

void GraphPerspective::redo() {
  Observable::holdObservers();
  tlp::Graph *graph = _graphs->currentGraph();

  if (graph != nullptr)
    graph->unpop();

  Observable::unholdObservers();

  foreach (View *v, _ui->workspace->panels()) {
    if (v->graph() == graph)
      v->undoCallback();
  }
}

void GraphPerspective::cut() {
  copy(_graphs->currentGraph(), true);
}

void GraphPerspective::paste() {
  if (_graphs->currentGraph() == nullptr)
    return;

  Graph *outGraph = _graphs->currentGraph();
  std::stringstream ss;
  ss << QStringToTlpString(QApplication::clipboard()->text());

  Observable::holdObservers();
  outGraph->push();
  DataSet data;
  data.set("file::data", ss.str());
  Graph *inGraph = tlp::importGraph("TLP Import", data);
  tlp::copyToGraph(outGraph, inGraph);
  delete inGraph;
  Observable::unholdObservers();
  centerPanelsForGraph(outGraph);
}

void GraphPerspective::copy() {
  copy(_graphs->currentGraph());
}

void GraphPerspective::copy(Graph *g, bool deleteAfter) {
  if (g == nullptr)
    return;

  Observable::holdObservers();
  g->push();

  BooleanProperty *selection = g->getProperty<BooleanProperty>("viewSelection");

  Graph *copyGraph = tlp::newGraph();
  tlp::copyToGraph(copyGraph, g, selection);

  std::stringstream ss;
  DataSet data;
  tlp::exportGraph(copyGraph, ss, "TLP Export", data);
  QApplication::clipboard()->setText(tlpStringToQString(ss.str()));

  if (deleteAfter) {
    for (node n : stableIterator(selection->getNodesEqualTo(true)))
      g->delNode(n);
  }

  delete copyGraph;

  Observable::unholdObservers();
}

void GraphPerspective::group() {
  Observable::holdObservers();
  tlp::Graph *graph = _graphs->currentGraph();
  tlp::BooleanProperty *selection = graph->getProperty<BooleanProperty>("viewSelection");
  std::set<node> groupedNodes;

  for (node n : selection->getNodesEqualTo(true)) {
    if (graph->isElement(n))
      groupedNodes.insert(n);
  }

  if (groupedNodes.empty()) {
    Observable::unholdObservers();
    qCritical() << trUtf8("[Group] Cannot create meta-nodes from empty selection").toUtf8().data();
    return;
  }

  graph->push();

  bool changeGraph = false;

  if (graph == graph->getRoot()) {
    qWarning() << trUtf8("[Group] Grouping can not be done on the root graph. "
                         "A subgraph has automatically been created")
                      .toUtf8()
                      .data();
    graph = graph->addCloneSubGraph("groups");
    changeGraph = true;
  }

  graph->createMetaNode(groupedNodes, false);

  selection->setAllNodeValue(false);
  selection->setAllEdgeValue(false);

  Observable::unholdObservers();

  if (!changeGraph)
    return;

  foreach (View *v, _ui->workspace->panels()) {
    if (v->graph() == graph->getRoot())
      v->setGraph(graph);
  }
}

void GraphPerspective::make_graph() {
  Graph *graph = _graphs->currentGraph();
  unsigned added = makeSelectionGraph(_graphs->currentGraph(), graph->getProperty<BooleanProperty>("viewSelection"));
  tlp::debug() << "Make selection a graph: " << added << " elements added to the selection.";
}

Graph *GraphPerspective::createSubGraph(Graph *graph) {
  if (graph == nullptr)
    return nullptr;

  graph->push();
  Observable::holdObservers();
  BooleanProperty *selection = graph->getProperty<BooleanProperty>("viewSelection");
  makeSelectionGraph(graph, selection);
  Graph *result = graph->addSubGraph(selection, "selection sub-graph");
  Observable::unholdObservers();
  return result;
}

void GraphPerspective::createSubGraph() {
  createSubGraph(_graphs->currentGraph());
}

void GraphPerspective::cloneSubGraph() {
  if (_graphs->currentGraph() == nullptr)
    return;

  tlp::BooleanProperty prop(_graphs->currentGraph());
  prop.setAllNodeValue(true);
  prop.setAllEdgeValue(true);
  _graphs->currentGraph()->push();
  _graphs->currentGraph()->addSubGraph(&prop, "clone sub-graph");
}

void GraphPerspective::addEmptySubGraph() {
  if (_graphs->currentGraph() == nullptr)
    return;

  _graphs->currentGraph()->push();
  _graphs->currentGraph()->addSubGraph(nullptr, "empty sub-graph");
}

void GraphPerspective::currentGraphChanged(Graph *graph) {
  bool enabled(graph != nullptr);
  _ui->actionUndo->setEnabled(enabled);
  _ui->actionRedo->setEnabled(enabled);
  _ui->actionCut->setEnabled(enabled);
  _ui->actionCopy->setEnabled(enabled);
  _ui->actionPaste->setEnabled(enabled);
  _ui->actionDelete->setEnabled(enabled);
  _ui->actionInvert_selection->setEnabled(enabled);
  _ui->actionSelect_All->setEnabled(enabled);
  _ui->actionCancel_selection->setEnabled(enabled);
  _ui->actionMake_selection_a_graph->setEnabled(enabled);
  _ui->actionGroup_elements->setEnabled(enabled);
  _ui->actionCreate_sub_graph->setEnabled(enabled);
  _ui->actionCreate_empty_sub_graph->setEnabled(enabled);
  _ui->actionClone_sub_graph->setEnabled(enabled);
  _ui->actionExport->setEnabled(enabled);
  _ui->singleModeButton->setEnabled(enabled);
  _ui->splitModeButton->setEnabled(enabled);
  _ui->splitHorizontalModeButton->setEnabled(enabled);
  _ui->split3ModeButton->setEnabled(enabled);
  _ui->split32ModeButton->setEnabled(enabled);
  _ui->split33ModeButton->setEnabled(enabled);
  _ui->gridModeButton->setEnabled(enabled);
  _ui->sixModeButton->setEnabled(enabled);
  _ui->exposeModeButton->setEnabled(enabled);
  _ui->searchButton->setEnabled(enabled);
  _ui->pythonButton->setEnabled(enabled);
  _ui->previousPageButton->setVisible(enabled);
  _ui->pageCountLabel->setVisible(enabled);
  _ui->nextPageButton->setVisible(enabled);

  if (graph == nullptr) {
    _ui->workspace->switchToStartupMode();
    _ui->exposeModeButton->setChecked(false);
    _ui->searchButton->setChecked(false);
    _ui->pythonButton->setChecked(false);
    setSearchOutput(false);
  } else {
    _ui->workspace->setGraphForFocusedPanel(graph);
  }
}

void GraphPerspective::CSVImport() {
  bool mustDeleteGraph = false;

  if (_graphs->size() == 0) {
    _graphs->addGraph(tlp::newGraph());
    mustDeleteGraph = true;
  }

  Graph *g = _graphs->currentGraph();

  if (g == nullptr)
    return;

  CSVImportWizard wizard(_mainWindow);

  if (mustDeleteGraph) {
    wizard.setWindowTitle("Import CSV data into a new graph");
    wizard.setButtonText(QWizard::FinishButton, QString("Import into a new graph"));
  } else {
    wizard.setWindowTitle(QString("Import CSV data into current graph: ") + g->getName().c_str());
    wizard.setButtonText(QWizard::FinishButton, QString("Import into current graph"));
  }

  wizard.setGraph(g);
  g->push();
  Observable::holdObservers();
  int result = wizard.exec();

  if (result == QDialog::Rejected) {
    if (mustDeleteGraph) {
      _graphs->removeGraph(g);
      delete g;
    } else {
      g->pop();
    }
  } else {
    applyRandomLayout(g);
    bool openPanels = true;
    foreach (View *v, _ui->workspace->panels()) {
      if (v->graph() == g) {
        openPanels = false;
        break;
      }
    }

    if (openPanels)
      showStartPanels(g);
  }

  Observable::unholdObservers();
}

void GraphPerspective::showStartPanels(Graph *g) {
  if (TulipSettings::instance().displayDefaultViews() == false)
    return;

  // expose mode is not safe to add a new panel
  // so hide it if needed
  _ui->workspace->hideExposeMode();
  View *firstPanel = nullptr;
  View *secondPanel = nullptr;

  foreach (const QString &panelName, QStringList() << "Spreadsheet view"
                                                   << "Node Link Diagram view") {
    View *view = PluginLister::instance()->getPluginObject<View>(QStringToTlpString(panelName), nullptr);

    if (firstPanel == nullptr)
      firstPanel = view;
    else
      secondPanel = view;

    view->setupUi();
    view->setGraph(g);
    view->setState(DataSet());
    _ui->workspace->addPanel(view);
  }
  _ui->workspace->setActivePanel(firstPanel);
  _ui->workspace->switchToSplitMode();
  secondPanel->centerView(false);
}

void GraphPerspective::applyRandomLayout(Graph *g) {
  Observable::holdObservers();
  LayoutProperty *viewLayout = g->getProperty<LayoutProperty>("viewLayout");
  Iterator<node> *it = viewLayout->getNonDefaultValuatedNodes();

  if (!it->hasNext()) {
    std::string str;
    g->applyPropertyAlgorithm("Random layout", viewLayout, str);
  }

  delete it;

  Observable::unholdObservers();
}

void GraphPerspective::centerPanelsForGraph(tlp::Graph *g, bool graphChanged, bool onlyGlMainView) {
  foreach (View *v, _ui->workspace->panels()) {
    if ((v->graph() == g) && (!onlyGlMainView || dynamic_cast<tlp::GlMainView *>(v)))
      v->centerView(graphChanged);
  }
}

void GraphPerspective::closePanelsForGraph(tlp::Graph *g) {
  QVector<View *> viewsToDelete;
  foreach (View *v, _ui->workspace->panels()) {
    if (v->graph() == g || g->isDescendantGraph(v->graph()))
      viewsToDelete += v;
  }

  if (!viewsToDelete.empty()) {
    // expose mode is not safe to add a delete a panel
    // so hide it if needed
    _ui->workspace->hideExposeMode();
    foreach (View *v, viewsToDelete) { _ui->workspace->delView(v); }
  }
}

bool GraphPerspective::setGlMainViewPropertiesForGraph(tlp::Graph *g, const std::map<std::string, tlp::PropertyInterface *> &propsMap) {
  bool result = false;
  foreach (View *v, _ui->workspace->panels()) {
    GlMainView *glMainView = dynamic_cast<tlp::GlMainView *>(v);

    if (v->graph() == g && glMainView != nullptr) {
      if (glMainView->getGlMainWidget()->getScene()->getMainGlGraph()->getInputData().installProperties(propsMap))
        result = true;
    }
  }
  return result;
}

void GraphPerspective::setSearchOutput(bool f) {
  if (f) {
    _ui->outputFrame->setCurrentWidget(_ui->searchPanel);
    _ui->pythonButton->setChecked(false);
  }

  _ui->outputFrame->setVisible(f);
}

void GraphPerspective::setPythonPanel(bool f) {
  if (f) {
    _ui->outputFrame->setCurrentWidget(_ui->pythonPanel);
    _ui->searchButton->setChecked(false);
  }

  _ui->outputFrame->setVisible(f);
}

void GraphPerspective::openPreferences() {
  PreferencesDialog dlg(_ui->mainWidget);
  dlg.readSettings();

  if (dlg.exec() == QDialog::Accepted) {
    dlg.writeSettings();

    foreach (tlp::View *v, _ui->workspace->panels()) {
      GlMainView *glMainView = dynamic_cast<tlp::GlMainView *>(v);

      if (glMainView != nullptr) {
        if (glMainView->getGlMainWidget() != nullptr) {
          glMainView->getGlMainWidget()->getScene()->getMainGlGraph()->getRenderingParameters().setSelectionColor(
              TulipSettings::instance().defaultSelectionColor());
          glMainView->refresh();
        }
      }
    }
  }
}

void GraphPerspective::setAutoCenterPanelsOnDraw(bool f) {
  _ui->workspace->setAutoCenterPanelsOnDraw(f);
}

void GraphPerspective::pluginsListChanged() {
  _ui->algorithmRunner->refreshPluginsList();
}

void GraphPerspective::addNewGraph() {
  Graph *g = tlp::newGraph();
  _graphs->addGraph(g);
  showStartPanels(g);
}

void GraphPerspective::newProject() {
  createPerspective(name().c_str());
}

void GraphPerspective::openRecentFile() {
  QAction *action = static_cast<QAction *>(sender());
  // workaround a Qt5 bug (seems only Linux related) when upgrading the Qt
  // version :
  // the path of a recently opened file stored in Tulip QSettings got a '&'
  // character
  // added to it, making it invalid and thus the file is not loaded.
  open(action->text().replace("&", ""));
}

void GraphPerspective::treatEvent(const tlp::Event &ev) {
  if (dynamic_cast<const tlp::PluginEvent *>(&ev)) {
    pluginsListChanged();
  }
}

void GraphPerspective::setWorkspaceMode() {
  _ui->workspaceButton->setChecked(true);
  _ui->developButton->setChecked(false);
  _ui->centralWidget->widget(1)->setVisible(false);
  _ui->centralWidget->setCurrentIndex(0);
}

void GraphPerspective::setDevelopMode() {
  _ui->workspaceButton->setChecked(false);
  _ui->developButton->setChecked(true);
  _ui->centralWidget->widget(1)->setVisible(true);
  _ui->centralWidget->setCurrentIndex(1);
}

void GraphPerspective::showUserDocumentation() {
  QDesktopServices::openUrl(QUrl::fromLocalFile(tlpStringToQString(tlp::TulipShareDir) + "doc/tulip-user/html/index.html"));
}

void GraphPerspective::showDevelDocumentation() {
  QDesktopServices::openUrl(QUrl::fromLocalFile(tlpStringToQString(tlp::TulipShareDir) + "doc/tulip-dev/html/index.html"));
}

void GraphPerspective::showPythonDocumentation() {
  QDesktopServices::openUrl(QUrl::fromLocalFile(tlpStringToQString(tlp::TulipShareDir) + "doc/tulip-python/html/index.html"));
}

void GraphPerspective::showHideSideBar() {
  if (_ui->docksWidget->isVisible()) {
    _ui->docksWidget->setVisible(false);
    _ui->sidebarButton->setToolTip("Show Sidebar");
  } else {
    _ui->docksWidget->setVisible(true);
    _ui->sidebarButton->setToolTip("Hide Sidebar");
  }
}

void GraphPerspective::displayColorScalesDialog() {
  _colorScalesDialog->show();
}

void GraphPerspective::showAboutTulipPage() {
  if (!checkSocketConnected()) {
    tlp::AboutTulipPage *aboutPage = new tlp::AboutTulipPage;
    QDialog aboutDialog(mainWindow(), Qt::Window);
    aboutDialog.setWindowTitle("About Tulip");
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(aboutPage);
    layout->setContentsMargins(0, 0, 0, 0);
    aboutDialog.setLayout(layout);
    aboutDialog.resize(800, 600);
    aboutDialog.exec();
  }
}

PLUGIN(GraphPerspective)
