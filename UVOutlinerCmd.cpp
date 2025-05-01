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

    return syntax;
}

MStatus UVOutlinerCmd::doIt(const MArgList& argList)
{
    MArgDatabase argData(syntax(), argList);

    _window = new UVOutliner();

    return MStatus::kSuccess;
}
