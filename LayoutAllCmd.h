#pragma once

#include <maya/MPxCommand.h>

#include "MeshManager.h"
#include "Global.h"

class LayoutAllCmd : public MPxCommand
{
public:
    LayoutAllCmd();
    virtual ~LayoutAllCmd();

    static void* creator();

    inline static const char kCmdName[] = "layoutAll";

    MStatus doIt(const MArgList& argList);
};
