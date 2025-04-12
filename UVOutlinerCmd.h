#pragma once

#include <QWidget>
#include <QDockWidget>
#include <QDialog>
#include <QTextEdit>
#include <QMainWindow>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>.h>
#include <maya/MQtUtil.h>
#include <maya/MSelectionList.h>
#include <maya/MFnMesh.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MItDag.h>

#include "MayaMixin.h"
#include "UVOutliner.h"

class UVOutlinerCmd : public MPxCommand
{
public:
    UVOutlinerCmd();

    static void* creator();
    static MSyntax syntax();

    MStatus doIt(const MArgList& argList);

    static void cleanup();

    inline static const char kCmdName[] = "uvOutliner";

    inline static const char kShowWindowFlagShortName[] = "-sw";
    inline static const char kShowWindowFlagName[] = "-showWindow";

    inline static const char kAddMeshFlagShortName[] = "-am";
    inline static const char kAddMeshFlagName[] = "-addMesh";

    inline static const char kRemoveMeshFlagShortName[] = "-rm";
    inline static const char kRemoveMeshFlagName[] = "-removeMesh";

private:
    inline static MayaQWidgetDockableMixin* _window = nullptr;

    void buildUVTree(QTreeWidget* treeWidget);

    // void populateOutliner(QTreeWidget* treeWidget);
    // int getUVShellCount(const MFnMesh& meshFn, int uvSetIndex);
    // MIntArray getShellFaceIndices(const MFnMesh& meshFn, int shellIndex, int uvSetIndex);
};
