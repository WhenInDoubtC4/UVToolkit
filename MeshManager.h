#pragma once

#include <QObject>
#include <QDebug>

#include <maya/MSelectionList.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>

#include "GroupDataNode.h"

class MeshManager : public QObject
{
    Q_OBJECT
public:
    MeshManager(MeshManager& other) = delete;
    MeshManager(const MeshManager& other) = delete;

    MeshManager* getInst();
    static void init();
    static void cleanup();

protected:
    MeshManager();
    virtual ~MeshManager();

private:
    inline static MeshManager* _instance = nullptr;
    MObject _groupData;

    MObject getGroupDataNode();
};
