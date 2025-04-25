#include "UVGroup.h"

UVGroup::UVGroup() {}

QJsonObject UVGroup::serialize()
{
    QJsonObject result;

    result["id"] = static_cast<qint64>(_id);
    result["name"] = _name;

    QJsonArray shellArray;
    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        QJsonObject shellObject;
        shellObject["path"] = shell.first.fullPathName().asChar();
        shellObject["shell"] = static_cast<qint64>(shell.second);

        shellArray << shellObject;
    }

    result["shells"] = shellArray;
    //Do this because of the root
    result["parent"] = _parent ? static_cast<qint64>(_parent->_id) : -1;

    QJsonArray childArray;
    for (UVGroup* childGroup : _children)
    {
        childArray << childGroup->serialize();
    }

    result["children"] = childArray;

    return result;
}

UVGroup::AABB UVGroup::getAABB() const
{
    AABB result;

    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        MObject faces = MeshData::getMeshData(shell.first)->getUvShell(shell.second).faces;
        for (MItMeshPolygon it(shell.first, faces); !it.isDone(); it.next())
        {
            for (unsigned int v = 0; v < it.polygonVertexCount(); v++)
            {
                float uvPoint[2];
                it.getUV(v, uvPoint);

                if (uvPoint[0] < result.xmin) result.xmin = uvPoint[0];
                if (uvPoint[0] > result.xmax) result.xmax = uvPoint[0];

                if (uvPoint[1] < result.ymin) result.ymin = uvPoint[1];
                if (uvPoint[1] > result.ymax) result.ymax = uvPoint[1];
            }
        }
    }

    return result;
}

UVGroup::AABB UVGroup::getAABBRecursive() const
{
    AABB result = getAABB();

    for (UVGroup* childGroup : _children)
    {
        AABB childAabb = childGroup->getAABBRecursive();

        result.xmin = std::min(result.xmin, childAabb.xmin);
        result.ymin = std::min(result.ymin, childAabb.ymin);
        result.xmax = std::max(result.xmax, childAabb.xmax);
        result.ymax = std::max(result.ymax, childAabb.ymax);
    }

    return result;
}

double UVGroup::getUvArea()
{
    double result = -1.;

    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        MeshData::UVData shellData = MeshData::getMeshData(shell.first)->getUvShell(shell.second);
        for (MItMeshPolygon it(shell.first, shellData.faces); !it.isDone(); it.next())
        {
            if (it.zeroUVArea()) continue;

            it.getUVArea(result);
            return result;
        }
    }

    return result;
}

MSelectionList UVGroup::getFaces() const
{
    MSelectionList result;

    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        MeshData* mesh = MeshData::getMeshData(shell.first);
        if (!mesh) continue;

        MObject faces = mesh->getUvShell(shell.second).faces;
        result.add(shell.first, faces);
    }

    return result;
}

MSelectionList UVGroup::getFacesRecursive() const
{
    MSelectionList result = getFaces();

    for (UVGroup* childGroup : _children)
    {
        result.merge(childGroup->getFacesRecursive());
    }

    return result;
}

double UVGroup::layout()
{
    qDebug() << "Group layout exec";

    //Layout the group and its contents ONLY

    //Store existing scaling setting
    int prevShellScalingSetting = MGlobal::optionVarIntValue(OptionVars::SHELL_PRE_SCALING);
    //Disable shell pre-scaling
    MGlobal::setOptionVarValue(OptionVars::SHELL_PRE_SCALING, 1);

    //Build a selection list for the group
    MSelectionList groupSelection;
    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        MeshData* data = MeshData::getMeshData(shell.first);
        if (!data) continue;

        MObject facesInShell = data->getUvShell(shell.second).faces;
        groupSelection.add(shell.first, facesInShell);
    }

    //Get the UV area of an arbitrary shell in the group
    double initialArea = getUvArea();
    if (initialArea <= 0.)
    {
        MGlobal::displayError("Shells in the group have an invalid area. If this issue persists do a layout manually first");
        return -1.;
    }

    //Layout group only
    MGlobal::clearSelectionList();
    MGlobal::setActiveSelectionList(groupSelection);
    MGlobal::executeCommand(Commands::LAYOUT_UV);

    //Get the new area and scale it back to its intial value
    double newArea = getUvArea();
    if (newArea <= 0.)
    {
        MGlobal::displayError("UV area invalid after group layout");
        return -1.;
    }

    double scaleFactor = sqrt(initialArea / newArea);
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -pu 0 -pv 0 -su %1 -sv %1").arg(scaleFactor)));

    //Restore initial shell scaling setting
    MGlobal::setOptionVarValue(OptionVars::SHELL_PRE_SCALING, prevShellScalingSetting);

    MGlobal::clearSelectionList();

    return scaleFactor;
}

double UVGroup::layoutRecursively()
{
    /* Order of operations:
     * Run layout method on all child groups
     * Create proxy planes for all child groups
     * Do a group layout on top level shells and the proxy planes
     * Scale the child groups and move them into position
     */

    //Store existing scaling setting
    int prevShellScalingSetting = MGlobal::optionVarIntValue(OptionVars::SHELL_PRE_SCALING);
    //Disable shell pre-scaling
    MGlobal::setOptionVarValue(OptionVars::SHELL_PRE_SCALING, 1);

    QList<QPair<UVGroup*, MDagPath>> childGroupSet;
    MSelectionList topLevelSelection;
    for (UVGroup* childGroup : _children)
    {
        double childScaleFactor = childGroup->layoutRecursively();

        if (childScaleFactor <= 0.) return -1.;

        //Create proxy planes
        AABB childAabb = childGroup->getAABB();
        if (!childAabb.isValid()) return -1.;

        MStringArray planeCommandOutput;
        if (!MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyPlane -w %1 -h %2 -sx 1 -sy 1 -createUVs 2").arg(childAabb.xmax - childAabb.xmin).arg(childAabb.ymax - childAabb.ymin)), planeCommandOutput)) return -1.;

        MSelectionList planeSelection;
        planeSelection.add(planeCommandOutput[0]);
        MDagPath planePath;
        planeSelection.getDagPath(0, planePath);
        if (!planePath.isValid()) return -1.;

        planePath.extendToShape();
        MeshData* planeData = MeshData::getMeshData(planePath);
        if (!planeData) return -1.;
        MObject planeFaces =  planeData->getUvShell(0).faces;

        MSelectionList currentPlaneUvSelection;
        currentPlaneUvSelection.add(planePath, planeFaces);
        topLevelSelection.add(planePath, planeFaces);

        MGlobal::clearSelectionList();
        MGlobal::setActiveSelectionList(currentPlaneUvSelection);
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -pu 0 -pv 0 -su %1 -sv %1").arg(childScaleFactor)));
        MGlobal::clearSelectionList();

        childGroupSet << qMakePair(childGroup, planePath);
    }

    //Add the top level shells to the selection with the planes
    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        MeshData* data = MeshData::getMeshData(shell.first);
        if (!data) continue;

        MObject facesInShell = data->getUvShell(shell.second).faces;
        topLevelSelection.add(shell.first, facesInShell);
    }

    //Get initial area
    double initialArea = getUvArea();
    if (initialArea <= 0.) return -1.;

    //Layout shells in the group and the proxy planes
    MGlobal::clearSelectionList();
    MGlobal::setActiveSelectionList(topLevelSelection);
    MGlobal::executeCommand(Commands::LAYOUT_UV);

    //Scale the groups and move them into position
    for (const QPair<UVGroup*, MDagPath>& childGroup : childGroupSet)
    {
        MGlobal::clearSelectionList();
        MGlobal::setActiveSelectionList(childGroup.first->getFaces());

        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -pu 0 -pv 0 -su %1 -sv %1").arg(0.995)));

        //Get bottom left position of the plane
        float planeXMin = std::numeric_limits<float>::max();
        float planeYMin = std::numeric_limits<float>::max();
        for (MItMeshPolygon it(childGroup.second.node()); !it.isDone(); it.next())
        {
            for (unsigned int v = 0; v < it.polygonVertexCount(); v++)
            {
                float uvPoint[2];
                it.getUV(v, uvPoint);

                if (uvPoint[0] < planeXMin) planeXMin = uvPoint[0];
                if (uvPoint[1] < planeYMin) planeYMin = uvPoint[1];
            }
        }

        if (planeXMin == std::numeric_limits<float>::max() || planeYMin == std::numeric_limits<float>::max()) return -1.;

        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -u %1 -v %2").arg(planeXMin).arg(planeYMin)));

        //Delete the plane
        MSelectionList planeTransform;
        planeTransform.add(childGroup.second.transform());
        MGlobal::setActiveSelectionList(planeTransform);
        MGlobal::executeCommand(Commands::DELETE_SELECTION);
    }

    //Scale the entire thing down to its initial size
    double newArea = getUvArea();
    if (newArea <= 0.) return -1.;

    double scaleFactor = sqrt(initialArea / newArea);
    MGlobal::clearSelectionList();
    MGlobal::setActiveSelectionList(topLevelSelection);
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -pu 0 -pv 0 -su %1 -sv %1").arg(scaleFactor)));

    //Restore initial shell scaling setting
    MGlobal::setOptionVarValue(OptionVars::SHELL_PRE_SCALING, prevShellScalingSetting);

    MGlobal::clearSelectionList();

    return scaleFactor;
}
