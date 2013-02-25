/**
 *
 * This file is part of Tulip (www.tulip-software.org)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux 1 and Inria Bordeaux - Sud Ouest
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

#ifndef HISTOGRAMVIEWNAVIGATOR_H_
#define HISTOGRAMVIEWNAVIGATOR_H_

#ifdef  _WIN32
// compilation pb workaround
#include <windows.h>
#endif

#include <tulip/GLInteractor.h>

namespace tlp {

class Histogram;
class HistogramView;

class HistogramViewNavigator : public GLInteractorComponent {

public :

	HistogramViewNavigator();
	~HistogramViewNavigator();

	bool eventFilter(QObject *, QEvent *);
	bool draw(GlMainWidget *) {return false;}
	InteractorComponent *clone() {return new HistogramViewNavigator();}
	bool compute(GlMainWidget *) {return false;}
  void viewChanged(View *view);


private :

    Histogram *getOverviewUnderPointer(const Coord &sceneCoord) const;

	HistogramView *histoView;
	Histogram *selectedHistoOverview;
};

}

#endif /* HISTOGRAMVIEWNAVIGATOR_H_ */