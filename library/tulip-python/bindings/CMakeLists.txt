ADD_SUBDIRECTORY(stl)
ADD_SUBDIRECTORY(tulip-core)
ADD_SUBDIRECTORY(tulip-ogl)
ADD_SUBDIRECTORY(tulip-gui)

# When building Python wheels on MacOS, copy dependencies dylibs in wheels folders.
# They will be distributed and relinked when generating the wheels.
IF(APPLE AND ACTIVATE_PYTHON_WHEELS_TARGETS AND NOT BUILD_CORE_ONLY)

  COPY_REAL_LIB(${FREETYPE_LIBRARY} ${TULIPOGL_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${GLEW_LIBRARY} ${TULIPOGL_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${FREETYPE_LIBRARY} ${TULIPOGL_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_QTCORE_LIBRARY}/QtCore ${TULIPGUI_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_QTGUI_LIBRARY}/QtGui ${TULIPGUI_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_QTNETWORK_LIBRARY}/QtNetwork ${TULIPGUI_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_QTXML_LIBRARY}/QtXml ${TULIPGUI_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_QTOPENGL_LIBRARY}/QtOpenGL ${TULIPGUI_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_QTWEBKIT_LIBRARY}/QtWebKit ${TULIPGUI_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_QTXMLPATTERNS_LIBRARY}/QtXmlPatterns ${TULIPGUI_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_PHONON_LIBRARY}/phonon ${TULIPGUI_PYTHON_NATIVE_FOLDER})
  COPY_REAL_LIB(${QT_QTDBUS_LIBRARY}/QtDBus ${TULIPGUI_PYTHON_NATIVE_FOLDER})

ENDIF(APPLE AND ACTIVATE_PYTHON_WHEELS_TARGETS AND NOT BUILD_CORE_ONLY)
