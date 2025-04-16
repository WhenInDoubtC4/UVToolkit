#pragma once

#include <QDebug>
#include <QObject>
#include <QHash>

#include <maya/MDagPath.h>
#include <maya/MString.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPolyMessage.h>
#include <maya/MFnAttribute.h>
#include <maya/MPlugArray.h>
#include <maya/MDataHandle.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MSelectionList.h>

#include "Global.h"

class MeshData : public QObject
{
    Q_OBJECT
public:
    MeshData(const MDagPath& dagPath);
    ~MeshData();

    struct UVData
    {
        MObject uvs;
        MObject vertices;
        MObject faces;
        MObject edges;
    };

    void initUvShells();
    MString getMeshName() const { return _dagPath.partialPathName(); };
    MDagPath getDagPath() const { return _dagPath; }
    const UVData& getUvShell(unsigned int index);

signals:
    void uvShellAdded(MeshData* data, unsigned int uvShellId);

private:
    MDagPath _dagPath;
    bool _isMeshInit = false;
    QList<UVData> _uvShellData;

    MCallbackId _attributeChangedCallbackId;
    MCallbackId _topologyChangedCallbackId;

    void onMeshAttributeChanged(MNodeMessage::AttributeMessage message, MPlug& plug, MPlug& otherPlug);
    void onTopologyChanged(MObject& node);

    void onUvShellCut(MPlug& cutPlug);
    void onUvShellSew(MPlug& sewPlug);

    int getNumUvsInShell(unsigned int shellIndex);

    void getMeshUvData(unsigned int shellIndex, MObject& outUvs, MObject& outVertices, MObject& outFaces, MObject& outEdges);

    enum UVOperationType
    {
        NOP = -1,
        SPLIT,
        MERGE
    };

    struct UVOperation
    {
        UVOperationType type = UVOperationType::NOP;
        QSet<int> oldShellIndices;
    };

    UVOperation _nextOperation;
};
