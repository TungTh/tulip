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

#include "tulip/QuickAccessBar.h"

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QFontDatabase>
#include <QGraphicsView>
#include <QListView>
#include <QMainWindow>
#include <QMouseEvent>

#include <tulip/ColorProperty.h>
#include <tulip/ForEach.h>
#include <tulip/GlGraph.h>
#include <tulip/GlGraphInputData.h>
#include <tulip/GlGraphRenderingParameters.h>
#include <tulip/GlMainView.h>
#include <tulip/GlMainWidget.h>
#include <tulip/GraphModel.h>
#include <tulip/GraphPropertiesModel.h>
#include <tulip/Perspective.h>
#include <tulip/SnapshotDialog.h>
#include <tulip/TlpQtTools.h>
#include <tulip/TulipFontDialog.h>
#include <tulip/TulipItemDelegate.h>

using namespace tlp;

class TopPopupComboBox : public QComboBox {
  QListView *_view;

public:
  TopPopupComboBox(QWidget *parent = nullptr) : QComboBox(parent), _view(nullptr) {
  }

  bool eventFilter(QObject *, QEvent *ev) {
    if (ev->type() == QEvent::MouseButtonPress) {
      QMouseEvent *mouseEv = static_cast<QMouseEvent *>(ev);

      if (!_view->geometry().contains(mouseEv->globalPos()))
        _view->close();
      else {
        setCurrentIndex(_view->indexAt(mouseEv->pos()).row());
        _view->close();
      }
    }

    return false;
  }

  virtual void showPopup() {
    QPoint mainWindowPos = tlp::Perspective::instance()->mainWindow()->pos();

    if (_view == nullptr) {
      _view = findChild<QListView *>();
      _view->installEventFilter(this);
      _view->viewport()->installEventFilter(this);
      _view->setParent(nullptr);
      _view->setMouseTracking(true);
    }

    _view->setAutoScroll(false);
    _view->setWindowFlags(Qt::Popup);
    _view->resize(width(), 200);
    _view->move(mapToGlobal(pos()).x() + mainWindowPos.x(), QCursor::pos().y() - 200 - height() / 2);
    _view->show();
  }

  virtual void hidePopup() {
    if (_view != nullptr)
      _view->close();
  }
};

QuickAccessBar::QuickAccessBar(QWidget *parent) : QWidget(parent), _mainView(nullptr) {
}

#include "ui_QuickAccessBar.h"

QuickAccessBarImpl::QuickAccessBarImpl(QGraphicsItem *quickAccessBarItem, QWidget *parent)
    : QuickAccessBar(parent), _ui(new Ui::QuickAccessBar), _quickAccessBarItem(quickAccessBarItem), delegate(new TulipItemDelegate(this)),
      _oldFontScale(1), _oldNodeScale(1) {
  _ui->setupUi(this);
  _ui->backgroundColorButton->setDialogTitle("Choose the background color");
  _ui->nodeColorButton->setDialogTitle("Choose the node's default color");
  _ui->edgeColorButton->setDialogTitle("Choose the edge's default color");
  _ui->nodeBorderColorButton->setDialogTitle("Choose the default color for the border of nodes");
  _ui->edgeBorderColorButton->setDialogTitle("Choose the default color for the border of edges");
  _ui->labelColorButton->setDialogTitle("Choose the default color for the label of nodes or edges");
}

QuickAccessBarImpl::~QuickAccessBarImpl() {
  delete _ui;
}

void QuickAccessBar::setGlMainView(GlMainView *v) {
  _mainView = v;
  reset();
}

void QuickAccessBarImpl::reset() {
  _resetting = true;

  _ui->backgroundColorButton->setDialogParent(tlp::Perspective::instance() ? tlp::Perspective::instance()->mainWindow() : nullptr);
  _ui->nodeColorButton->setDialogParent(tlp::Perspective::instance() ? tlp::Perspective::instance()->mainWindow() : nullptr);
  _ui->edgeColorButton->setDialogParent(tlp::Perspective::instance() ? tlp::Perspective::instance()->mainWindow() : nullptr);
  _ui->nodeBorderColorButton->setDialogParent(tlp::Perspective::instance() ? tlp::Perspective::instance()->mainWindow() : nullptr);
  _ui->edgeBorderColorButton->setDialogParent(tlp::Perspective::instance() ? tlp::Perspective::instance()->mainWindow() : nullptr);
  _ui->labelColorButton->setDialogParent(tlp::Perspective::instance() ? tlp::Perspective::instance()->mainWindow() : nullptr);

  _ui->backgroundColorButton->setTulipColor(scene()->getBackgroundColor());
  _ui->colorInterpolationToggle->setChecked(renderingParameters()->interpolateEdgesColors());
  _ui->colorInterpolationToggle->setIcon((renderingParameters()->interpolateEdgesColors()
                                              ? QIcon(":/tulip/gui/icons/20/color_interpolation_enabled.png")
                                              : QIcon(":/tulip/gui/icons/20/color_interpolation_disabled.png")));
  _ui->sizeInterpolationToggle->setChecked(renderingParameters()->interpolateEdgesSizes());
  _ui->sizeInterpolationToggle->setIcon((renderingParameters()->interpolateEdgesSizes()
                                             ? QIcon(":/tulip/gui/icons/20/size_interpolation_enabled.png")
                                             : QIcon(":/tulip/gui/icons/20/size_interpolation_disabled.png")));
  _ui->showEdgesToggle->setChecked(renderingParameters()->displayEdges());
  _ui->showEdgesToggle->setIcon(
      (renderingParameters()->displayEdges() ? QIcon(":/tulip/gui/icons/20/edges_enabled.png") : QIcon(":/tulip/gui/icons/20/edges_disabled.png")));
  _ui->showLabelsToggle->setChecked(renderingParameters()->displayNodesLabels());
  _ui->showLabelsToggle->setIcon((renderingParameters()->displayNodesLabels() ? QIcon(":/tulip/gui/icons/20/labels_enabled.png")
                                                                              : QIcon(":/tulip/gui/icons/20/labels_disabled.png")));
  _ui->labelsScaledToggle->setChecked(renderingParameters()->labelsScaled());
  _ui->labelsScaledToggle->setIcon((renderingParameters()->labelsScaled() ? QIcon(":/tulip/gui/icons/20/labels_scaled_enabled.png")
                                                                          : QIcon(":/tulip/gui/icons/20/labels_scaled_disabled.png")));
  updateFontButtonStyle();
  _resetting = false;
}

void QuickAccessBarImpl::takeSnapshot() {
  _mainView->openSnapshotDialog();
}

void QuickAccessBarImpl::setBackgroundColor(const QColor &c) {
  if (scene()->getBackgroundColor() != QColorToColor(c)) {
    scene()->setBackgroundColor(QColorToColor(c));
    _mainView->emitDrawNeededSignal();
    emit settingsChanged();
  }
}

void QuickAccessBarImpl::setColorInterpolation(bool f) {
  if (renderingParameters()->interpolateEdgesColors() != f) {
    renderingParameters()->setInterpolateEdgesColors(f);
    _ui->colorInterpolationToggle->setIcon(
        (f ? QIcon(":/tulip/gui/icons/20/color_interpolation_enabled.png") : QIcon(":/tulip/gui/icons/20/color_interpolation_disabled.png")));
    _mainView->emitDrawNeededSignal();
    emit settingsChanged();
  }
}

void QuickAccessBarImpl::setSizeInterpolation(bool f) {
  if (renderingParameters()->interpolateEdgesSizes() != f) {
    renderingParameters()->setInterpolateEdgesSizes(f);
    _ui->sizeInterpolationToggle->setIcon(
        (f ? QIcon(":/tulip/gui/icons/20/size_interpolation_enabled.png") : QIcon(":/tulip/gui/icons/20/size_interpolation_disabled.png")));
    _mainView->emitDrawNeededSignal();
    emit settingsChanged();
  }
}

void QuickAccessBarImpl::setLabelColor(const QColor &c) {

  BooleanProperty *selected = inputData()->getElementSelection();
  bool hasSelected = false;

  _mainView->graph()->push();

  Observable::holdObservers();
  ColorProperty *labelColors = inputData()->getElementLabelColor();

  Color color = QColorToColor(c);

  for (node n : selected->getNonDefaultValuatedNodes(_mainView->graph())) {
    labelColors->setNodeValue(n, color);
    hasSelected = true;
  }

  if (hasSelected == false) {
    labelColors->setAllNodeValue(color, _mainView->graph());
  }

  for (edge e : selected->getNonDefaultValuatedEdges(_mainView->graph())) {
    labelColors->setEdgeValue(e, color);
    hasSelected = true;
  }

  if (hasSelected == false) {
    labelColors->setAllEdgeValue(color, _mainView->graph());
  }

  Observable::unholdObservers();
  emit settingsChanged();
}

void QuickAccessBarImpl::setAllColorValues(unsigned int eltType, ColorProperty *prop, const Color &color) {
  BooleanProperty *selected = inputData()->getElementSelection();
  bool hasSelected = false;

  _mainView->graph()->push();

  Observable::holdObservers();

  if (eltType == NODE) {
    for (node n : selected->getNonDefaultValuatedNodes(_mainView->graph())) {
      prop->setNodeValue(n, color);
      hasSelected = true;
    }

    if (hasSelected == false)
      prop->setAllNodeValue(color, _mainView->graph());
  } else {
    for (edge e : selected->getNonDefaultValuatedEdges(_mainView->graph())) {
      prop->setEdgeValue(e, color);
      hasSelected = true;
    }

    if (hasSelected == false)
      prop->setAllEdgeValue(color, _mainView->graph());
  }

  Observable::unholdObservers();
  emit settingsChanged();
}

void QuickAccessBarImpl::setNodeColor(const QColor &c) {
  setAllColorValues(NODE, inputData()->getElementColor(), QColorToColor(c));
}

void QuickAccessBarImpl::setEdgeColor(const QColor &c) {
  setAllColorValues(EDGE, inputData()->getElementColor(), QColorToColor(c));
}

void QuickAccessBarImpl::setNodeBorderColor(const QColor &c) {
  setAllColorValues(NODE, inputData()->getElementBorderColor(), QColorToColor(c));
}

void QuickAccessBarImpl::setEdgeBorderColor(const QColor &c) {
  setAllColorValues(EDGE, inputData()->getElementBorderColor(), QColorToColor(c));
}

void QuickAccessBarImpl::setAllValues(unsigned int eltType, PropertyInterface *prop) {
  QVariant val = TulipItemDelegate::showEditorDialog((tlp::ElementType)eltType, prop, _mainView->graph(), delegate,
                                                     Perspective::instance() ? static_cast<QWidget *>(Perspective::instance()->mainWindow())
                                                                             : static_cast<QWidget *>(_mainView->graphicsView()));

  // Check if edition has been cancelled
  if (!val.isValid())
    return;

  BooleanProperty *selected = inputData()->getElementSelection();
  bool hasSelected = false;

  _mainView->graph()->push();

  Observable::holdObservers();

  if (eltType == NODE) {
    for (node n : selected->getNonDefaultValuatedNodes(_mainView->graph())) {
      GraphModel::setNodeValue(n.id, prop, val);
      hasSelected = true;
    }

    if (hasSelected == false)
      GraphModel::setAllNodeValue(prop, val, _mainView->graph());
  } else {
    for (edge e : selected->getNonDefaultValuatedEdges(_mainView->graph())) {
      GraphModel::setEdgeValue(e.id, prop, val);
      hasSelected = true;
    }

    if (hasSelected == false)
      GraphModel::setAllEdgeValue(prop, val, _mainView->graph());
  }

  Observable::unholdObservers();
  emit settingsChanged();
}

void QuickAccessBarImpl::setNodeShape() {
  setAllValues(NODE, inputData()->getElementShape());
}

void QuickAccessBarImpl::setEdgeShape() {
  setAllValues(EDGE, inputData()->getElementShape());
}

void QuickAccessBarImpl::setNodeSize() {
  setAllValues(NODE, inputData()->getElementSize());
}

void QuickAccessBarImpl::setEdgeSize() {
  setAllValues(EDGE, inputData()->getElementSize());
}

void QuickAccessBarImpl::setNodeLabelPosition() {
  setAllValues(NODE, inputData()->getElementLabelPosition());
}

void QuickAccessBarImpl::setEdgesVisible(bool v) {
  if (renderingParameters()->displayEdges() != v) {
    renderingParameters()->setDisplayEdges(v);
    _ui->showEdgesToggle->setIcon((v ? QIcon(":/tulip/gui/icons/20/edges_enabled.png") : QIcon(":/tulip/gui/icons/20/edges_disabled.png")));
    _mainView->emitDrawNeededSignal();
    emit settingsChanged();
  }
}

void QuickAccessBarImpl::setLabelsVisible(bool v) {
  if (renderingParameters()->displayNodesLabels() != v) {
    renderingParameters()->setDisplayNodesLabels(v);
    _ui->showLabelsToggle->setIcon((v ? QIcon(":/tulip/gui/icons/20/labels_enabled.png") : QIcon(":/tulip/gui/icons/20/labels_disabled.png")));
    _mainView->emitDrawNeededSignal();
    emit settingsChanged();
  }
}

void QuickAccessBarImpl::setLabelsScaled(bool v) {
  if (renderingParameters()->labelsScaled() != v) {
    renderingParameters()->setLabelsScaled(v);
    _ui->labelsScaledToggle->setIcon(
        (v ? QIcon(":/tulip/gui/icons/20/labels_scaled_enabled.png") : QIcon(":/tulip/gui/icons/20/labels_scaled_disabled.png")));
    _mainView->emitDrawNeededSignal();
    emit settingsChanged();
  }
}

GlGraphRenderingParameters *QuickAccessBarImpl::renderingParameters() const {
  return &scene()->getMainGlGraph()->getRenderingParameters();
}

GlGraphInputData *QuickAccessBarImpl::inputData() const {
  return &scene()->getMainGlGraph()->getInputData();
}

GlScene *QuickAccessBarImpl::scene() const {
  return _mainView->getGlMainWidget()->getScene();
}

void QuickAccessBarImpl::selectFont() {
  TulipFontDialog dlg;
  dlg.selectFont(TulipFont::fromFile(inputData()->getElementFont()->getNodeDefaultValue().c_str()));

  if (dlg.exec() != QDialog::Accepted || !dlg.font().exists())
    return;

  _mainView->graph()->push();

  Observable::holdObservers();

  inputData()->getElementFont()->setAllNodeValue(QStringToTlpString(dlg.font().fontFile()), _mainView->graph());

  inputData()->getElementFont()->setAllEdgeValue(QStringToTlpString(dlg.font().fontFile()), _mainView->graph());

  Observable::unholdObservers();
  updateFontButtonStyle();
  emit settingsChanged();
}

void QuickAccessBarImpl::updateFontButtonStyle() {
  QString fontFile = inputData()->getElementFont()->getNodeDefaultValue().c_str();
  TulipFont selectedFont = TulipFont::fromFile(fontFile);
  _ui->fontButton->setStyleSheet("font-family: " + selectedFont.fontFamily() + "; " + (selectedFont.isItalic() ? "font-style: italic; " : "") +
                                 (selectedFont.isBold() ? "font-weight: bold; " : ""));
  //_ui->fontButton->setText(selectedFont.fontName() + (selectedFont.isBold() ? " Bold" : "") + (selectedFont.isItalic() ? " Italic" : ""));
}
