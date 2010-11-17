#include "tulip/QClusteringQualityMeasure.h"

#include "tulip/ClusteringAlgorithmBase.h"
#include <iostream>

#include <tulip/DoubleProperty.h>
#include <tulip/Edge.h>
#include <tulip/ForEach.h>

using namespace tlp;

QClusteringQualityMeasure::QClusteringQualityMeasure(const tlp::ClusteringAlgorithmBase*const clustering) 
 : ClusteringQualityMeasure(clustering), _sumEdges(0) {
}

void QClusteringQualityMeasure::initialize() {
  DoubleProperty* metric = NULL;
  _clusteringAlgorithm->getDataSet().get<DoubleProperty*>("metric", metric);
  if(metric) {
    std::cout << "metric :)" << std::endl; 
    edge e;
    forEach(e, _clusteringAlgorithm->getOriginalGraph()->getEdges())
      _sumEdges += metric->getEdgeValue(e);
  }
  else {
    _sumEdges = _clusteringAlgorithm->getOriginalGraph()->numberOfEdges();
  }
}

double QClusteringQualityMeasure::getQuality() {
  double result = 0;
  node n;
  forEach(n,_clusteringAlgorithm->getQuotientGraph()->getNodes()){
    double e_ii = _clusteringAlgorithm->getIntraEdges(n);
    double e_ij = _clusteringAlgorithm->getExtraEdges(n);
    
    e_ij += e_ii;
//     std::cout << result << "+=" << e_ii << "-" << e_ij << std::endl; 
    e_ii /= _sumEdges;
    e_ij /= _sumEdges;
    e_ij *= e_ij;
    result += (e_ii - e_ij);
  }
  _q = result;
  //  std::cout << "Quality Q : " << result << " => " << _clusteringAlgorithm->getQuotientGraph()->numberOfNodes() << std::endl;
  return result;
}

void QClusteringQualityMeasure::removeFromQ(node n1, double &measure){
  double intra_q = _clusteringAlgorithm->getIntraEdges(n1);
  double extra_q = 0;

  edge e;
  forEach(e, _clusteringAlgorithm->getQuotientGraph()->getInOutEdges(n1)) {
    node n = _clusteringAlgorithm->getQuotientGraph()->opposite(e, n1);
    extra_q += _clusteringAlgorithm->getExtraEdges(e);
  }

  extra_q += intra_q;
  intra_q /= _sumEdges;
  extra_q /= _sumEdges;
  extra_q *= intra_q;

  measure -= (intra_q - extra_q);  
}

double QClusteringQualityMeasure::getQualityIfMerged(node n1, node n2) {
  double measure = _q;
  removeFromQ(n1,measure);
  removeFromQ(n2, measure);
 
  double extra1 = 0.;
  double extra2 = 0.;

  edge n1n2;
  double val_n1n2=0;;
  edge e;
  forEach(e, _clusteringAlgorithm->getQuotientGraph()->getInOutEdges(n1)) {
    node n = _clusteringAlgorithm->getQuotientGraph()->opposite(e, n1);
    if (n2 == n) {
      n1n2 = e;
      val_n1n2 +=(double) _clusteringAlgorithm->getExtraEdges(n1n2);
    }
    else
      extra1 += _clusteringAlgorithm->getExtraEdges(e);
  }
  forEach(e, _clusteringAlgorithm->getQuotientGraph()->getInOutEdges(n2)) {
    node n = _clusteringAlgorithm->getQuotientGraph()->opposite(e, n2);
    if(n == n1) continue;
    extra2 += _clusteringAlgorithm->getExtraEdges(e);
  }
  
  double new_intra = _clusteringAlgorithm->getIntraEdges(n1) + _clusteringAlgorithm->getIntraEdges(n2) + val_n1n2;
  double new_extra = extra1 + extra2 + new_intra;

  new_intra /= _sumEdges;
  new_extra /= _sumEdges;
  new_extra *= new_intra;
  
  measure += new_intra - new_extra;
  //std::cout << "old Q = " << _q << " => new Q = " << measure << " with new_intra = " << new_intra << " and new_extra = "<< new_extra << std::endl;
  return measure;

 /*
  double a1 = 0.;
  double a2 = 0.;

  edge n1n2;
  double val_n1n2=0;;
  edge e;
  forEach(e, _clusteringAlgorithm->getQuotientGraph()->getInOutEdges(n1)) {
    node n = _clusteringAlgorithm->getQuotientGraph()->opposite(e, n1);
    if (n2 == n) {
      n1n2 = e;
      val_n1n2 +=(double) _clusteringAlgorithm->getExtraEdges(n1n2);
    }
    a1 += _clusteringAlgorithm->getExtraEdges(e);
  }
  forEach(e, _clusteringAlgorithm->getQuotientGraph()->getInOutEdges(n2)) {
    a2 += _clusteringAlgorithm->getExtraEdges(e);
  }
  a1 += _clusteringAlgorithm->getIntraEdges(n1);
  a2 += _clusteringAlgorithm->getIntraEdges(n2);
  
  a1 /= _sumEdges;
  a2 /= _sumEdges;
  
  double feij = val_n1n2 / _sumEdges;
  return (feij - 2. * a1 * a2 + 2 * feij * (a1 + a2) - feij*feij);  */
}
