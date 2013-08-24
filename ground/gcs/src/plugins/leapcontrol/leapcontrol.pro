TEMPLATE = lib
TARGET = LeapControl
QT += svg
QT += opengl
QT += network

include(../../taulabsgcsplugin.pri) 
include(../../plugins/coreplugin/coreplugin.pri) 
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += leapcontrolgadget.h
HEADERS += leapcontrolgadgetwidget.h
HEADERS += leapcontrolgadgetfactory.h
HEADERS += leapcontrolplugin.h

SOURCES += leapcontrolgadget.cpp
SOURCES += leapcontrolgadgetwidget.cpp
SOURCES += leapcontrolgadgetfactory.cpp
SOURCES += leapcontrolplugin.cpp

OTHER_FILES += LeapControl.pluginspec

FORMS += leapcontrol.ui
