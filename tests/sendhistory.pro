QT += core testlib
QT -= gui
CONFIG += console testcase c++17
CONFIG -= app_bundle
TARGET = tst_sendhistory
INCLUDEPATH += ..
SOURCES += tst_sendhistory.cpp ../sendhistorymodel.cpp
HEADERS += ../sendhistorymodel.h
