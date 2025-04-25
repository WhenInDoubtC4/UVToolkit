#pragma once

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

#include "MeshManager.h"
#include "Global.h"

class LayoutAllCmd : public MPxCommand
{
public:
    LayoutAllCmd();
    virtual ~LayoutAllCmd();

    static void* creator();
    static MSyntax syntax();

    inline static const char kCmdName[] = "layoutAll";

    MStatus doIt(const MArgList& argList);

    inline static const char kRecursiveFlagShortName[] = "-r";
    inline static const char kRecursiveFlagName[] = "-recursive";
};
