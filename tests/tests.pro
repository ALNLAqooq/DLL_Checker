QT += core testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

TARGET = test_peparser

INCLUDEPATH += ../include

SOURCES += \
    test_peparser.cpp \
    ../src/peparser.cpp \
    ../src/comparisonengine.cpp

HEADERS += \
    ../include/peparser.h \
    ../include/comparisonengine.h \
    ../include/dependencyscanner.h

# Windows libraries
win32 {
    LIBS += -limagehlp -lversion
}

# Enable RTTI and exceptions
QMAKE_CXXFLAGS += -frtti -fexceptions

# Test specific settings
DEFINES += QT_TESTLIB_LIB
