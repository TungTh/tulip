/*
    FishEyeInteractor.h

   Created on: 29 mai 2009
       Author: Antoine Lambert
       E-mail: antoine.lambert@labri.fr

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

 */

#ifndef FISHEYEINTERACTOR_H_
#define FISHEYEINTERACTOR_H_

#include <tulip/GLInteractor.h>
#include <tulip/GlMainWidget.h>
#include <tulip/GlShaderProgram.h>

#include "FishEyeConfigWidget.h"

using namespace tlp;

class FishEyeInteractorComponent : public GLInteractorComponent {

public :

  FishEyeInteractorComponent();
  FishEyeInteractorComponent(const FishEyeInteractorComponent &fisheyeInteractorComponent);
  ~FishEyeInteractorComponent();

  bool eventFilter(QObject *widget, QEvent *e);

  InteractorComponent *clone() {
    return new FishEyeInteractorComponent(*this);
  }
  bool compute(GlMainWidget *) {
    return false;
  }

  void viewChanged(View *view);

  void setConfigWidget(FishEyeConfigWidget *configWidget) {
    this->configWidget = configWidget;
  }

  bool draw(GlMainWidget *glMainWidget);

private :

  FishEyeConfigWidget *configWidget;
  Coord fisheyeCenter;
  GlShaderProgram *fisheyeShader;
  bool activateFishEye;

};

/*@{*/
/** \file
 *  \brief  Tulip Fisheye Interactor

 * This interactor plugin allow to perform a fisheye distortion effect on Tulip views.
 * It allows to gain focus on a certain area of a visualization by magnifying it without losing
 * the context.
 *
 * Your graphic card must support shader programs otherwise the plugin won't work.
 *
 *
 *  <b>LICENCE</b>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */
class FishEyeInteractor : public GLInteractorComposite {

public :

  PLUGININFORMATIONS("FishEyeInteractor", "Antoine Lambert", "29/05/2009", "FishEye Interactor", "1.0", "Visualization")

  FishEyeInteractor(const PluginContext *);
  ~FishEyeInteractor();

  void construct();

  void uninstall();

  QWidget *configurationWidget() const;

  unsigned int priority() const {
    return 0;
  }

  virtual bool isCompatible(const std::string &viewName) const {
    return (viewName=="Node Link Diagram view");
  }

private :

  FishEyeConfigWidget *fisheyeConfigWidget;
};



#endif /* FISHEYEINTERACTOR_H_ */