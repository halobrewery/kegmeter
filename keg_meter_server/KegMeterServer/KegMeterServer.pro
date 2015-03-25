#-------------------------------------------------
#
# Project created by QtCreator 2015-03-04T12:32:17
#
#-------------------------------------------------

QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KegMeterController
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    kegmeter.cpp \
    serialsearchandconnectdialog.cpp \
    serialcomm.cpp \
    appsettings.cpp \
    kegmeterserver.cpp \
    kegmeterconnection.cpp \
    calibratekegmeterdialog.cpp

HEADERS  += mainwindow.h \
    kegmeter.h \
    serialsearchandconnectdialog.h \
    serialcomm.h \
    abstractcomm.h \
    appsettings.h \
    kegmeterserver.h \
    kegmeterconnection.h \
    calibratekegmeterdialog.h

FORMS    += mainwindow.ui \
    kegmeter.ui \
    serialsearchandconnectdialog.ui \
    calibratekegmeterdialog.ui
