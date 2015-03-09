#-------------------------------------------------
#
# Project created by QtCreator 2015-03-04T12:32:17
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KegMeterController
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    kegmeter.cpp \
    kegmeterdata.cpp \
    serialsearchandconnectdialog.cpp

HEADERS  += mainwindow.h \
    kegmeter.h \
    kegmeterdata.h \
    serialsearchandconnectdialog.h

FORMS    += mainwindow.ui \
    kegmeter.ui \
    serialsearchandconnectdialog.ui
