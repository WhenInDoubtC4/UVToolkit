#include "MeshData.h"

MeshData::MeshData(const MDagPath& dagPath)
    : QObject()
    , _dagPath(dagPath)
{
    _dagPath.extendToShape();

    //Setup callbacks for later updates
    MObject meshNode = _dagPath.node();
    _attributeChangedCallbackId = MNodeMessage::addAttributeChangedCallback(meshNode, MAttr2PlugFunction_wrapper<&MeshData::onMeshAttributeChanged>, this);
    _topologyChangedCallbackId = MPolyMessage::addPolyTopologyChangedCallback(meshNode, MNodeFunction_wrapper<&MeshData::onTopologyChanged>, this);

    qDebug() << "Creating mesh data object for" << _dagPath.fullPathName().asChar();
}

MeshData::~MeshData()
{
    qDebug() << "Deleting mesh data";

    MMessage::removeCallback(_attributeChangedCallbackId);
    MMessage::removeCallback(_topologyChangedCallbackId);
}

void MeshData::initUvShells()
{
    //Check if the mesh has been initialized yet
    MFnMesh mesh(_dagPath);
    if (mesh.numPolygons() == 0) return;

    _isMeshInit = true;

    //Add UV shells to the tree widget
    MIntArray uvShellsIds;
    unsigned int uvShellCount;
    mesh.getUvShellsIds(uvShellsIds, uvShellCount);

    qDebug() << "Initting UV shells for mesh" << _dagPath.fullPathName().asChar() << "UV shell count" << uvShellCount;

    for (unsigned int i = 0; i < uvShellCount; i++)
    {
        emit uvShellAdded(_dagPath, i);
    }
}

void MeshData::onMeshAttributeChanged(MNodeMessage::AttributeMessage message, MPlug& plug, MPlug& otherPlug)
{
    MFnMesh mesh(_dagPath);
    qDebug() << "Node message on mesh" << message << plug.name().asChar() << otherPlug.name().asChar() << mesh.numPolygons();

    if (message & MNodeMessage::kAttributeEval) qDebug() << "Attribute eval-";
    if (message & MNodeMessage::kAttributeSet ) qDebug() << "Attribute set";
    if (message & MNodeMessage::kAttributeAdded) qDebug() << "Attribute added";
    if (message & MNodeMessage::kAttributeRemoved ) qDebug() << "Attribute removed";
    if (message & MNodeMessage::kAttributeRenamed ) qDebug() << "Attribute renamed";
    if (message & MNodeMessage::kOtherPlugSet )
    {
        qDebug() << "Attribute plug set";
    }

    if (message & MNodeMessage::kAttributeArrayAdded  ) qDebug() << "Attribute array added";
    if (message & MNodeMessage::kConnectionMade  ) qDebug() << "Attribute connection made";

    //Try init mesh once it has any polygons
    if (!_isMeshInit && mesh.numPolygons() > 0) initUvShells();
}

void MeshData::onTopologyChanged(MObject& node)
{
    qDebug() << "Topology changed";
}
