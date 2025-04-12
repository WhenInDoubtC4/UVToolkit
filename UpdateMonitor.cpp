#include "UpdateMonitor.h"

UpdateMonitor::UpdateMonitor()
{
    //Whenever a new node is added to the dependency graph
    _nodeAddedCallbackId = MDGMessage::addNodeAddedCallback(MNodeFunction_wrapper<&UpdateMonitor::onNodeAdded>, "transform", this);

    //Whenever a new node is removed from the dependency graph
    _nodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(MNodeFunction_wrapper<&UpdateMonitor::onNodeRemoved>, "transform", this);

    //Called after any operation that changes which files are loaded
    _sceneUpdatedCallbackId = MSceneMessage::addCallback(MSceneMessage::kSceneUpdate, MBasicFunction_wrapper<&UpdateMonitor::onSceneUpdated>, this);
}

UpdateMonitor::~UpdateMonitor()
{

}

UpdateMonitor* UpdateMonitor::getInstance()
{
    if (!_instance) return new UpdateMonitor();
    return _instance;
}

void UpdateMonitor::cleanup()
{
    MMessage::removeCallback(_nodeAddedCallbackId);
    MMessage::removeCallback(_nodeRemovedCallbackId);

    MMessage::removeCallback(_sceneUpdatedCallbackId);

    if (_instance) delete _instance;
}

void UpdateMonitor::onNodeAdded(MObject& object)
{
    if (object.isNull() || !object.hasFn(MFn::kDagNode)) return;

    MFnDagNode node(object);
    if (!node.hasObj(MFn::kMesh)) return;

    //Call add command on the UV outliner
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("%1 %2 %3").arg(UVOutlinerCmd::kCmdName, UVOutlinerCmd::kAddMeshFlagName, node.fullPathName().asChar())));
}

void UpdateMonitor::onNodeRemoved(MObject& object)
{
    if (object.isNull() || !object.hasFn(MFn::kDagNode)) return;

    MFnDagNode node(object);
    if (!node.hasObj(MFn::kMesh)) return;

    //Cannot get the dag path because the mesh is being deleted
    MString meshName = node.name();
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("%1 %2 %3").arg(UVOutlinerCmd::kCmdName, UVOutlinerCmd::kRemoveMeshFlagName, meshName.asChar())));
}

void UpdateMonitor::onSceneUpdated()
{
    qDebug() << "Scene updated";

    //Refresh everything
}
