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

    //_uvShellData.reserve(uvShellCount);

    qDebug() << "Initting UV shells for mesh" << _dagPath.fullPathName().asChar() << "UV shell count" << uvShellCount;

    for (unsigned int i = 0; i < uvShellCount; i++)
    {
        MObject uvs;
        MObject vertices;
        MObject faces;
        MObject edges;
        getMeshUvData(i, uvs, vertices, faces, edges);

        UVData uvData{uvs, vertices, faces, edges};
        //TODO: potentially bug prone if the size of the array does not mach
        _uvShellData << uvData;
        //_uvShellData[i] = uvData;

        emit uvShellAdded(this, i);
    }
}

const MeshData::UVData& MeshData::getUvShell(unsigned int index)
{
    if (index >= _uvShellData.length())
    {
        //This is not good at all but can't think of a better solution that does not involve outright crashing maya
        return UVData{};
    }

    return _uvShellData[index];
}

void MeshData::onMeshAttributeChanged(MNodeMessage::AttributeMessage message, MPlug& plug, MPlug& otherPlug)
{
    MFnMesh mesh(_dagPath);
    qDebug() << "Node message on mesh" << message << plug.name().asChar() << otherPlug.name().asChar() << mesh.numPolygons();

    if (message & MNodeMessage::kAttributeEval) qDebug() << "Attribute eval-" << plug.name().asChar() << otherPlug.name().asChar();
    if (message & MNodeMessage::kAttributeSet ) qDebug() << "Attribute set" << plug.name().asChar() << otherPlug.name().asChar();
    if (message & MNodeMessage::kAttributeAdded) qDebug() << "Attribute added" << plug.name().asChar() << otherPlug.name().asChar();
    if (message & MNodeMessage::kAttributeRemoved ) qDebug() << "Attribute removed" << plug.name().asChar() << otherPlug.name().asChar();
    if (message & MNodeMessage::kAttributeRenamed ) qDebug() << "Attribute renamed"<< plug.name().asChar() << otherPlug.name().asChar();
    if (message & MNodeMessage::kOtherPlugSet )
    {
        qDebug() << "Other plug set" << plug.name().asChar() << "--" << otherPlug.name().asChar();

        if (otherPlug.name().indexW("polyMapCut") >= 0)
        {
            onUvShellCut(otherPlug);
        }
        else if (otherPlug.name().indexW("polyMapSew") >= 0)
        {
            onUvShellSew(otherPlug);
        }
    }

    if (message & MNodeMessage::kAttributeArrayAdded  ) qDebug() << "Attribute array added"<< plug.name().asChar() << otherPlug.name().asChar();
    if (message & MNodeMessage::kConnectionMade  )
    {
        qDebug() << "Attribute connection made";
    }

    //Try init mesh once it has any polygons
    if (!_isMeshInit && mesh.numPolygons() > 0) initUvShells();
}

void MeshData::onUvShellCut(MPlug& cutPlug)
{
    qDebug() << "UV shell cut!";

    MFnDependencyNode node(cutPlug.node());
    MPlug inputComponentsPlug = node.findPlug("inputComponents", false);

    if (inputComponentsPlug.isNull())
    {
        //TODO: Fallback to reset all?
        qDebug() << "Could not find input components plug on cut operation";
        return;
    }

    int oldShellIndex = -1;

    MFnComponentListData componentList(inputComponentsPlug.asMDataHandle().data());
    for (unsigned int i = 0; i < componentList.length(); i++)
    {
        MFnSingleIndexedComponent currentComponent(componentList[i]);
        //TODO: can this be anything else other than edges?
        if (currentComponent.hasObj(MFn::kMeshEdgeComponent))
        {
            MSelectionList cutEdgeList;
            cutEdgeList.add(_dagPath, currentComponent.object());

            for (int shell = 0; shell < _uvShellData.count(); shell++)
            {
                if (cutEdgeList.hasItemPartly(_dagPath, _uvShellData[shell].edges))
                {
                    oldShellIndex = shell;
                    break;
                }
            }
        }
    }

    if (oldShellIndex < 0)
    {
        //TODO: Fallback
        qDebug() << "Could not find which shell was split";
        return;
    }

    //Queue operation and process on topology change
    //This is necessary because the mesh doesn't update until then sob emoji
    _nextOperation = UVOperation{UVOperationType::SPLIT, {oldShellIndex}};
}

void MeshData::onUvShellSew(MPlug& sewPlug)
{
    qDebug() << "UV shell sew!";

    MFnDependencyNode node(sewPlug.node());
    MPlug inputComponentsPlug = node.findPlug("inputComponents", false);

    if (inputComponentsPlug.isNull())
    {
        //TODO: Fallback to reset all?
        qDebug() << "Could not find input components plug on sew operation";
        return;
    }

    QSet<int> oldShellIndices;
    MFnComponentListData componentList(inputComponentsPlug.asMDataHandle().data());
    for (unsigned int i = 0; i < componentList.length(); i++)
    {
        MFnSingleIndexedComponent currentComponent(componentList[i]);
        //TODO: handle stuff other than edges
        if (currentComponent.hasObj(MFn::kMeshEdgeComponent))
        {
            MSelectionList sewnEdgeList;
            sewnEdgeList.add(_dagPath, currentComponent.object());

            for (int shell = 0; shell < _uvShellData.count(); shell++)
            {
                if (sewnEdgeList.hasItemPartly(_dagPath, _uvShellData[shell].edges))
                {
                    oldShellIndices << shell;
                }
            }
        }
    }

    if (oldShellIndices.empty())
    {
        //TODO: fallback
        qDebug() << "Could not find original shells";
        return;
    }

    _nextOperation = UVOperation{UVOperationType::MERGE, oldShellIndices};
}

void MeshData::onTopologyChanged(MObject& node)
{
    qDebug() << "Topology changed";

    //Check for UV shell changes
    MFnMesh mesh(_dagPath);
    MIntArray uvShellIds;
    unsigned int numUvShells;
    mesh.getUvShellsIds(uvShellIds, numUvShells);

    if (numUvShells == _uvShellData.count()) return;

    qDebug() << "UV shell count change!";

    //Figure out which shells changed
    if (_nextOperation.type == UVOperationType::SPLIT)
    {
        int oldShellIndex = _nextOperation.oldShellIndices.values()[0];
        _nextOperation = UVOperation{UVOperationType::NOP, {}};

        //Split the UV shell data
        //Figure out what shell indices the one shell split into

        qDebug() << "New uv shell count" << numUvShells;

        QSet<int> newShellIndices;
        MFnSingleIndexedComponent oldUvIndices(_uvShellData[oldShellIndex].uvs);
        for (int i = 0; i < oldUvIndices.elementCount(); i++)
        {
            newShellIndices << uvShellIds[oldUvIndices.element(i)];
        }

        qDebug() << "Shell" << oldShellIndex << "has been split into shells:";
        for (const int& index : newShellIndices) qDebug() << index;
    }
    else if (_nextOperation.type == UVOperationType::MERGE)
    {
        QSet<int> oldShellIndices = _nextOperation.oldShellIndices;
        _nextOperation = UVOperation{UVOperationType::NOP, {}};

        int newShellIndex = -1;
        for (const int& oldShellIndex : oldShellIndices)
        {
            MFnSingleIndexedComponent oldUvIndices(_uvShellData[oldShellIndex].uvs);
            for (int i = 0; i < oldUvIndices.elementCount(); i++)
            {
                //TODO: Make this not this prone to failure
                newShellIndex = uvShellIds[oldUvIndices.element(i)];
                break;
            }
        }

        qDebug() << "Shells";
        for (const int& index : oldShellIndices) qDebug() << index;
        qDebug() << "have been merged into shell" << newShellIndex;
    }
}

int MeshData::getNumUvsInShell(unsigned int shellIndex)
{
    MFnMesh mesh(_dagPath);

    MIntArray shellIds;
    unsigned int numUvShells;
    mesh.getUvShellsIds(shellIds, numUvShells);

    int uvsInShell = 0;
    for (int uvIndex : shellIds)
    {
        if (uvIndex == shellIndex) uvsInShell++;
    }

    return uvsInShell;
}

void MeshData::getMeshUvData(unsigned int shellIndex, MObject& outUvs, MObject& outVertices, MObject& outFaces, MObject& outEdges)
{
    MFnMesh mesh(_dagPath);
    MIntArray uvShellIndices;
    unsigned int numUvShells;
    mesh.getUvShellsIds(uvShellIndices, numUvShells);

    //Get UVs
    MIntArray uvs;
    for (unsigned int i = 0; i < uvShellIndices.length(); i++)
    {
        if (uvShellIndices[i] == shellIndex) uvs.append(i);
    }

    //Get faces
    QSet<int> faceIndices;
    for (unsigned int f = 0; f < mesh.numPolygons(); f++)
    {
        //This vertex index is local to the polygon
        for (unsigned int v = 0; v < mesh.polygonVertexCount(f); v++)
        {
            int polygonUvId;
            mesh.getPolygonUVid(f, v, polygonUvId);

            for (unsigned int i = 0; i < uvs.length(); i++)
            {
                if (polygonUvId == uvs[i])
                {
                    faceIndices << f;
                }
            }
        }
    }

    //Get vertices
    QSet<int> vertexIndices;
    for (const int& faceIndex : faceIndices)
    {
        MIntArray verts;
        mesh.getPolygonVertices(faceIndex, verts);

        for (const int& vertexIndex : verts) vertexIndices << vertexIndex;
    }

    //Get edges
    QSet<int> edgeIndices;
    for (unsigned int e = 0; e < mesh.numEdges(); e++)
    {
        int2 edge;
        mesh.getEdgeVertices(e, edge);

        int contains = 0;
        for (const int& vertexIndex : vertexIndices)
        {
            if (vertexIndex == edge[0] || vertexIndex == edge[1]) contains++;

            if (contains == 2) break;
        }

        if (contains == 2) edgeIndices << e;
    }

    MIntArray vertexIndices_intArr;
    MIntArray faceIndices_intArr;
    MIntArray edgeIndices_intArr;
    for (const int& vertexIndex : vertexIndices) vertexIndices_intArr.append(vertexIndex);
    for (const int& faceIndex : faceIndices) faceIndices_intArr.append(faceIndex);
    for (const int& edgeIndex : edgeIndices) edgeIndices_intArr.append(edgeIndex);

    MFnSingleIndexedComponent uvComponent;
    MObject uvComponentObj = uvComponent.create(MFn::kMeshMapComponent);
    uvComponent.addElements(uvs);

    MFnSingleIndexedComponent vertexComponent;
    MObject vertexComponentObj = vertexComponent.create(MFn::kMeshVertComponent);
    vertexComponent.addElements(vertexIndices_intArr);

    MFnSingleIndexedComponent faceComponent;
    MObject faceComponentObj = faceComponent.create(MFn::kMeshPolygonComponent);
    faceComponent.addElements(faceIndices_intArr);

    MFnSingleIndexedComponent edgeComponent;
    MObject edgeComponentObj = edgeComponent.create(MFn::kMeshEdgeComponent);
    edgeComponent.addElements(edgeIndices_intArr);

    outUvs = uvComponentObj;
    outVertices = vertexComponentObj;
    outFaces = faceComponentObj;
    outEdges = edgeComponentObj;
}
