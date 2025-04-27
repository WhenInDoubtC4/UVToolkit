QT += core gui widgets

TEMPLATE = lib
DEFINES += UVGROUPUTIL_LIBRARY

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

TARGET = UVGroupUtil
DEFINES += PROJECT_NAME=\\\"$${TARGET}\\\"

#Copy .mll to the plugins folder
copydata.commands = $(COPY_DIR) $$shell_path($$OUT_PWD/release/$${TARGET}$${TARGET_EXT}) $$shell_path($${DEVKIT_LOCATION}plug-ins/plug-ins)
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

SOURCES += \
    EditUVEditorWindowCmd.cpp \
    GroupDataNode.cpp \
    GroupShellsCmd.cpp \
    GroupTreeWidgetItem.cpp \
    LayoutAllCmd.cpp \
    MainWindow.cpp \
    MayaMixin.cpp \
    MeshData.cpp \
    MeshManager.cpp \
    PyScript.cpp \
    ShellTreeWidgetItem.cpp \
    Plugin.cpp \
    UVEditorOverlayWindow.cpp \
    UVGroup.cpp \
    UVOutliner.cpp \
    UVOutlinerCmd.cpp \
    UVTreeWidget.cpp \
    UVTreeWidgetItem.cpp

HEADERS += \
    EditUVEditorWindowCmd.h \
    Global.h \
    GroupDataNode.h \
    GroupShellsCmd.h \
    GroupTreeWidgetItem.h \
    LayoutAllCmd.h \
    MainWindow.h \
    MayaMixin.h \
    MeshData.h \
    MeshManager.h \
    PyScript.h \
    ShellTreeWidgetItem.h \
    UVEditorOverlayWindow.h \
    UVGroup.h \
    UVOutliner.h \
    UVOutlinerCmd.h \
    UVTreeWidget.h \
    UVTreeWidgetItem.h

FORMS += \
    GroupTreeWidgetItem.ui \
    MainWindow.ui \
    ShellTreeWidgetItem.ui \
    UVEditorOverlayWindow.ui \
    UVOutliner.ui

RESOURCES += \
    Icons.qrc \
    Scripts.qrc
