INCLUDE_DIRECTORIES(${TulipCoreBuildInclude} ${TulipCoreInclude} ${TulipOGLESInclude} ${GLEW_INCLUDE_DIR})

MACRO(GLYPH name)
  ADD_LIBRARY(${name}-${TulipVersion} SHARED ${ARGN})
  TARGET_LINK_LIBRARIES(${name}-${TulipVersion} ${LibTulipCoreName} ${LibTulipOGLESName})
  INSTALL_TULIP_PLUGIN(${name}-${TulipVersion} ${TulipGlyphPluginsInstallDir})
ENDMACRO(GLYPH)

GLYPH(Cone ConeGlyph.cpp)
GLYPH(Cross CrossGlyph.cpp)
GLYPH(Cube CubeGlyph.cpp)
GLYPH(Cylinder CylinderGlyph.cpp)
GLYPH(Diamond DiamondGlyph.cpp)
GLYPH(Hexagon HexagonGlyph.cpp)
GLYPH(Pentagon PentagonGlyph.cpp)
Glyph(Ring RingGlyph.cpp)
Glyph(RoundedBox RoundedBoxGlyph.cpp)
Glyph(Sphere SphereGlyph.cpp)
Glyph(Square SquareGlyph.cpp)
Glyph(Star StarGlyph.cpp)
