QT += core gui widgets

TARGET = ImageFilter
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    filter2d.cpp \
    imageinfowidget.cpp \
    mainwindow.cpp

HEADERS += \
    filter2d.h \
    imageinfowidget.h \
    mainwindow.h

QMAKE_CXXFLAGS += -Wall -Wextra

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

unix:!macx {
    target.path = /usr/local/bin
    INSTALLS += target
}
