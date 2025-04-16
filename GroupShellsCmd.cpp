#include "GroupShellsCmd.h"

GroupShellsCmd::GroupShellsCmd()
    : MPxCommand()
{

}

void* GroupShellsCmd::creator()
{
    return new GroupShellsCmd();
}

MSyntax GroupShellsCmd::syntax()
{
    MSyntax syntax;

    syntax.addFlag(kNameArgShortName, kNameArgName, MSyntax::kString);

    return syntax;
}

MStatus GroupShellsCmd::doIt(const MArgList& argList)
{
    MArgDatabase argData(syntax(), argList);

    MSelectionList currentSelection;
    MGlobal::getActiveSelectionList(currentSelection);
\
    if (currentSelection.isEmpty())
    {
        MGlobal::displayInfo("Nothing selected");
        return MStatus::kSuccess;
    }

    QList<MDagPath> pathsToCheck;
    for (MItSelectionList it(currentSelection); !it.isDone(); it.next())
    {
        MDagPath dagPath;
        it.getDagPath(dagPath);

        if (dagPath.hasFn(MFn::kTransform)) dagPath.extendToShape();
        if (!dagPath.hasFn(MFn::kMesh)) continue;

        pathsToCheck << dagPath;
    }

    if (pathsToCheck.empty())
    {
        MGlobal::displayInfo("No elements with UVs selected");
        return MStatus::kSuccess;
    }

    QList<QPair<MeshData*, unsigned int>> shells;
    for (MeshData* meshData : MeshManager::getInst()->getMeshData())
    {
        //Check UV shells only on relevant meshes
        MDagPath meshPath(meshData->getDagPath());
        if (!pathsToCheck.contains(meshPath)) continue;

        for (unsigned int shell = 0; shell < meshData->getNumUvShells(); shell++)
        {
            const MeshData::UVData& uvShell = meshData->getUvShell(shell);

            //Check uvs
            if (currentSelection.hasItem(meshPath, uvShell.uvs) ||
                currentSelection.hasItem(meshPath, uvShell.vertices) ||
                currentSelection.hasItem(meshPath, uvShell.edges) ||
                currentSelection.hasItem(meshPath, uvShell.faces))
            {
                shells << qMakePair(meshData, shell);
            }
        }
    }

    //Do not create an emptry group
    if (shells.empty())
    {
        MGlobal::displayInfo("No UV shells selected");
        return MStatus::kSuccess;
    }

    //Create a new group directly under the world and assign the selected shells to it
    MeshManager::UVGroup* newGroup = MeshManager::getInst()->createGroup();
    for (QPair<MeshData*, unsigned int>& shell : shells)
    {
        newGroup->addUvShell(shell.first, shell.second);
    }

    //Set group name, if applicable
    if (argData.isFlagSet(kNameArgName))
    {
        MString groupName = argData.flagArgumentString(kNameArgName, 0);
        if (groupName.isEmpty()) return MStatus::kInvalidParameter;

        newGroup->setName(MQtUtil::toQString(groupName));
    }

    //Reparent group, if applicable

    qDebug() << QString::fromUtf8(QJsonDocument(newGroup->serialize()).toJson(QJsonDocument::Compact));

    return MStatus::kSuccess;
}
