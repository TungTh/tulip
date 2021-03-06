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
#include "Kruskal.h"

#include <tulip/GraphTools.h>
#include <tulip/ConnectedTest.h>
#include <tulip/DoubleProperty.h>

PLUGIN(Kruskal)

using namespace std;
using namespace tlp;

static const char *paramHelp[] = {
    // edge weight
    "Metric containing the edge weights."};
//======================================================
Kruskal::Kruskal(const tlp::PluginContext *context) : BooleanAlgorithm(context) {
  addInParameter<NumericProperty *>("edge weight", paramHelp[0], "viewMetric");
}
//======================================================
Kruskal::~Kruskal() {
}
//======================================================

bool Kruskal::check(std::string &erreurMsg) {
  if (ConnectedTest::isConnected(graph)) {
    erreurMsg = "";
    return true;
  } else {
    erreurMsg = "The graph must be connected.";
    return false;
  }
}
//======================================================
/// Compute the Minimum Spanning Tree
bool Kruskal::run() {
  /* Initialisation */
  NumericProperty *edgeWeight = nullptr;

  if (dataSet != nullptr) {
    dataSet->get("edge weight", edgeWeight);
  }

  if (edgeWeight == nullptr)
    edgeWeight = graph->getProperty<DoubleProperty>("viewMetric");

  selectMinimumSpanningTree(graph, result, edgeWeight, pluginProgress);

  return true;
}
//=======================================================================
