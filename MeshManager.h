#pragma once

#include <QObject>
#include <QDebug>
#include <QPair>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <maya/MSelectionList.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>

#include "MeshData.h"
#include "UVGroup.h"
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

    UVGroup* getRootGroup() const { return _groupDataRoot; };
    UVGroup* createGroup(UVGroup* parent = nullptr);
    QList<UVGroup*> getTopLevelGroups();
    void deleteGroup(UVGroup* group);
    void addUntrackedMeshes();
    void readdExistingGroups();

    QJsonDocument serializeGroupData();

    UVGroup* getShellGroup(MeshData* mesh, unsigned int shellindex) const;
    void removeInvalidUvShells(MeshData* mesh);

signals:
    void meshUvShellAdded(MeshData* mesh, unsigned int shellIndex);
    void meshUvShellIndexChanged(MeshData* data, unsigned int oldIndex, unsigned int newIndex);
    void meshUvShellRemoved(MeshData* mesh, unsigned int index);
    void meshUvDataRefreshed(MeshData* mesh);
    void groupCreated(UVGroup* group);
    void uvShellAddedToGroup(UVGroup* group, MeshData* mesh, unsigned int shellIndex);
    void uvShellRemovedFromGroup(UVGroup* group, MeshData* mesh, unsigned int shellIndex);

protected:
    MeshManager();
    virtual ~MeshManager();

private:
    inline static MeshManager* _instance = nullptr;
    MObject _groupData;
    UVGroup* _groupDataRoot = nullptr;
    inline static long long _nextGroupId = 0;

    MObject getGroupDataNode();
    void gatherExistingMeshes();
    void clearGroupData(UVGroup* root);
    UVGroup* getShellGroup_impl(MeshData* mesh, unsigned int shellindex, UVGroup* root) const;
    void removeInvalidUvShells_impl(MeshData* mesh, UVGroup* root);
    void readdExistingGroups_impl(UVGroup* root);

private slots:
    void onUvShellAdded(MeshData* mesh, unsigned int shellIndex);
    void onUvShellIndexChanged(MeshData* mesh, unsigned int oldIndex, unsigned int newIndex);
    void onUvShellRemoved(MeshData* mesh, unsigned int index);
    void onUvShellSplit(MeshData* mesh, unsigned int oldShell, const QSet<int>& newIndices);
    void onUvDataRefreshed(MeshData* mesh);
};

