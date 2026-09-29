TARGET = qmodbus
TEMPLATE = app
VERSION = 0.1.0

QT += gui widgets serialport

CONFIG += c++17

SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/BatchProcessor.cpp \
    3rdparty/libmodbus/src/modbus.c \
    3rdparty/libmodbus/src/modbus-data.c \
    3rdparty/libmodbus/src/modbus-rtu.c \
    3rdparty/libmodbus/src/modbus-tcp.c \
    3rdparty/libmodbus/src/modbus-ascii.c \
    src/asciisettingswidget.cpp \
    src/rtusettingswidget.cpp \
    src/serialsettingswidget.cpp \
    src/tcpipsettingswidget.cpp \
    src/ipaddressctrl.cpp \
    src/iplineedit.cpp

HEADERS += src/mainwindow.h \
    src/BatchProcessor.h \
    3rdparty/libmodbus/src/modbus.h \
    src/serialsettingswidget.h \
    src/imodbus.h \
    src/tcpipsettingswidget.h \
    src/ipaddressctrl.h \
    src/iplineedit.h

INCLUDEPATH += 3rdparty/libmodbus \
               3rdparty/libmodbus/src \
               src

win32 {
    LIBS += -lws2_32
}

FORMS += forms/mainwindow.ui \
    forms/about.ui	\
    forms/BatchProcessor.ui \
    forms/serialsettingswidget.ui \
    forms/tcpipsettingswidget.ui \
    forms/ipaddressctrl.ui

RESOURCES += data/qmodbus.qrc

RC_FILE += qmodbus.rc

include(deployment.pri)
