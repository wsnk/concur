TEMPLATE = app
CONFIG  += console c++11
CONFIG  -= qt app_bundle

DESTDIR = ../bin
TARGET  = concurrent_pool-test

mingw: TARGET = $${TARGET}-mgw

CONFIG( debug, debug|release ) {
  TARGET = $${TARGET}d
} else {
  DEFINES += NDEBUG
}

include( $$(COMPONENTS_DIR)/qmake/IncludeBoost.pri )
INCLUDEPATH *= $$(BOOST_ROOT)
LIBS        *= $$IncludeBoost( unit_test_framework )

INCLUDEPATH *= ../
INCLUDEPATH *= $$(COMPONENTS_DIR)/libs/comut
INCLUDEPATH *= $$(COMPONENTS_DIR)/libs/concurrency_queue

include( concurrent_pool.pri )

SOURCES += \
    ../test/src/main.cpp \
    ../test/src/test_mt_pool.cpp \
    ../test/src/test_pool.cpp \
    ../test/src/test_scsr_performance.cpp \
    ../test/src/test_scmr_buffer_loop.cpp \
    ../test/src/test_mt_scmr_ring_pool.cpp \
    ../test/src/test_mt_scsr_pool.cpp \
    ../test/src/test_mt_scmr_octopus_pool.cpp \
    ../test/src/test_mt_ptr_ring_pool.cpp

HEADERS += \
    ../test/src/config.h \
    ../test/src/thread_helper.h \
    ../scmr_pool2.h
