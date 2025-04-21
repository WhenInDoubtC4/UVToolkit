#include "MainWindow.h"
#include "PyScript.h"
#include "EditUVEditorWindowCmd.h"
#include "UVOutlinerCmd.h"
#include "MayaMixin.h"
#include "GroupDataNode.h"
#include "MeshManager.h"
#include "GroupShellsCmd.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include <maya/MEventMessage.h>
#include <maya/MSceneMessage.h>

MCallbackId uvEditorOpenCallbackId;
MCallbackId uvEditorCloseCallbackId;
MCallbackId afterPluginLoadedCallbackId;
MCallbackId beforePluginUnloadedCallbackId;

//Reroute qDebug() to stdout
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTextStream cout(stdout, QIODevice::WriteOnly);
    std::cout << msg.toStdString() << "\n";
}

MStatus initializePlugin( MObject obj )
{
    MFnPlugin plugin(obj, "Adam Gyenes", "1.0", "Any");

    qInstallMessageHandler(myMessageOutput);

    plugin.registerNode(GroupDataNode::typeName, GroupDataNode::typeId, GroupDataNode::creator, GroupDataNode::initialize);

    plugin.registerCommand(MainWindowCmd::kCmdName, MainWindowCmd::creator);
    plugin.registerCommand(EditUVEditorWindowCmd::kCmdName, EditUVEditorWindowCmd::creator, EditUVEditorWindowCmd::syntax);
    plugin.registerCommand(UVOutlinerCmd::kCmdName, UVOutlinerCmd::creator, UVOutlinerCmd::syntax);
    plugin.registerCommand(GroupShellsCmd::kCmdName, GroupShellsCmd::creator, GroupShellsCmd::syntax);

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

    //Init an uninit mesh manager here so that it doesn't cause a race condition when
    afterPluginLoadedCallbackId = MSceneMessage::addStringArrayCallback(MSceneMessage::kAfterPluginLoad, [](const MStringArray& strs, void* clientData)
    {
        MString pluginName = strs[1];
        qDebug() << "Load plugin name:" << pluginName.asChar();

        if (pluginName != PROJECT_NAME) return;

        MeshManager::init();
    });

    beforePluginUnloadedCallbackId = MSceneMessage::addStringArrayCallback(MSceneMessage::kBeforePluginUnload, [](const MStringArray& strs, void* clientData)
    {
        MString pluginName = strs[0];
        qDebug() << "Unload plugin name:" << pluginName.asChar();

        if (pluginName != PROJECT_NAME) return;

        MeshManager::cleanup();
    });

    int result;
    MGlobal::executeCommand("workspaceControl -q -exists polyTexturePlacementPanel1Window", result);
    qDebug() << "REsult is " << result;

    return MStatus::kSuccess;
}

MStatus uninitializePlugin( MObject obj )
{
    MFnPlugin plugin(obj);

    plugin.deregisterNode(GroupDataNode::typeId);

    plugin.deregisterCommand(MainWindowCmd::kCmdName);
    plugin.deregisterCommand(EditUVEditorWindowCmd::kCmdName);
    plugin.deregisterCommand(UVOutlinerCmd::kCmdName);
    plugin.deregisterCommand(GroupShellsCmd::kCmdName);

    MMessage::removeCallback(uvEditorOpenCallbackId);
    MMessage::removeCallback(uvEditorCloseCallbackId);
    MMessage::removeCallback(afterPluginLoadedCallbackId);
    MMessage::removeCallback(beforePluginUnloadedCallbackId);

    MayaQWidgetDockableMixin::cleanup();
    MainWindowCmd::cleanup();

    return MStatus::kSuccess;
}
