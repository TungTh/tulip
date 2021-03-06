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
#ifndef _DagLevelMETRIC_H
#define _DagLevelMETRIC_H

#include <tulip/TulipPluginHeaders.h>
/** \addtogroup metric */

/** This plugin is an implementation of a DAG layer decomposition
 *
 *  \note This algorithm works on general DAG, the complexity is in O(|E|+|V|);
 *
 */
class DagLevelMetric : public tlp::DoubleAlgorithm {
public:
  PLUGININFORMATION("Dag Level", "David Auber", "10/03/2000", "Implements a DAG layer decomposition.", "1.0", "Hierarchical")
  DagLevelMetric(const tlp::PluginContext *context);
  ~DagLevelMetric();
  bool run();
  bool check(std::string &erreurMsg);
};

#endif
