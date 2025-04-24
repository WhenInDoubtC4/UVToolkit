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
#include <maya/MItMeshPolygon.h>

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

    static MeshData* getMeshData(const MDagPath& path);
    static const QSet<MeshData*>& getMeshData() { return _meshData; };

    static void removeInvalidMeshes();

    void initUvShells();
    void refreshUvShells();
    MString getMeshName() const { return _dagPath.partialPathName(); };
    MDagPath getDagPath() const { return _dagPath; }
    const UVData& getUvShell(unsigned int index);
    unsigned int getNumUvShells() const { return _uvShellData.count(); };

signals:
    void uvShellAdded(MeshData* data, unsigned int uvShellId);
    void uvShellIndexChanged(MeshData* data, unsigned int oldIndex, unsigned int newIndex);
    void uvShellRemoved(MeshData* data, unsigned int index);
    void uvShellSplit(MeshData* data, unsigned int oldShell, const QSet<int>& newIndices);
    void uvDataRefreshed(MeshData* data);

private:
    friend class MeshManager;
    inline static QSet<MeshData*> _meshData;

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
