TEMPLATE = app
CONFIG  += console c++11
CONFIG  -= qt app_bundle

DESTDIR = ../bin
TARGET  = concur-test-exchange_logic

mingw: TARGET = $${TARGET}-mgw

CONFIG( debug, debug|release ) {
  TARGET = $${TARGET}d
} else {
  DEFINES += NDEBUG
}

include( $$(COMPONENTS_DIR)/qmake/IncludeBoost.pri )
INCLUDEPATH *= $$(BOOST_ROOT)
LIBS        *= $$IncludeBoost( timer unit_test_framework thread system date_time chrono )

INCLUDEPATH *= ../../../

SOURCES += \
    ../src/main.cpp \
    ../src/test_helpers.cpp \
    ../src/test_scene_1.cpp

HEADERS += \
    ../src/test_helpers.h \
    ../src/test_scene_1.h
