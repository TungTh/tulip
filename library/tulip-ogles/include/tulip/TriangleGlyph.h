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

/**
 *
 * tulip-ogles library : rewrite of old tulip-ogl library using OpenGL ES API
 * Copyright (c) 2016 Antoine Lambert, Thales Services SAS
 * antoine-e.lambert@thalesgroup.com / antoine.lambert33@gmail.com
 *
 */

#include <tulip/Glyph.h>
#include <tulip/TulipViewSettings.h>

using namespace tlp;

class TLP_GLES_SCOPE TriangleGlyph : public Glyph {

public:
  GLYPHINFORMATION("2D - Triangle", "2D - Arrow", "Antoine Lambert", "19/05/2016", "Triangle", "1.0", NodeShape::Triangle)

  TriangleGlyph(PluginContext *context = nullptr);

  void getIncludeBoundingBox(BoundingBox &boundingBox);
};
