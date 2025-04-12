QT += core gui widgets

TEMPLATE = lib
DEFINES += TESTPLUGIN5_LIBRARY

CONFIG += c++17
CONFIG += qt warn_on release plugin

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEVKIT_LOCATION = "C:/Users/Adam/devkitBase/"
LIBS += -L"$${DEVKIT_LOCATION}lib" -lOpenMaya -lOpenMayaUI -lOpenMayaRender -lFoundation

DEFINES	+= NDEBUG _WINDOWS NT_PLUGIN
INCLUDEPATH += "$${DEVKIT_LOCATION}include"

unix|macx {
  CONFIG += no_plugin_name_prefix
}

_CFLAGS	= /FD /GS
QMAKE_CFLAGS += $${_CFLAGS}
QMAKE_CXXFLAGS += $${_CFLAGS}
TARGET_EXT = .mll

TARGET = TestPlugin5

SOURCES += \
    EditUVEditorWindowCmd.cpp \
    MainWindow.cpp \
    MayaMixin.cpp \
    PyScript.cpp \
    TestPlugin5.cpp \
    UVEditorOverlayWindow.cpp \
    UVOutliner.cpp \
    UVOutlinerCmd.cpp \
    UVTreeWidgetItem.cpp

HEADERS += \
    EditUVEditorWindowCmd.h \
    Global.h \
    MainWindow.h \
    MayaMixin.h \
    PyScript.h \
    UVEditorOverlayWindow.h \
    UVOutliner.h \
    UVOutlinerCmd.h \
    UVTreeWidgetItem.h

FORMS += \
    MainWindow.ui \
    UVEditorOverlayWindow.ui \
    UVOutliner.ui \
    UVTreeWidgetItem.ui

RESOURCES += \
    Scripts.qrc
