/**
 *
 * This file is part of Tulip (www.tulip-software.org)
 *
 * Authors: Tulip development Team
 * from LaBRI, University of Bordeaux
 *
 * This plug-in is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * This plug-in is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */
#ifdef _OPENMP
#include <omp.h>
#define MAX_THREADS (unsigned int)omp_get_max_threads()
#else
#define MAX_THREADS 1
#endif

#include <tulip/TulipException.h>
#include <tulip/GraphTools.h>
#include <tulip/DrawingTools.h>
#include <tulip/StableIterator.h>
#include <tulip/MutableContainer.h>
#include <tulip/LayoutProperty.h>
#include <tulip/BooleanProperty.h>
#include <tulip/DrawingTools.h>
#include <tulip/Rectangle.h>
#include <tulip/SimpleTest.h>

#include "EdgeBundling.h"
#include "QuadTree.h"
#include "OctreeBundle.h"
#include "Dijkstra.h"
#include "BendsTools.h"
#include "SphereUtils.h"

#include <sstream>

using namespace std;
using namespace tlp;

//============================================

static const char *paramHelp[] = {
    // layout
    "The input layout of the graph.",

    // size
    "The input node sizes.",

    // grid_graph
    "If true, a subgraph corresponding to the grid used for routing edges will be added.",

    // 3D_layout
    "If true, it is assumed that the input layout is in 3D and 3D edge bundling will be performed.",

    // sphere_layout
    "If true, it is assumed that nodes have originally been laid out on a sphere surface."
    "Edges will be routed along the sphere surface. The 3D_layout parameter needs also to be set to true"
    " for that feature to work.",

    // long_edges
    "This parameter defines how long edges will be routed. A value less than 1.0 "
    "will promote paths outside dense regions of the input graph drawing.",

    // split_ratio
    "This parameter defines the granularity of the grid that will be generated for routing edges. "
    "The higher its value, the more precise the grid is.",

    // iterations
    "This parameter defines the number of iterations of the edge bundling process. "
    "The higher its value, the more edges will be bundled.",

    // max_thread
    "This parameter defines the number of threads to use for speeding up the edge bundling process. "
    "A value of 0 will use as much threads as processors on the host machine.",

    // edge_node_overlap
    "If true, edges can be routed on original nodes."};

//============================================
EdgeBundling::EdgeBundling(const PluginContext *context) : Algorithm(context) {
  addInParameter<LayoutProperty>("layout", paramHelp[0], "viewLayout");
  addInParameter<SizeProperty>("size", paramHelp[1], "viewSize");
  addInParameter<bool>("grid_graph", paramHelp[2], "false");
  addInParameter<bool>("3D_layout", paramHelp[3], "false");
  addInParameter<bool>("sphere_layout", paramHelp[4], "false");
  addInParameter<double>("long_edges", paramHelp[5], "0.9");
  addInParameter<double>("split_ratio", paramHelp[6], "10");
  addInParameter<unsigned int>("iterations", paramHelp[7], "2");
  addInParameter<unsigned int>("max_thread", paramHelp[8], "0");
  addInParameter<bool>("edge_node_overlap", paramHelp[9], "false");
  addDependency("Voronoi diagram", "1.0");
  addDependency("Equal Value", "1.1");
}
//============================================
class SortNodes {
public:
  static Graph *g;
  static DoubleProperty *dist;
  bool operator()(const node a, const node b) const { // sort in deceresing order;
    if (dist->getNodeValue(a) == dist->getNodeValue(b))
      return a.id > b.id;

    return dist->getNodeValue(a) > dist->getNodeValue(b);
  }
};
DoubleProperty *SortNodes::dist = 0;
Graph *SortNodes::g = 0;
//============================================
DoubleProperty *EdgeBundling::computeWeights(Graph *graph) {
  DoubleProperty *weights = graph->getProperty<DoubleProperty>("cmpWeights");
  for (edge e : graph->getEdges()) {
    const pair<node, node> &ends = graph->ends(e);
    const Coord &a = layout->getNodeValue(ends.first);
    const Coord &b = layout->getNodeValue(ends.second);
    double abNorm = (double)(a - b).norm();
    double initialWeight = pow(abNorm, longEdges);

    if (ntype->getEdgeValue(e) == 2 && !edgeNodeOverlap)
      initialWeight = abNorm;

    weights->setEdgeValue(e, initialWeight);
  }
  return weights;
}
//============================================
void updateLayout(node src, edge e, Graph *graph, LayoutProperty *layout, const vector<node> &nBends, bool layout3D) {
  if (nBends.size() < 3)
    return;

  // if source and target nodes are at the same position, don't set bends to avoid visual artifacts
  // when rendering the graph
  if (layout->getNodeValue(nBends.front()).dist(layout->getNodeValue(nBends.back())) < 1e-5) {
    return;
  }

  vector<Coord> bends(nBends.size() - 2);
  bool sens = true;
  int i = 1;

  if (graph->source(e) == src) {
    i = nBends.size() - 2;
    sens = false;
  }

  for (unsigned int j = 0; j < bends.size(); ++j) {
    Coord coord = layout->getNodeValue(nBends[i]);

    if (!layout3D)
      coord[2] = 0;

    bends[j] = coord;

    if (sens)
      ++i;
    else
      --i;
  }

#ifdef _OPENMP
#pragma omp critical(LAYOUT)
#endif
  layout->setEdgeValue(e, bends);
}
//============================================
// fix all graph edge to 1 and all grid edge to 0 graph-grid edge 2 edge on the contour of a node 3
void EdgeBundling::fixEdgeType() {
  ntype->setAllEdgeValue(0);
  for (edge e : graph->getEdges()) {
    if (oriGraph->isElement(e)) {
      ntype->setEdgeValue(e, 1);
      continue;
    }

    const pair<node, node> &ends = graph->ends(e);

    bool inSrc = oriGraph->isElement(ends.first);

    bool inTgt = oriGraph->isElement(ends.second);

    if (!inSrc && !inTgt)
      ntype->setEdgeValue(e, 0);
    else
      ntype->setEdgeValue(e, 2);
  }
}
//============================================
void computeDik(Dijkstra &dijkstra, const Graph *const vertexCoverGraph, const Graph *const oriGraph, const node n,
                const MutableContainer<double> &mWeights, unsigned int optimizatioLevel) {
  set<node> focus;

  if (optimizatioLevel > 0) {
#ifdef _OPENMP
#pragma omp critical(FOCUS)
#endif
    {
      for (node ni : vertexCoverGraph->getInOutNodes(n))
        focus.insert(ni);
    }
  }

  dijkstra.initDijkstra(oriGraph, n, mWeights, focus);
}
//==========================================================================
void EdgeBundling::computeDistances() {
  for (node n : oriGraph->getNodes()) {
    computeDistance(n);
  }
}
//==========================================================================
void EdgeBundling::computeDistance(node n) {
  double maxDist = 0;
  Coord nPos = layout->getNodeValue(n);
  for (node n2 : vertexCoverGraph->getInOutNodes(n)) {
    const Coord &n2Pos = layout->getNodeValue(n2);
    double dist = (nPos - n2Pos).norm();
    maxDist += dist;
  }
  distance->setNodeValue(n, maxDist);
}
//============================================
bool EdgeBundling::run() {

  optimizationLevel = 3;
  maxThread = 0;
  unsigned int MAX_ITER = 2;
  edgeNodeOverlap = false;
  longEdges = 0.9;
  splitRatio = 10;
  layout3D = false;
  bool sphereLayout = false;
  bool keepGrid = false;
  double dist = 50.0;

  SizeProperty *size = graph->getProperty<SizeProperty>("viewSize");
  layout = graph->getProperty<LayoutProperty>("viewLayout");

  if (dataSet != nullptr) {
    dataSet->get("long_edges", longEdges);
    dataSet->get("split_ratio", splitRatio);
    dataSet->get("iterations", MAX_ITER);
    dataSet->get("optimization", optimizationLevel);
    dataSet->get("edge_node_overlap", edgeNodeOverlap);
    dataSet->get("max_thread", maxThread);
    dataSet->get("3D_layout", layout3D);
    dataSet->get("grid_graph", keepGrid);
    dataSet->get("sphere_layout", sphereLayout);
    dataSet->get("layout", layout);
    dataSet->get("size", size);
  }

#ifdef _OPENMP

  if (maxThread == 0)
    omp_set_num_threads(omp_get_num_procs());
  else
    omp_set_num_threads(maxThread);

  omp_set_nested(true);
  omp_set_dynamic(false);
#endif

  ntype = graph->getProperty<DoubleProperty>("nodetype");
  coloration = graph->getProperty<DoubleProperty>("coloration");

  distance = graph->getProperty<DoubleProperty>("distance");
  ntype->setAllNodeValue(0);
  ntype->setAllEdgeValue(0);

  string err;
  oriGraph = graph->addCloneSubGraph("Original Graph");

  // Make the graph simple
  std::vector<tlp::edge> removedEdges;
  tlp::SimpleTest::makeSimple(oriGraph, removedEdges);

  std::vector<std::vector<tlp::node>> samePositionNodes;

  try {
    // Grid graph computation first step : generate quad-tree/octree
    if (layout3D) {
      OctreeBundle::compute(graph, splitRatio, layout, size);
      for (edge e : stableIterator(graph->getEdges())) {
        if (oriGraph->isElement(e))
          continue;

        graph->delEdge(e);
      }

      if (sphereLayout) {
        centerOnOriginAndScale(graph, layout, dist * 2.);
        addSphereGraph(graph, dist + 0.5 * dist);
        addSphereGraph(graph, dist - 0.2 * dist);
      }
    } else {
      // Preprocess the graph to ensure that two nodes does not have the same position
      // otherwise the quad tree computation will fail

      // First create a clone subgraph
      Graph *workGraph = graph->addCloneSubGraph();

      // Apply the 'Equal Value' algorithm on the input layout property
      DataSet equalValueParams;
      equalValueParams.set("Property", layout);
      string err;
      workGraph->applyAlgorithm("Equal Value", err, &equalValueParams);

      // Iterate on the created clusters
      for (Graph *sg : stableIterator(workGraph->getSubGraphs())) {
        // At least two nodes have the same position
        if (sg->numberOfNodes() > 1) {
          std::vector<tlp::node> nodes;
          int i = 0;
          // keep one of the node in the clone subgraph, remove the others from it
          // but keep a list of those nodes having the same position
          for (node n : stableIterator(sg->getNodes())) {
            if (i++ > 0) {
              workGraph->delNode(n);
            }

            nodes.push_back(n);
          }
          samePositionNodes.push_back(nodes);
        }

        workGraph->delSubGraph(sg);
      }

      // Execute the quad tree computation on cleaned subgraph
      QuadTreeBundle::compute(workGraph, splitRatio, layout, size);
      graph->delSubGraph(workGraph);
    }
  } catch (tlp::TulipException &e) {
    string errorMsg(e.what());
    pluginProgress->setError(errorMsg);
    return false;
  }

  // Grid graph computation second step : generate a voronoi diagram
  // from the original nodes positions and the centers of the previoulsy
  // computed quad-tree/octree cells
  tlp::DataSet voroDataSet;
  voroDataSet.set("connect", true);
  graph->applyAlgorithm("Voronoi diagram", err, &voroDataSet);

  // Remove the clone sub-graph added by the Voronoi plugin
  tlp::Graph *cloneVoronoi = graph->getSubGraph("Original graph");
  graph->delSubGraph(cloneVoronoi);

  // If sphere mode, remove the grid nodes inside the sphere
  // as we only want to route on the sphere surface
  if (sphereLayout) {
    for (node n : stableIterator(graph->getNodes())) {
      if (oriGraph->isElement(n))
        continue;

      const Coord &c = layout->getNodeValue(n);

      if (c.norm() < 0.9 * dist) {
        graph->delNode(n, true);
      }
    }
  }

  fixEdgeType();

  // remove all original graph edges
  //==========================================================
  gridGraph = graph->getSubGraph("Voronoi");
  gridGraph->setName("Grid Graph");

  for (edge e : stableIterator(gridGraph->getEdges())) {
    if (ntype->getEdgeValue(e) == 1) {
      gridGraph->delEdge(e);
    }
  }

  // If there was nodes at the same position, the voronoi diagram process
  // only considers one of them when connecting original graph nodes to
  // their enclosing cell vertices.
  // So connect the other ones to the enclosing cell vertices too
  // in order for the shortest path computation to work
  for (size_t i = 0; i < samePositionNodes.size(); ++i) {
    node rep;

    // get the nodes that has been connected to the voronoi cell vertices
    for (size_t j = 0; j < samePositionNodes[i].size(); ++j) {
      if (gridGraph->deg(samePositionNodes[i][j]) > 0) {
        rep = samePositionNodes[i][j];
        break;
      }
    }

    // connect the other nodes to the enclosing voronoi cell vertices
    for (node n : gridGraph->getOutNodes(rep)) {
      for (size_t j = 0; j < samePositionNodes[i].size(); ++j) {
        if (samePositionNodes[i][j] == rep)
          continue;

        tlp::edge e = gridGraph->addEdge(samePositionNodes[i][j], n);
        ntype->setEdgeValue(e, 2);
      }
    }
  }

  // Initialization of grid edges weights
  //==========================================================
  MutableContainer<double> mWeights;
  MutableContainer<double> mWeightsInit;
  {
    for (edge e : graph->getEdges()) {
      pair<node, node> ends = graph->ends(e);
      const Coord &a = layout->getNodeValue(ends.first);
      const Coord &b = layout->getNodeValue(ends.second);
      double abNorm = (double)(a - b).norm();
      double initialWeight = pow(abNorm, longEdges);

      if (ntype->getEdgeValue(e) == 2 && !edgeNodeOverlap)
        initialWeight = abNorm;

      mWeights.set(e.id, initialWeight);
      mWeightsInit.set(e.id, initialWeight);
    }
  }

  //==========================================================

  DoubleProperty *depth = graph->getProperty<DoubleProperty>("Depth");
  // unsigned int nbDijkstraComputed = 0;
  DoubleProperty *preference = gridGraph->getProperty<DoubleProperty>("edgeTaken");
  preference->setAllNodeValue(0);

  // Load the grid graph into an optimized structure for graph traversal
  Dijkstra::loadGraph(gridGraph);

  // Routing edges into bundles
  for (unsigned int iteration = 0; iteration < MAX_ITER; iteration++) {

    if (iteration < MAX_ITER - 1) {
      depth->setAllNodeValue(0);
      depth->setAllEdgeValue(0);
    }

    vertexCoverGraph = oriGraph->addCloneSubGraph("vertexCoverGraph"); // used for optimizing the vertex cover problem

    MutableContainer<bool> edgeTreated;
    edgeTreated.setAll(false);
    SortNodes::g = vertexCoverGraph;
    SortNodes::dist = distance;
    set<node, SortNodes> orderedNodes;
    computeDistances();
    {
      for (node n : vertexCoverGraph->getNodes())
        orderedNodes.insert(n);
    }

    while (vertexCoverGraph->numberOfNodes() > 0) {
      stringstream strm;
      strm << "Computing iteration " << iteration + 1 << "/" << MAX_ITER;
      pluginProgress->setComment(strm.str());
      unsigned int i = oriGraph->numberOfEdges() - vertexCoverGraph->numberOfEdges();

      if ((i % 10) == 0)
        pluginProgress->progress(i, oriGraph->numberOfEdges());

      //====================================
      // Select the destination nodes. We do not have to compute
      // dijkstra for other nodes.
      vector<node> toTreatByThreads;
      set<node> blockNodes;
      vector<node> toDelete;
      set<node, SortNodes>::iterator it = orderedNodes.begin();

      for (; it != orderedNodes.end(); ++it) {
        node n = *it;

        if ((blockNodes.find(n) == blockNodes.end() || optimizationLevel < 3) && (vertexCoverGraph->deg(n) > 0 || optimizationLevel < 2)) {
          bool addOk = true;

          if (vertexCoverGraph->deg(n) == 1 && optimizationLevel > 1) {
            Iterator<node> *it = vertexCoverGraph->getInOutNodes(n);
            node tmp = it->next();
            delete it;

            if (vertexCoverGraph->deg(tmp) != 1) {
              addOk = false;
            }
          }

          if (addOk) {
            toTreatByThreads.push_back(n);

            if ((optimizationLevel == 3) && (toTreatByThreads.size() < MAX_THREADS)) {
              for (node tmp : vertexCoverGraph->getInOutNodes(n))
                blockNodes.insert(tmp);
            }
          }
        }

        if (vertexCoverGraph->deg(n) == 0 && optimizationLevel > 1) {
          toDelete.push_back(n);
        }

        if (toTreatByThreads.size() >= MAX_THREADS)
          break;
      }

      if (optimizationLevel > 1)
        for (unsigned int i = 0; i < toDelete.size(); i++) {
          orderedNodes.erase(toDelete[i]);
          vertexCoverGraph->delNode(toDelete[i]);
        }

      forceEdgeTest = false;

      int nbThread = toTreatByThreads.size();

      // nbDijkstraComputed += nbThread;
      if (iteration < MAX_ITER - 1) {
#ifdef _OPENMP
#pragma omp parallel for default(none) shared(nbThread, depth, toTreatByThreads, mWeights, edgeTreated) schedule(dynamic, 1)
#endif

        for (int j = 0; j < nbThread; j++) {
          node n = toTreatByThreads[j];
          Dijkstra dijkstra;

          if (edgeNodeOverlap)
            computeDik(dijkstra, vertexCoverGraph, 0, n, mWeights, optimizationLevel);
          else
            computeDik(dijkstra, vertexCoverGraph, oriGraph, n, mWeights, optimizationLevel);

          // for each edge of n compute the shortest paths in the grid
          for (edge e : vertexCoverGraph->getInOutEdges(n)) {
            node n2 = graph->opposite(e, n);

            if (optimizationLevel < 3 || forceEdgeTest) {
              bool stop = false;
#ifdef _OPENMP
#pragma omp critical(EDGETREATED)
#endif
              // when we are not using colration edge can be treated two times
              {
                if (edgeTreated.get(e.id))
                  stop = true;

                edgeTreated.set(e.id, true);
              }

              if (stop) {
                continue;
              }
            }

            BooleanProperty tmpP(gridGraph);
            tmpP.setAllNodeValue(false);
            tmpP.setAllEdgeValue(false);
            dijkstra.searchPaths(n2, depth);
          }
        }
      } else {
#ifdef _OPENMP
#pragma omp parallel for default(none) shared(nbThread, toTreatByThreads, mWeights, preference, edgeTreated) schedule(dynamic, 1)
#endif

        for (int j = 0; j < nbThread; j++) {
          node n = toTreatByThreads[j];
          Dijkstra dijkstra;

          if (edgeNodeOverlap)
            computeDik(dijkstra, vertexCoverGraph, 0, n, mWeights, optimizationLevel);
          else
            computeDik(dijkstra, vertexCoverGraph, oriGraph, n, mWeights, optimizationLevel);

          // for each edge of n compute the shortest paths in the grid
          for (edge e : vertexCoverGraph->getInOutEdges(n)) {
            if (optimizationLevel < 3 || forceEdgeTest) {
              bool stop = false;
#ifdef _OPENMP
#pragma omp critical(EDGETREATED)
#endif
              // when we are not using colration edge can be treated two times
              {
                if (edgeTreated.get(e.id))
                  stop = true;

                edgeTreated.set(e.id, true);
              }

              if (stop)
                continue;
            }

            BooleanProperty tmpP(gridGraph);
            tmpP.setAllNodeValue(false);
            tmpP.setAllEdgeValue(false);
            {
              /// bends
              vector<node> tmpV;
              dijkstra.searchPath(graph->opposite(e, n), tmpV);

              // update preference property (if their is two shortest path try to use the
              // one with high preference)
              for (size_t ii = 0; ii < tmpV.size(); ++ii) {
                double res = preference->getNodeValue(tmpV[ii]) + 1;
#ifdef _OPENMP
#pragma omp critical(PREF)
#endif
                preference->setNodeValue(tmpV[ii], res);
              }

              if (!layout3D)
                tmpV = BendsTools::bendsSimplification(tmpV, layout);

              updateLayout(n, e, graph, layout, tmpV, layout3D);
            }
          }
        }
      }

      for (size_t j = 0; j < toTreatByThreads.size(); ++j) {
        node n = toTreatByThreads[j];
        vector<node> neigbors;
        for (node n2 : vertexCoverGraph->getInOutNodes(n)) {
          neigbors.push_back(n2);
          orderedNodes.erase(n2);
        }
        orderedNodes.erase(n);
        vertexCoverGraph->delNode(n);

        for (unsigned int i = 0; i < neigbors.size(); ++i) {
          computeDistance(neigbors[i]);
          orderedNodes.insert(neigbors[i]);
        }
      }
    }

    oriGraph->delSubGraph(vertexCoverGraph);

    // Adjust weights of routing grid.
    if (iteration < MAX_ITER - 1) {
      for (edge e : gridGraph->getEdges()) {
        mWeights.set(e.id, mWeightsInit.get(e.id));

        if (ntype->getEdgeValue(e) == 2 && !edgeNodeOverlap) {
          continue;
        }

        // double avgdepth = weightFactor * depth->getEdgeValue(e) + 1.;
        double avgdepth = depth->getEdgeValue(e);

        if (avgdepth > 0)
          mWeights.set(e.id, (mWeightsInit.get(e.id) / (log(avgdepth) + 1)));
      }
    }
  }

  // Reinsert multiple edges if any and update their layout
  for (size_t i = 0; i < removedEdges.size(); ++i) {
    const std::pair<node, node> &eEnds = graph->ends(removedEdges[i]);
    tlp::edge origEdge = oriGraph->existEdge(eEnds.first, eEnds.second);

    if (origEdge.isValid()) {
      oriGraph->addEdge(removedEdges[i]);
      layout->setEdgeValue(removedEdges[i], layout->getEdgeValue(origEdge));
    } else {
      origEdge = oriGraph->existEdge(eEnds.second, eEnds.first);
      oriGraph->addEdge(removedEdges[i]);
      std::vector<tlp::Coord> bends = layout->getEdgeValue(origEdge);
      std::reverse(bends.begin(), bends.end());
      layout->setEdgeValue(removedEdges[i], bends);
    }
  }

  // If sphere mode, move the edge bends to the closest point on the sphere surface
  if (sphereLayout) {
    moveBendsToSphere(oriGraph, dist, layout);
  }

  if (!keepGrid) {
    for (node n : stableIterator(graph->getNodes())) {
      if (!oriGraph->isElement(n))
        graph->delNode(n, true);
    }
    graph->delAllSubGraphs(oriGraph);
    graph->delAllSubGraphs(gridGraph);
  }

  return true;
}
//============================================================

PLUGIN(EdgeBundling)
