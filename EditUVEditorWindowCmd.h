#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QStackedLayout>
#include <QEvent>
#include <QStackedWidget>

#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

#include "UVEditorOverlayWindow.h"
#include "Global.h"

class EditUVEditorWindowCmd : public MPxCommand
{
public:
    EditUVEditorWindowCmd();

    static void* creator();
    static MSyntax syntax();

    MStatus doIt(const MArgList& argList);

    inline static const char kCmdName[] = "editUVEditorWindow";
private:
};

class OverlayEventFilter : public QObject
{
public:
    OverlayEventFilter(QWidget* stackedWidget, UVEditorOverlayWindow* mdiArea);

    bool eventFilter(QObject* watched, QEvent* event);

private:
    QWidget* _stackedWidget;
    UVEditorOverlayWindow* _mdiArea;
    QWidget* _parentWindow;
};
