IF(NOT EMSCRIPTEN)
INSTALL(FILES
  fontb.ttf
  font.ttf 
  fontawesome-webfont.ttf
  fontawesome-webfont.woff
  fontawesome-webfont.woff2
  halfCylinderTexture.png
  cylinderTexture.png
  roundTexture.png
  materialdesignicons-webfont.ttf
DESTINATION ${TulipBitmapInstallDir}
COMPONENT tulip_ogl)

INSTALL(DIRECTORY
  fonts
COMPONENT tulip_ogl
DESTINATION ${TulipBitmapInstallDir}
PATTERN ".svn" EXCLUDE)
ENDIF(NOT EMSCRIPTEN)

IF(ACTIVATE_PYTHON_WHEELS_TARGETS)
ADD_CUSTOM_TARGET(copyTulipOglBitmaps ALL ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/fontb.ttf ${TULIPOGL_PYTHON_FOLDER}/share/bitmaps/fontb.ttf
                                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/font.ttf ${TULIPOGL_PYTHON_FOLDER}/share/bitmaps/font.ttf
                                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/fontawesome-webfont.ttf ${TULIPOGL_PYTHON_FOLDER}/share/bitmaps/fontawesome-webfont.ttf
                                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/halfCylinderTexture.png ${TULIPOGL_PYTHON_FOLDER}/share/bitmaps/halfCylinderTexture.png
                                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/cylinderTexture.png ${TULIPOGL_PYTHON_FOLDER}/share/bitmaps/cylinderTexture.png
                                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/roundTexture.png ${TULIPOGL_PYTHON_FOLDER}/share/bitmaps/roundTexture.png)
ADD_DEPENDENCIES(tulip-gui-wheel copyTulipOglBitmaps)
ENDIF(ACTIVATE_PYTHON_WHEELS_TARGETS)
