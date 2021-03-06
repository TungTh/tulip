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

#include <tulip/DrawingTools.h>
#include <tulip/StringCollection.h>

#include "ConnectedComponentPacking.h"
#include "rectanglePackingFonctions.h"
#include "DatasetTools.h"

using namespace std;
using namespace tlp;

PLUGIN(ConnectedComponentPacking)

const float spacing = 8;
#define COMPLEXITY "auto;n5;n4logn;n4;n3logn;n3;n2logn;n2;nlogn;n;"

static const char *paramHelp[] = {
    // coordinates
    "Input layout of nodes and edges.",

    // rotation
    "Input rotation of nodes around the z-axis.",

    // complexity
    "Complexity of the algorithm.<br> n is the number of connected components in the graph."};

//====================================================================
ConnectedComponentPacking::ConnectedComponentPacking(const tlp::PluginContext *context) : LayoutAlgorithm(context) {
  addInParameter<LayoutProperty>("coordinates", paramHelp[0], "viewLayout");
  addNodeSizePropertyParameter(this);
  addInParameter<DoubleProperty>("rotation", paramHelp[1], "viewRotation");
  addInParameter<StringCollection>("complexity", paramHelp[2], COMPLEXITY, true,
                                   "auto <br> n5 <br> n4logn <br> n4 <br> n3logn <br> n3 <br> n2logn <br> n2 <br> nlogn <br> n");
}
//====================================================================
bool ConnectedComponentPacking::run() {

  LayoutProperty *layout = nullptr;
  SizeProperty *size = nullptr;
  DoubleProperty *rotation = nullptr;
  string complexity("auto");

  workingGraph = graph->addCloneSubGraph("workingGraph");

  if (dataSet != nullptr) {
    dataSet->get("coordinates", layout);
    getNodeSizePropertyParameter(dataSet, size);
    dataSet->get("rotation", rotation);
    StringCollection complexityCol;

    if (dataSet->get("complexity", complexityCol))
      complexity = complexityCol.getCurrentString();
  }

  if (layout == nullptr)
    layout = workingGraph->getProperty<LayoutProperty>("viewLayout");

  if (size == nullptr)
    size = workingGraph->getProperty<SizeProperty>("viewSize");

  if (rotation == nullptr)
    rotation = workingGraph->getProperty<DoubleProperty>("viewRotation");

  if (complexity == "none")
    complexity = "auto";

  // compute the connected components's subgraphs
  std::vector<std::set<node>> components;
  ConnectedTest::computeConnectedComponents(workingGraph, components);

  for (unsigned int i = 0; i < components.size(); ++i) {
    workingGraph->inducedSubGraph(components[i]);
  }

  vector<Rectangle<float>> rectangles;
  Iterator<Graph *> *it = workingGraph->getSubGraphs();

  while (it->hasNext()) {
    Graph *sg = it->next();
    BoundingBox tmp = tlp::computeBoundingBox(sg, layout, size, rotation);
    Rectangle<float> tmpRec;
    tmpRec[1][0] = tmp[1][0] + spacing;
    tmpRec[1][1] = tmp[1][1] + spacing;
    tmpRec[0][0] = tmp[0][0] + spacing;
    tmpRec[0][1] = tmp[0][1] + spacing;
    assert(tmpRec.isValid());
    rectangles.push_back(tmpRec);
  }

  delete it;

  if (complexity == "auto") {
    if (rectangles.size() < 25) {
      complexity = "n5";
    } else if (rectangles.size() < 50) {
      complexity = "n4logn";
    } else if (rectangles.size() < 100) {
      complexity = "n4";
    } else if (rectangles.size() < 150) {
      complexity = "n3logn";
    } else if (rectangles.size() < 250) {
      complexity = "n3";
    } else if (rectangles.size() < 500) {
      complexity = "n2logn";
    } else if (rectangles.size() < 1000) {
      complexity = "n2";
    } else if (rectangles.size() < 5000) {
      complexity = "nlogn";
    } else
      complexity = "n";
  }

  vector<Rectangle<float>> rectanglesBackup(rectangles);
  RectanglePackingLimitRectangles(rectangles, complexity.c_str(), pluginProgress);
  Iterator<node> *itN = graph->getNodes();

  while (itN->hasNext()) {
    node n = itN->next();
    result->setNodeValue(n, layout->getNodeValue(n));
  }

  delete itN;
  Iterator<edge> *itE = graph->getEdges();

  while (itE->hasNext()) {
    edge e = itE->next();
    result->setEdgeValue(e, layout->getEdgeValue(e));
  }

  delete itE;
  unsigned int i = 0;
  it = workingGraph->getSubGraphs();

  while (it->hasNext()) {
    Graph *sg = it->next();
    Coord move(rectangles[i][0][0] - rectanglesBackup[i][0][0], rectangles[i][0][1] - rectanglesBackup[i][0][1], 0);
    result->translate(move, sg);
    ++i;
  }

  delete it;
  graph->delAllSubGraphs(workingGraph);

  return true;
}
