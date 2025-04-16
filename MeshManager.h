#pragma once

#include <QObject>
#include <QDebug>

#include <maya/MSelectionList.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>

#include "MeshData.h"
#include "GroupDataNode.h"

class MeshManager : public QObject
{
    Q_OBJECT
public:
    MeshManager(MeshManager& other) = delete;
    MeshManager(const MeshManager& other) = delete;

    static MeshManager* getInst();
    static void init();
    static void cleanup();

    void addMesh(MeshData* mesh);
    MeshData* removeMeshByName(const MString& name);
    const QSet<MeshData*>& getMeshData() const { return _meshData; };

signals:
    void meshUvShellAdded(MeshData* mesh, unsigned int shellIndex);

protected:
    MeshManager();
    virtual ~MeshManager();

private:
    inline static MeshManager* _instance = nullptr;
    MObject _groupData;
    QSet<MeshData*> _meshData;

    MObject getGroupDataNode();
    void gatherExistingMeshes();

private slots:
    void onUvShellAdded(MeshData* mesh, unsigned int shellIndex);
};
