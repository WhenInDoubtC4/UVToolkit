#pragma once

#include <QDebug>

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MQtUtil.h>

#include "MeshManager.h"

class GroupShellsCmd : public MPxCommand
{
public:
    GroupShellsCmd();

    static void* creator();
    static MSyntax syntax();

    MStatus doIt(const MArgList& argList);

    inline static const char kCmdName[] = "groupUvShells";

    inline static const char kNameArgName[] = "-name";
    inline static const char kNameArgShortName[] = "-n";
};
