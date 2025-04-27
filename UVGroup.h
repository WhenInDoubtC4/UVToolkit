#pragma once

#include <limits>

#include <QJsonArray>
#include <QJsonObject>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>

#include "MeshData.h"
#include "Global.h"

class UVGroup
{
public:
    struct AABB
    {
        float xmin = std::numeric_limits<float>::max();
        float xmax = std::numeric_limits<float>::min();
        float ymin = std::numeric_limits<float>::max();
        float ymax = std::numeric_limits<float>::min();

        bool isValid() { return xmax > xmin && ymax > ymin; };
    };

    void addUvShell(MeshData* mesh, unsigned int shellIndex);
    void removeUvShell(MeshData* mesh, unsigned int shellIndex);
    void move(UVGroup* target);
    QJsonObject serialize();

    void setName(const QString& name) { _name = name; };

    const long long getId() const { return _id; };
    QString getName() { return _name; };
    const QList<QPair<MDagPath, unsigned int>>& getShells() const { return _shells; };
    UVGroup* getParent() { return _parent; };
    const QSet<UVGroup*>& getChildren() const { return _children; };

    AABB getAABB() const;
    AABB getAABBRecursive() const;
    MSelectionList getFaces() const;
    MSelectionList getFacesRecursive() const;
    double getUvArea();
    double layout();
    double layoutRecursively();

private:
    friend class MeshManager;
    UVGroup(); //Make constructor private so only MeshManager can create

    long long _id = 0;
    QString _name;
    QList<QPair<MDagPath, unsigned int>> _shells;
    UVGroup* _parent = nullptr;
    QSet<UVGroup*> _children;
};
