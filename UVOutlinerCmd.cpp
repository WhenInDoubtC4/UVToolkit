#include "UVOutlinerCmd.h"

UVOutlinerCmd::UVOutlinerCmd()
    : MPxCommand()
{

}

void* UVOutlinerCmd::creator()
{
    return new UVOutlinerCmd();
}

MSyntax UVOutlinerCmd::syntax()
{
    MSyntax syntax;

    syntax.addFlag(kShowWindowFlagShortName, kShowWindowFlagName);

    return syntax;
}

MStatus UVOutlinerCmd::doIt(const MArgList& argList)
{
    MArgDatabase argData(syntax(), argList);

    if (argData.isFlagSet(kShowWindowFlagName))
    {
        _window = new UVOutliner();
    }

    return MStatus::kSuccess;
}
