#pragma once

#include <QWidget>
#include <QDockWidget>
#include <QDialog>
#include <QTextEdit>
#include <QMainWindow>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MQtUtil.h>

#include "UVOutliner.h"

class UVOutlinerCmd : public MPxCommand
{
public:
    UVOutlinerCmd();

    static void* creator();
    static MSyntax syntax();

    MStatus doIt(const MArgList& argList);

    inline static const char kCmdName[] = "uvOutliner";

    inline static const char kShowWindowFlagShortName[] = "-sw";
    inline static const char kShowWindowFlagName[] = "-showWindow";

private:
    inline static UVOutliner* _window = nullptr;
};
