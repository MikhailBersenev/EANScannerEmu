QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cappsettings.cpp \
    cinfomessagehandler.cpp \
    cstringsender.cpp \
    cstringsenderlinuxwayland.cpp \
    cstringsenderlinuxx11.cpp \
    cstringsenderwin32.cpp \
    cstringsendermac.cpp \
    cutils.cpp \
    generatedialog.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    cappsettings.h \
    cinfomessagehandler.h \
    cstringsender.h \
    cstringsenderlinuxwayland.h \
    cstringsenderlinuxx11.h \
    cstringsenderwin32.h \
    cstringsendermac.h \
    cutils.h \
    generatedialog.h \
    mainwindow.h

FORMS += \
    generatedialog.ui \
    mainwindow.ui

# TRANSLATIONS += \
#     EANScannerEmu_en_US.ts
# CONFIG += lrelease
# CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

DISTFILES += \
    sound.wav
unix:!macx: {
    LIBS += -lX11 -lXtst
}
macx: {
    LIBS += -framework ApplicationServices -framework Carbon
}
RC_FILE = res.rc
