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

#include <tulip/PropertyAlgorithm.h>
#include <tulip/LayoutProperty.h>

static const char *paramHelp[] = {
    // layout
    "The layout property from which a perfect aspect ratio has to be computed."};

class PerfectLayout : public tlp::LayoutAlgorithm {
public:
  PLUGININFORMATION("Perfect aspect ratio", "Tulip team", "09/19/2010", "Scales the graph layout to get an aspect ratio of 1.", "1.0", "")

  PerfectLayout(const tlp::PluginContext *context) : LayoutAlgorithm(context) {
    addInParameter<tlp::LayoutProperty>("layout", paramHelp[0], "viewLayout", false);
  }

  virtual ~PerfectLayout() {
  }

  bool run() {
    tlp::LayoutProperty *layout = nullptr;

    if (dataSet != nullptr) {
      dataSet->get("layout", layout);
    }

    if (!layout)
      layout = graph->getProperty<tlp::LayoutProperty>("viewLayout");

    result->copy(layout);
    result->perfectAspectRatio();
    return true;
  }
};

PLUGIN(PerfectLayout)
