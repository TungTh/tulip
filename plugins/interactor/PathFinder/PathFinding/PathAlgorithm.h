//-*-c++-*-
/*
 Author: Ludwig Fiolka
 Email : ludwig.fiolka@inria.fr
 Last modification : Oct 26, 2009
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 */

#ifndef PATHALGORITHM_H_
#define PATHALGORITHM_H_

#include <tulip/Node.h>
#include <float.h>

namespace tlp {
class BooleanProperty;
class DoubleProperty;
class Graph;
}

/**
 * @brief An facade for any path finding algorithm.
 * This class will initiate and run the correct path finding algorithm relative to the parameters given.
 */
class PathAlgorithm {
public:
  /**
   * By default, Tulip works on oriented edges. This behavior can be overloaded by forcing the edges to be Oriented, non oriented or reversed.
   */
  enum EdgeOrientation { Oriented, NonOriented, Reversed };

  /**
   * A path algorithm can look for only one (shortest) path, all the shortest paths or all the paths.
   */
  enum PathType { OneShortest, AllShortest, AllPaths };

  /**
   * Compute a path between two nodes.
   * @param pathType the type of path to look for.
   * @param edgesOrientation The edge orientation policy.
   * @param src The source node
   * @param tgt The target node
   * @param result Nodes and edges located in the path will be set to true in a resulting boolean property.
   * @param weights The edges weights
   * @param tolerance (only when all paths are selected) The length tolerance factor.
   * @return
   *
   * @see PathType
   * @see EdgeOrientation
   * @see Dikjstra
   * @see DFS
   */
  static bool computePath(tlp::Graph *graph, PathType pathType, EdgeOrientation edgesOrientation, tlp::node src, tlp::node tgt, tlp::BooleanProperty *result, tlp::DoubleProperty *weights=0, double tolerance=DBL_MAX);
};

#endif /* PATHALGORITHM_H_ */