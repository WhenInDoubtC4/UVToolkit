#include "MainWindow.h"
#include "PyScript.h"
#include "EditUVEditorWindowCmd.h"
#include "UVOutlinerCmd.h"
#include "MayaMixin.h"
#include "UpdateMonitor.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include <maya/MEventMessage.h>

MCallbackId uvEditorOpenCallbackId;
MCallbackId uvEditorCloseCallbackId;

//Reroute qDebug() to stdout
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTextStream cout(stdout, QIODevice::WriteOnly);
    std::cout << msg.toStdString() << "\n";
}

MStatus initializePlugin( MObject obj )
{
    MFnPlugin plugin(obj, "Adam Gyenes", "1.0", "Any");

    plugin.registerCommand(MainWindowCmd::kCmdName, MainWindowCmd::creator);
    plugin.registerCommand(EditUVEditorWindowCmd::kCmdName, EditUVEditorWindowCmd::creator, EditUVEditorWindowCmd::syntax);
    plugin.registerCommand(UVOutlinerCmd::kCmdName, UVOutlinerCmd::creator, UVOutlinerCmd::syntax);

    //Run init plugin script
    // PyScript script(":/onInitPlugin.py");
    // MGlobal::executePythonCommand(script.getMString());

    // script.setGlobal("GLOVBAL", "meow");
    // script.setGlobal("TEST1", 69);
    // script.setGlobal("TEST2", true);

    uvEditorOpenCallbackId = MEventMessage::addEventCallback("texWindowEditorShowup", [](void* data)
    {
        MGlobal::displayInfo("UV editor opened!!");

        MGlobal::executeCommand(EditUVEditorWindowCmd::kCmdName, true);
    });

    uvEditorCloseCallbackId = MEventMessage::addEventCallback("texWindowEditorClose", [](void* data)
    {
        MGlobal::displayWarning("UV editor closed!!");
    });

    qInstallMessageHandler(myMessageOutput);

    UpdateMonitor::getInstance();

    int result;
    MGlobal::executeCommand("workspaceControl -q -exists polyTexturePlacementPanel1Window", result);
    qDebug() << "REsult is " << result;

    return MStatus::kSuccess;
}

MStatus uninitializePlugin( MObject obj )
{
    MFnPlugin plugin(obj);

    plugin.deregisterCommand(MainWindowCmd::kCmdName);
    plugin.deregisterCommand(EditUVEditorWindowCmd::kCmdName);
    plugin.deregisterCommand(UVOutlinerCmd::kCmdName);

    MEventMessage::removeCallback(uvEditorOpenCallbackId);
    MEventMessage::removeCallback(uvEditorCloseCallbackId);

    MayaQWidgetDockableMixin::cleanup();
    MainWindowCmd::cleanup();
    UVOutlinerCmd::cleanup();

    UpdateMonitor::cleanup();

    return MStatus::kSuccess;
}
