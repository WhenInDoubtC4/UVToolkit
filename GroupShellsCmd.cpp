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
    syntax.addFlag(kSerializeFlagShortName, kSerializeFlagName);
    syntax.addFlag(kDeserializeFlagShortName, kDeserializeFlagName);

    return syntax;
}

MStatus GroupShellsCmd::doIt(const MArgList& argList)
{
    MArgDatabase argData(syntax(), argList);

    if (argData.isFlagSet(kSerializeFlagName))
    {
        return serializeGroupData();
    }
    else if (argData.isFlagSet(kDeserializeFlagName))
    {
        return deserializeGroupData();
    }

    /////////////////////////////////////////////////////
    /// Grouping

    MSelectionList currentSelection;
    MGlobal::getActiveSelectionList(currentSelection);

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
    for (MeshData* meshData : MeshData::getMeshData())
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

    //Find out what groups the shells belong to, if at all
    QSet<UVGroup*> shellGroups;
    for (QPair<MeshData*, unsigned int>& shell : shells)
    {
        UVGroup* shellGroup = MeshManager::getInst()->getShellGroup(shell.first, shell.second);
        shellGroups << shellGroup;
    }

    UVGroup* newGroup;
    if (shellGroups.count() == 1)
    {
        //Scenario 1/3: None of the shells belong to any group: Create a new group under the world and add shells
        if (shellGroups.contains(nullptr))
        {
            qDebug() << "Creating new group under the world";
            newGroup = MeshManager::getInst()->createGroup();
        }
        else
        //Scenario 2/3: All shells belong to the same group: Create a group nested within the original group
        {
            qDebug() << "Creating nested group";
            //Ungroup all shells first
            UVGroup* oldGroup = *shellGroups.begin();
            for (QPair<MeshData*, unsigned int>& shell : shells)
            {
                oldGroup->removeUvShell(shell.first, shell.second);
            }

            //Create a new group parented to the old group
            newGroup = MeshManager::getInst()->createGroup(oldGroup);
        }
    }
    //Scenario 3/3: The shells belong to different groups: Remove shells from their respecive existing groups (if applicable), create a new group under the world and add them
    else
    {
        qDebug() << "Removing shells from their existing groups";
        //Ungroup all shells
        for (QPair<MeshData*, unsigned int>& shell : shells)
        {
            UVGroup* oldGroup = MeshManager::getInst()->getShellGroup(shell.first, shell.second);
            if (!oldGroup) continue;

            oldGroup->removeUvShell(shell.first, shell.second);
        }

        //Create a new group under the world
        newGroup = MeshManager::getInst()->createGroup();
    }

    qDebug() << "Adding shells to group...";

    //Assign the selected shells to the group (they should all be ungrouped at this point)
    for (QPair<MeshData*, unsigned int>& shell : shells)
    {
        qDebug() << shell.first->getDagPath().fullPathName().asChar() << "shell" << shell.second;
        newGroup->addUvShell(shell.first, shell.second);
    }

    //Set group name, if applicable
    if (argData.isFlagSet(kNameArgName))
    {
        MString groupName = argData.flagArgumentString(kNameArgName, 0);
        if (groupName.isEmpty()) return MStatus::kInvalidParameter;

        newGroup->setName(MQtUtil::toQString(groupName));
    }

    //qDebug() << QString::fromUtf8(MeshManager::getInst()->serializeGroupData().toJson(QJsonDocument::Compact));

    return MStatus::kSuccess;
}

MStatus GroupShellsCmd::serializeGroupData()
{
    qDebug() << "Serializing";

    QJsonDocument groupDataJson = MeshManager::getInst()->serializeGroupData();
    if (groupDataJson.isEmpty() || groupDataJson.isNull())
    {
        MGlobal::displayError("Failed to serialize group data");
        return MStatus::kFailure;
    }

    MString serializedData = groupDataJson.toJson(QJsonDocument::Compact).constData();
    qDebug() << groupDataJson.toJson(QJsonDocument::Compact);


    MObject dataNode = MeshManager::getInst()->getGroupDataNode();
    MFnDependencyNode depNode(dataNode);
    MPlug attribute = depNode.findPlug(GroupDataNode::kAttributeName, false);
    attribute.setValue(serializedData);

    return MStatus::kSuccess;
}

MStatus GroupShellsCmd::deserializeGroupData()
{
    qDebug() << "Deserializing";

    MObject dataNode = MeshManager::getInst()->getGroupDataNode();
    MFnDependencyNode depNode(dataNode);
    MPlug attribute = depNode.findPlug(GroupDataNode::kAttributeName, false);

    MString serializedData;
    attribute.getValue(serializedData);
    qDebug() << "Attribute value:" << serializedData.asChar();

    QJsonDocument groupJson = QJsonDocument::fromJson(MQtUtil::toQString(serializedData).toUtf8());
    if (groupJson.isNull() || groupJson.isEmpty())
    {
        MGlobal::displayError("Invalid json group data");
        return MStatus::kFailure;
    }

    //Make sure mesh data is up to date before creating the groups
    MeshManager::getInst()->addUntrackedMeshes();

    MeshManager::getInst()->createGroupFromJsonRecursive(groupJson.object());

    return MStatus::kSuccess;
}
