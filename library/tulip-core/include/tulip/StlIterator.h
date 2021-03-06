/*
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
///@cond DOXYGEN_HIDDEN

#ifndef TULIP_STLITERATOR_H
#define TULIP_STLITERATOR_H
#include <map>
#include <tulip/Iterator.h>
#include <tulip/memorypool.h>

namespace tlp {

/**
* @class StlIterator
* @ingroup Iterators
*
* @brief StlIterator wraps a stl iterator.
*
* Creates a Tulip Iterator from two stl iterators.
*
* @param startIt a stl iterator from which to begin the iteration
* @param endIt a stl iterator on which to end the iteration
*
**/
template <typename VALUE, typename ITERATOR> struct StlIterator : public Iterator<VALUE> {
  StlIterator(const ITERATOR &startIt, const ITERATOR &endIt) : it(startIt), itEnd(endIt) {
  }
  VALUE next() {
    VALUE tmp = *it;
    ++it;
    return tmp;
  }
  bool hasNext() {
    return (itEnd != it);
  }

private:
  ITERATOR it, itEnd;
};
//=================================================
template <typename VALUE, typename ITERATOR>
struct MPStlIterator : public StlIterator<VALUE, ITERATOR>, public MemoryPool<MPStlIterator<VALUE, ITERATOR>> {
  MPStlIterator(const ITERATOR &startIt, const ITERATOR &endIt) : StlIterator<VALUE, ITERATOR>(startIt, endIt) {
  }
};
//=================================================
/**
* @brief Convenient function for creating a StlIterator from a stl container.
* @ingroup Iterators
*
* Creates a StlIterator from a stl container.
*
* @param stlContainer a stl container (e.g. std::vector)
* @return a StlIterator
**/
template <class Container>
inline StlIterator<typename Container::value_type, typename Container::const_iterator> *stlIterator(const Container &stlContainer) {
  return new MPStlIterator<typename Container::value_type, typename Container::const_iterator>(stlContainer.begin(), stlContainer.end());
}
//=================================================
template <typename KEY, typename VALUE> struct StlMapIterator : public Iterator<std::pair<KEY, VALUE>> {
  StlMapIterator(typename std::map<KEY, VALUE>::const_iterator startIt, typename std::map<KEY, VALUE>::const_iterator endIt)
      : it(startIt), itEnd(endIt) {
  }

  std::pair<KEY, VALUE> next() {
    std::pair<KEY, VALUE> tmp = *it;
    ++it;
    return tmp;
  }

  bool hasNext() {
    return (itEnd != it);
  }

private:
  typename std::map<KEY, VALUE>::const_iterator it, itEnd;
};
//=================================================
template <typename KEY, typename VALUE> struct StlMapKeyIterator : public tlp::Iterator<KEY> {
  StlMapKeyIterator(typename std::map<KEY, VALUE>::const_iterator startIt, typename std::map<KEY, VALUE>::const_iterator endIt)
      : it(startIt), itEnd(endIt) {
  }

  KEY next() {
    const KEY tmp = it->first;
    ++it;
    return tmp;
  }

  bool hasNext() {
    return it != itEnd;
  }

private:
  typename std::map<KEY, VALUE>::const_iterator it, itEnd;
};
}

#endif
///@endcond
