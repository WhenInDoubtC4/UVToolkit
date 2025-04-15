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

        initMeshData();
        //buildUVTree(_window->getTreeWidget());
    }

    return MStatus::kSuccess;
}

void UVOutlinerCmd::initMeshData()
{
    for(MItDag it(MItDag::TraversalType::kBreadthFirst, MFn::kTransform); !it.isDone(); it.next())
    {
        //Filter meshes
        MDagPath path;
        if (it.getPath(path) != MStatus::kSuccess)
        {
            MGlobal::displayError("Cannot get dag path for " + it.fullPathName());
            continue;
        }
        if (!path.hasFn(MFn::kMesh)) continue;

        //Create top level item for the mesh
        MFnMesh mesh(path);

        _window->addMesh(new MeshData(path));
    }
}

// void UVOutlinerCmd::buildUVTree(QTreeWidget* treeWidget)
// {
//     qDebug() << "All meshes:";
//     for(MItDag it(MItDag::TraversalType::kBreadthFirst, MFn::kTransform); !it.isDone(); it.next())
//     {
//         //Filter meshes
//         MDagPath path;
//         if (it.getPath(path) != MStatus::kSuccess)
//         {
//             MGlobal::displayError("Cannot get dag path for " + it.fullPathName());
//             continue;
//         }
//         if (!path.hasFn(MFn::kMesh)) continue;

//         //Create top level item for the mesh
//         MFnMesh mesh(path);

//         //TODO: Multi uv set support
//         MIntArray uvShellIds;
//         unsigned int uvShellCount;
//         mesh.getUvShellsIds(uvShellIds, uvShellCount);

//         for (unsigned int i = 0; i < uvShellCount; i++)
//         {
//             dynamic_cast<UVOutliner*>(_window)->addItem(path, i);
//         }
//     }
// }

