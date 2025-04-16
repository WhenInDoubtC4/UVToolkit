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

void UVOutlinerCmd::cleanup()
{
    if (_window) delete _window;
}

MStatus UVOutlinerCmd::doIt(const MArgList& argList)
{
    MArgDatabase argData(syntax(), argList);

    if (argData.isFlagSet(kShowWindowFlagName))
    {
        if (_window)
        {
            _window->show();
            _window->raise();

            //TODO: Do a full refresh

            return MStatus::kSuccess;
        }

        _window = new UVOutliner(MQtUtil::mainWindow());
    }

    return MStatus::kSuccess;
}
