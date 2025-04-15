#pragma once

#include <QDebug>
#include <QObject>

#include <maya/MDagPath.h>
#include <maya/MString.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPolyMessage.h>

#include "UVTreeWidgetItem.h"
#include "Global.h"

class MeshData : public QObject
{
    Q_OBJECT
public:
    MeshData(const MDagPath& dagPath);
    ~MeshData();

    void initUvShells();
    MString getMeshName() const { return _dagPath.partialPathName(); };
    MDagPath getDagPath() const { return _dagPath; }
    void removeFromUi();

signals:
    void uvShellAdded(MDagPath& mesh, unsigned int uvShellId);

private:
    MDagPath _dagPath;
    MString _name;
    bool _isMeshInit = false;

    MCallbackId _attributeChangedCallbackId;
    MCallbackId _topologyChangedCallbackId;

    void onMeshAttributeChanged(MNodeMessage::AttributeMessage message, MPlug& plug, MPlug& otherPlug);
    void onTopologyChanged(MObject& node);
};
