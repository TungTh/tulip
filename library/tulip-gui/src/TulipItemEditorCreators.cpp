#include "tulip/TulipItemEditorCreators.h"
#include <QtCore/QDebug>
#include <tulip/TlpQtTools.h>
#include <tulip/ColorButton.h>
#include <QtGui/QCheckBox>
#include <QtGui/QStylePainter>
#include <QtGui/QApplication>
#include "tulip/SizeEditor.h"
#include <tulip/ColorScaleConfigDialog.h>

using namespace tlp;

/*
  ColorEditorCreator
*/
QWidget* ColorEditorCreator::createWidget(QWidget *parent) const {
  return new ColorButton(parent);
}

bool ColorEditorCreator::paint(QPainter* painter, const QStyleOptionViewItem& option, const QVariant& v) const {
  painter->setBrush(colorToQColor(v.value<tlp::Color>()));
  painter->setPen(Qt::black);
  painter->drawRect(option.rect.x()+6,option.rect.y()+6,option.rect.width()-12,option.rect.height()-12);
  return true;
}

void ColorEditorCreator::setEditorData(QWidget *editor, const QVariant &data,tlp::Graph*) {
  static_cast<ColorButton*>(editor)->setTulipColor(data.value<tlp::Color>());
}

QVariant ColorEditorCreator::editorData(QWidget *editor,tlp::Graph*) {
  return QVariant::fromValue<tlp::Color>(static_cast<ColorButton*>(editor)->tulipColor());
}

/*
  BooleanEditorCreator
*/
QWidget* BooleanEditorCreator::createWidget(QWidget* parent) const {
  return new QCheckBox(parent);
}

QRect checkRect(const QStyleOptionViewItem &option,const QRect& bounding, QWidget* widget) {
  QStyleOptionButton opt;
  opt.QStyleOption::operator=(option);
  opt.rect = bounding;
  QStyle *style = widget ? widget->style() : QApplication::style();
  return style->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt, widget);
}

bool BooleanEditorCreator::paint(QPainter *p, const QStyleOptionViewItem &option, const QVariant &variant) const {
  QCheckBox cb;
  QStyleOptionViewItem opt(option);
  opt.rect = checkRect(option,option.rect,&cb);
  opt.state |= variant.toBool() ? QStyle::State_On : QStyle::State_Off;
  opt.state = opt.state & ~QStyle::State_HasFocus;
  QStyle* style = cb.style();
  style->drawPrimitive(QStyle::PE_IndicatorViewItemCheck,&opt,p,&cb);
  return true;
}

void BooleanEditorCreator::setEditorData(QWidget* editor, const QVariant &data,tlp::Graph*) {
  static_cast<QCheckBox*>(editor)->setChecked(data.toBool());
}

QVariant BooleanEditorCreator::editorData(QWidget* editor,tlp::Graph*) {
  return static_cast<QCheckBox*>(editor)->isChecked();
}

/*
  SizeEditorCreator
*/
QWidget* SizeEditorCreator::createWidget(QWidget* parent) const {
  return new SizeEditor(parent);
}

void SizeEditorCreator::setEditorData(QWidget* w, const QVariant& v,tlp::Graph*) {
  static_cast<SizeEditor*>(w)->setSize(v.value<tlp::Size>());
}

QVariant SizeEditorCreator::editorData(QWidget* w,tlp::Graph*) {
  return QVariant::fromValue<tlp::Size>(static_cast<SizeEditor*>(w)->size());
}

/*
  PropertyInterfaceEditorCreator
*/
QWidget* PropertyInterfaceEditorCreator::createWidget(QWidget* parent) const {
  return new QComboBox(parent);
}

void PropertyInterfaceEditorCreator::setEditorData(QWidget* w, const QVariant& val,tlp::Graph* g) {
  PropertyInterface* prop = val.value<PropertyInterface*>();

  QSet<QString> locals,inherited;
  std::string name;
  forEach(name,g->getProperties()) {
    PropertyInterface* pi = g->getProperty(name);

    if (g->existLocalProperty(name))
      locals.insert(name.c_str());
    else
      inherited.insert(name.c_str());
  }

  QFont f;
  f.setBold(true);
  QComboBox* combo = static_cast<QComboBox*>(w);
  combo->clear();

  int index=0;
  foreach(QString s,inherited) {
    combo->addItem(s);
    combo->setItemData(index,f,Qt::FontRole);
    combo->setItemData(index,QObject::trUtf8("Inherited"),Qt::ToolTipRole);

    if (prop && s == prop->getName().c_str())
      combo->setCurrentIndex(index);

    index++;
  }

  foreach(QString s,locals) {
    combo->addItem(s);
    combo->setItemData(index,QObject::trUtf8("Local"),Qt::ToolTipRole);

    if (prop && s == prop->getName().c_str())
      combo->setCurrentIndex(index);

    index++;
  }

  if (!prop)
    combo->setCurrentIndex(0);
}

QVariant PropertyInterfaceEditorCreator::editorData(QWidget* w,tlp::Graph* g) {
  QComboBox* combo = static_cast<QComboBox*>(w);
  std::string propName = combo->currentText().toStdString();
  PropertyInterface *prop = NULL;

  if (g->existProperty(propName))
    prop = g->getProperty(propName);

  return QVariant::fromValue<PropertyInterface*>(prop);
}

QString PropertyInterfaceEditorCreator::displayText(const QVariant& v) const {
  PropertyInterface *prop = v.value<PropertyInterface*>();

  if (prop==NULL)
    return "";

  return prop->getName().c_str();
}

/*
  ColorScaleEditorCreator
*/
QWidget* ColorScaleEditorCreator::createWidget(QWidget* parent) const {
  ColorScale cs;
  return new ColorScaleConfigDialog(&cs,parent);
}

bool ColorScaleEditorCreator::paint(QPainter* painter, const QStyleOptionViewItem& option, const QVariant& var) const {
  ColorScale cs = var.value<ColorScale>();
  /*
  QCheckBox cb;
  QStyleOptionViewItem opt(option);
  opt.rect = checkRect(option,option.rect,&cb);
  opt.state |= variant.toBool() ? QStyle::State_On : QStyle::State_Off;
  opt.state = opt.state & ~QStyle::State_HasFocus;
  QStyle* style = cb.style();
  style->drawPrimitive(QStyle::PE_IndicatorViewItemCheck,&opt,p,&cb);
  return true;
  */
  return true;
}

void ColorScaleEditorCreator::setEditorData(QWidget* w, const QVariant& var,tlp::Graph*) {
  static_cast<ColorScaleConfigDialog*>(w)->setColorScale(var.value<ColorScale>());
}

QVariant ColorScaleEditorCreator::editorData(QWidget* w,tlp::Graph*) {
  return QVariant::fromValue<ColorScale>(static_cast<ColorScaleConfigDialog*>(w)->getColorScale());
}
