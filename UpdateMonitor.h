#pragma once

#include <QDebug>

#include <maya/MDGMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MObject.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>

#include "Global.h"
#include "UVOutlinerCmd.h"

class UpdateMonitor
{
public:
    UpdateMonitor(UpdateMonitor& other) = delete;
    UpdateMonitor(const UpdateMonitor& other) = delete;

    static UpdateMonitor* getInstance();
    static void cleanup();

protected:
    UpdateMonitor();
    ~UpdateMonitor();

private:
    inline static UpdateMonitor* _instance = nullptr;

    inline static MCallbackId _nodeAddedCallbackId;
    inline static MCallbackId _nodeRemovedCallbackId;
    inline static MCallbackId _sceneUpdatedCallbackId;

    void onNodeAdded(MObject& object);
    void onNodeRemoved(MObject& object);
    void onSceneUpdated();
};
