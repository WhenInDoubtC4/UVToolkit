#include "MainWindow.h"
#include "PyScript.h"
#include "EditUVEditorWindowCmd.h"
#include "UVOutlinerCmd.h"
#include "MayaMixin.h"
#include "GroupDataNode.h"
#include "MeshManager.h"
#include "GroupShellsCmd.h"
#include "LayoutAllCmd.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include <maya/MEventMessage.h>
#include <maya/MSceneMessage.h>

// MCallbackId uvEditorOpenCallbackId;
// MCallbackId uvEditorCloseCallbackId;
MCallbackId afterPluginLoadedCallbackId;
MCallbackId beforePluginUnloadedCallbackId;
MCallbackId beforeSaveCallbackId;
MCallbackId afterOpenCallbackId;

bool isPluginBeingUnloaded = false;

//Reroute qDebug() to stdout
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTextStream cout(stdout, QIODevice::WriteOnly);
    std::cout << msg.toStdString() << "\n";
}

QMenu* pluginMenu;

void setupMenuBar()
{
    auto mainWindow = dynamic_cast<QMainWindow*>(MQtUtil::mainWindow());
    if (!mainWindow) return;

    pluginMenu = new QMenu("UV Group Util", MQtUtil::mainWindow());

    auto showOutlinerAction = new QAction("UV Outliner");

    QObject::connect(showOutlinerAction, &QAction::triggered, [=]
    {
        MGlobal::executeCommand(UVOutlinerCmd::kCmdName);
    });

    pluginMenu->addAction(showOutlinerAction);

    mainWindow->menuBar()->addMenu(pluginMenu);
}

void cleanupMenuBar()
{
    delete pluginMenu;
}

MStatus initializePlugin( MObject obj )
{
    isPluginBeingUnloaded = false;

    MFnPlugin plugin(obj, "Adam Gyenes", "1.0", "Any");

    qInstallMessageHandler(myMessageOutput);

    plugin.registerNode(GroupDataNode::typeName, GroupDataNode::typeId, GroupDataNode::creator, GroupDataNode::initialize);

    plugin.registerCommand(MainWindowCmd::kCmdName, MainWindowCmd::creator);
    plugin.registerCommand(EditUVEditorWindowCmd::kCmdName, EditUVEditorWindowCmd::creator, EditUVEditorWindowCmd::syntax);
    plugin.registerCommand(UVOutlinerCmd::kCmdName, UVOutlinerCmd::creator, UVOutlinerCmd::syntax);
    plugin.registerCommand(GroupShellsCmd::kCmdName, GroupShellsCmd::creator, GroupShellsCmd::syntax);
    plugin.registerCommand(LayoutAllCmd::kCmdName, LayoutAllCmd::creator);

    // uvEditorOpenCallbackId = MEventMessage::addEventCallback("texWindowEditorShowup", [](void* data)
    // {
    //     MGlobal::displayInfo("UV editor opened!!");

    //     MGlobal::executeCommand(EditUVEditorWindowCmd::kCmdName, true);
    // });

    // uvEditorCloseCallbackId = MEventMessage::addEventCallback("texWindowEditorClose", [](void* data)
    // {
    //     MGlobal::displayWarning("UV editor closed!!");
    // });

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

    beforeSaveCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeSave, [](void* clientData)
    {
        if (isPluginBeingUnloaded) return;
        //Serialize the group data
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("%1 %2").arg(GroupShellsCmd::kCmdName).arg(GroupShellsCmd::kSerializeFlagName)));
    });

    afterOpenCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, [](void* clientData)
    {
        //Deserialize group data
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("%1 %2").arg(GroupShellsCmd::kCmdName).arg(GroupShellsCmd::kDeserializeFlagName)));
    });

    setupMenuBar();

    return MStatus::kSuccess;
}

MStatus uninitializePlugin( MObject obj )
{
    isPluginBeingUnloaded = true;

    MFnPlugin plugin(obj);

    cleanupMenuBar();

    plugin.deregisterNode(GroupDataNode::typeId);

    plugin.deregisterCommand(MainWindowCmd::kCmdName);
    plugin.deregisterCommand(EditUVEditorWindowCmd::kCmdName);
    plugin.deregisterCommand(UVOutlinerCmd::kCmdName);
    plugin.deregisterCommand(GroupShellsCmd::kCmdName);
    plugin.deregisterCommand(LayoutAllCmd::kCmdName);

    // MMessage::removeCallback(uvEditorOpenCallbackId);
    // MMessage::removeCallback(uvEditorCloseCallbackId);
    MMessage::removeCallback(afterPluginLoadedCallbackId);
    MMessage::removeCallback(beforePluginUnloadedCallbackId);
    MMessage::removeCallback(beforeSaveCallbackId);
    MMessage::removeCallback(afterOpenCallbackId);

    MayaQWidgetDockableMixin::cleanup();
    MainWindowCmd::cleanup();

    return MStatus::kSuccess;
}
