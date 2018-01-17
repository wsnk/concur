TEMPLATE = app
CONFIG  += console c++11
CONFIG  -= qt app_bundle

DESTDIR = ../bin
TARGET  = perf-test

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
    ../src/thread_master.cpp \
    ../src/test_scmp_ring_collection.cpp

HEADERS += \
    ../src/test_config.h \
    ../src/thread_master.h


