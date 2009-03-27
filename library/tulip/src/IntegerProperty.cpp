//-*-c++-*-
#include <float.h>
#include "tulip/IntegerProperty.h"
#include "tulip/PluginContext.h"
#include "tulip/Observable.h"
#include "tulip/IntegerAlgorithm.h"
#include "tulip/AbstractProperty.h"

using namespace std;
using namespace tlp;

//==============================
///Constructeur d'un IntegerProperty
IntegerProperty::IntegerProperty (Graph *sg):AbstractProperty<IntegerType,IntegerType, IntegerAlgorithm>(sg) {
  minMaxOk=false;
  // the property observes itself; see afterSet... methods
  addPropertyObserver(this);
}
//====================================================================
///Renvoie le minimum de la m�trique associ� aux noeuds du IntegerProperty
int IntegerProperty::getNodeMin() {
  if (!minMaxOk) 
    computeMinMax();
  return minN;
}
//====================================================================
///Renvoie le maximum de la m�trique associ� aux noeuds du IntegerProperty
int IntegerProperty::getNodeMax() {
  if (!minMaxOk) 
    computeMinMax();
  return maxN;
}
//====================================================================
///Renvoie le Minimum de la m�trique associ� aux ar�tes du IntegerProperty
int IntegerProperty::getEdgeMin() {
  if (!minMaxOk) 
    computeMinMax();
  return minE;
}
//====================================================================
///Renvoie le Maximum de la m�trique associ� aux ar�tes du IntegerProperty
int IntegerProperty::getEdgeMax() {
  if (!minMaxOk) 
    computeMinMax();
  return maxE;
}
//========================================================================
///Calcul le min et le Max de la m�trique associ� au proxy
///Attention, la gestion du mim et max des ar�tes n'est pas 
///assur� ici et doit �tre ajout� ult�rieurement
void IntegerProperty::computeMinMax() {
  //cerr << "Compute Min Max" << endl;
  int tmp;
  Iterator<node> *itN=graph->getNodes();
  if (itN->hasNext()) {
    node itn=itN->next();
    tmp=getNodeValue(itn);
    maxN=tmp;
    minN=tmp;
  }
  for (;itN->hasNext();) {
    node itn=itN->next();
    tmp=getNodeValue(itn);
    if (tmp>maxN) maxN=tmp;
    if (tmp<minN) minN=tmp;
  }
  delete itN;
  Iterator<edge> *itE=graph->getEdges();
  if (itE->hasNext()) {
    edge ite=itE->next();
    tmp=getEdgeValue(ite);
    maxE=tmp;
    minE=tmp;
  }
  for (;itE->hasNext();) {
    edge ite=itE->next();
    tmp=getEdgeValue(ite);
    if (tmp>maxE) maxE=tmp;
    if (tmp<minE) minE=tmp;
  }
  delete itE;
  minMaxOk=true;
}
//=================================================================================
void IntegerProperty::clone_handler(AbstractProperty<IntegerType,IntegerType> &proxyC) {
  if (typeid(this)==typeid(&proxyC)) {
    IntegerProperty *proxy=(IntegerProperty *)&proxyC;
    minMaxOk=proxy->minMaxOk;
    if (minMaxOk) {
      minE=proxy->minE;
      maxE=proxy->maxE;
      minN=proxy->minN;
      maxN=proxy->maxN;
    }
  }
  else{
    minMaxOk=false;
  }
}

//=================================================================================
PropertyInterface* IntegerProperty::clonePrototype(Graph * g, std::string n)
{
	if( !g )
		return 0;
	IntegerProperty * p = g->getLocalProperty<IntegerProperty>( n );
	p->setAllNodeValue( getNodeDefaultValue() );
	p->setAllEdgeValue( getEdgeDefaultValue() );
	return p;
}
//=============================================================
void IntegerProperty::copy( const node n0, const node n1, PropertyInterface * p )
{
	if( !p )
		return;
	IntegerProperty * tp = dynamic_cast<IntegerProperty*>(p);
	assert( tp );
	setNodeValue( n0, tp->getNodeValue(n1) );
}
//=============================================================
void IntegerProperty::copy( const edge e0, const edge e1, PropertyInterface * p )
{
	if( !p )
		return;
	IntegerProperty * tp = dynamic_cast<IntegerProperty*>(p);
	assert( tp );
	setEdgeValue( e0, tp->getEdgeValue(e1) );
}
//===============================================================
void IntegerProperty::afterSetNodeValue(PropertyInterface* prop, const node n) {
  if (minMaxOk) {
    IntegerType::RealType val = getNodeValue(n);
    if (val > maxN)
      maxN = val;
    else if (val < minN)
      minN = val;
  }
}
//===============================================================
void IntegerProperty::afterSetEdgeValue(PropertyInterface* prop, const edge e) {
  if (minMaxOk) {
    IntegerType::RealType val = getEdgeValue(e);
    if (val > maxE)
      maxE = val;
    else if (val < minE)
      minE = val;
  }
}
//===============================================================
void IntegerProperty::afterSetAllNodeValue(PropertyInterface* prop) {
  if (minMaxOk)
    minN = maxN = getNodeDefaultValue();
}
//===============================================================
void IntegerProperty::afterSetAllEdgeValue(PropertyInterface* prop) {
  if (minMaxOk)
    minE = maxE = getEdgeDefaultValue();
}
//=================================================================================
PropertyInterface* IntegerVectorProperty::clonePrototype(Graph * g, std::string n) {
  if( !g )
    return 0;
  IntegerVectorProperty * p = g->getLocalProperty<IntegerVectorProperty>( n );
  p->setAllNodeValue( getNodeDefaultValue() );
  p->setAllEdgeValue( getEdgeDefaultValue() );
  return p;
}
//=============================================================
void IntegerVectorProperty::copy( const node n0, const node n1, PropertyInterface * p ) {
  if( !p )
    return;
  IntegerVectorProperty * tp = dynamic_cast<IntegerVectorProperty*>(p);
  assert( tp );
  setNodeValue( n0, tp->getNodeValue(n1) );
}
//=============================================================
void IntegerVectorProperty::copy( const edge e0, const edge e1, PropertyInterface * p ) {
  if( !p )
    return;
  IntegerVectorProperty * tp = dynamic_cast<IntegerVectorProperty*>(p);
  assert( tp );
  setEdgeValue( e0, tp->getEdgeValue(e1) );
}




