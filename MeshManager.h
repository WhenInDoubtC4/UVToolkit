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
#include "GroupDataNode.h"

class MeshManager : public QObject
{
    Q_OBJECT
public:
    MeshManager(MeshManager& other) = delete;
    MeshManager(const MeshManager& other) = delete;

    struct UVGroup
    {
    public:
        void addUvShell(MeshData* mesh, unsigned int shellIndex);
        void removeUvShell(MeshData* mesh, unsigned int shellIndex);
        QJsonObject serialize();

        void setName(const QString& name) { _name = name; };

        const long long getId() const { return _id; };
        QString getName() { return _name; };
        const QList<QPair<MDagPath, unsigned int>>& getShells() const { return _shells; };
        UVGroup* getParent() { return _parent; };

    private:
        friend class MeshManager;

        long long _id = 0;
        QString _name;
        QList<QPair<MDagPath, unsigned int>> _shells;
        UVGroup* _parent;
        QSet<UVGroup*> _children;
    };

    static MeshManager* getInst();
    static void init();
    static void cleanup();

    void addMesh(MeshData* mesh);
    MeshData* removeMeshByName(const MString& name);
    const QSet<MeshData*>& getMeshData() const { return _meshData; };

    UVGroup* createGroup(UVGroup* parent = nullptr);
    void deleteGroup(UVGroup* group);

    QJsonDocument serializeGroupData();

    UVGroup* getShellGroup(MeshData* mesh, unsigned int shellindex) const;

signals:
    void meshUvShellAdded(MeshData* mesh, unsigned int shellIndex);
    void meshUvShellIndexChanged(MeshData* data, unsigned int oldIndex, unsigned int newIndex);
    void meshUvShellRemoved(MeshData* mesh, unsigned int index);
    void groupCreated(UVGroup* group);
    void uvShellAddedToGroup(UVGroup* group, MeshData* mesh, unsigned int shellIndex);
    void uvShellRemovedFromGroup(UVGroup* group, MeshData* mesh, unsigned int shellIndex);

protected:
    MeshManager();
    virtual ~MeshManager();

private:
    inline static MeshManager* _instance = nullptr;
    MObject _groupData;
    QSet<MeshData*> _meshData;
    UVGroup* _groupDataRoot = nullptr;
    inline static long long _nextGroupId = 0;

    MObject getGroupDataNode();
    void gatherExistingMeshes();
    void clearGroupData(UVGroup* root);
    UVGroup* getShellGroup_impl(MeshData* mesh, unsigned int shellindex, UVGroup* root) const;

private slots:
    void onUvShellAdded(MeshData* mesh, unsigned int shellIndex);
    void onUvShellIndexChanged(MeshData* mesh, unsigned int oldIndex, unsigned int newIndex);
    void onUvShellRemoved(MeshData* mesh, unsigned int index);
    void onUvShellSplit(MeshData* mesh, unsigned int oldShell, const QSet<int>& newIndices);
};
