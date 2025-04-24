#include "LayoutAllCmd.h"

LayoutAllCmd::LayoutAllCmd()
    : MPxCommand()
{

}

LayoutAllCmd::~LayoutAllCmd()
{

}

void* LayoutAllCmd::creator()
{
    return new LayoutAllCmd();
}

MStatus LayoutAllCmd::doIt(const MArgList& argList)
{
    /* Order of operations:
     * 1. Gather all top level groups
     * 2. Create selection lists for: everything, each group
     * 3. Layout everything
     * 4. Get the UV area of an arbitrary shell in each group
     * 5. Layout the groups only
     * 6. Scale the group back down to match the UV area of what was noted earlier
     * 7. Create a proxy plane for each group
     * 8. Lay out globally with the proxy planes
     * 9. Scale the groups to match the proxy planes and move them into position
     */

    //Assume that the UVs have been laid out with the proper settings prior
    //Group layout will only work if scaling is disabled as all the scaling is handled here
    //Store existing scaling setting
    int prevShellScalingSetting;
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("optionVar -q %1").arg(OptionVars::SHELL_PRE_SCALING)), prevShellScalingSetting);
    //Disable shell scaling
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("optionVar -iv %1 1").arg(OptionVars::SHELL_PRE_SCALING)));

    MGlobal::clearSelectionList();

    QList<UVGroup*> topLevelGroups = MeshManager::getInst()->getTopLevelGroups();

    qDebug() << "Group layout has" << topLevelGroups.size() << "top level groups";

    ////////////////////////////////////////////////////
    ///Create selection lists
    MSelectionList globalSelection;
    //TODO: Handle hidden objects
    for (MeshData* mesh : MeshData::getMeshData())
    {
        for (unsigned int i = 0; i < mesh->getNumUvShells(); i++)
        {
            globalSelection.add(mesh->getDagPath(), mesh->getUvShell(i).faces);
        }
    }

    QList<MSelectionList> topLevelGroupSelections(topLevelGroups.size());
    for (int i = 0; i < topLevelGroups.size(); i++)
    {
        MSelectionList currentGroupSelection;
        for (const QPair<MDagPath, unsigned int>& shell : topLevelGroups[i]->getShells())
        {
            MeshData* mesh = MeshData::getMeshData(shell.first);
            if (!mesh) continue;

            currentGroupSelection.add(shell.first, mesh->getUvShell(shell.second).faces);
        }
        topLevelGroupSelections[i] = currentGroupSelection;
    }

    ////////////////////////////////////////////////////
    ///Global layout
    MGlobal::setActiveSelectionList(globalSelection);
    //TODO: This will do a layout based on the current settings. Might have to override this and set to preserve UV ratios
    MGlobal::executeCommand(Commands::LAYOUT_UV);

    ////////////////////////////////////////////////////
    ///Get the UV area of an arbitrary shell in each group
    QList<double> groupUvAreas(topLevelGroups.size());
    for (int i = 0; i < topLevelGroups.size(); i++)
    {
        double currentArea = topLevelGroups[i]->getUvArea();
        if (currentArea <= 0.)
        {
            MGlobal::displayError("Failed to get the UV area for a group");
            return MStatus::kFailure;
        }
        groupUvAreas[i] = currentArea;
    }

    ////////////////////////////////////////////////////
    ///Group operations
    QList<MDagPath> proxyPlanes(topLevelGroups.size());
    QList<MSelectionList> planeSelections(topLevelGroups.size());
    QList<double> proxyPlaneScales(topLevelGroups.size());

    for (int i = 0; i < topLevelGroups.size(); i++)
    {
        //Layout the group only
        //TODO: instead of running the layout command here, call the layout method on the group itself
        MGlobal::clearSelectionList();
        MGlobal::setActiveSelectionList(topLevelGroupSelections[i]);
        MGlobal::executeCommand(Commands::LAYOUT_UV);

        //Scale it back down to match the UV area of it prior
        double newArea = topLevelGroups[i]->getUvArea();
        if (newArea <= 0.)
        {
            MGlobal::displayError("Failed to get UV area for group after layout");
            return MStatus::kFailure;
        }

        double scaleFactor = sqrt(groupUvAreas[i] / newArea);
        proxyPlaneScales[i] = scaleFactor;
        if (!MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -pu 0 -pv 0 -su %1 -sv %1").arg(scaleFactor))))
        {
            MGlobal::displayError("Cannot scale group after layout");
            return MStatus::kFailure;
        }

        //Create the proxy bounding plane
        UVGroup::AABB groupAabb = topLevelGroups[i]->getAABB();
        if (!groupAabb.isValid())
        {
            MGlobal::displayError("Cannot get AABB for group");
            return MStatus::kFailure;
        }

        MStringArray planeCommandOutput;
        if (!MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyPlane -w %1 -h %2 -sx 1 -sy 1 -createUVs 2").arg(groupAabb.xmax - groupAabb.xmin).arg(groupAabb.ymax - groupAabb.ymin)), planeCommandOutput))
        {
            MGlobal::displayError("Could not create proxy plane for group");
            return MStatus::kFailure;
        }

        MSelectionList planeSelection;
        planeSelection.add(planeCommandOutput[0]);
        MDagPath planePath;
        planeSelection.getDagPath(0, planePath);
        if (!planePath.isValid())
        {
            MGlobal::displayError("DAG path for proxy plane is invalid");
            return MStatus::kFailure;
        }
        planePath.extendToShape();
        proxyPlanes[i] = planePath;

        //Scale the plane uv down
        MeshData* planeData = MeshData::getMeshData(planePath);
        if (!planeData)
        {
            MGlobal::displayError("Failed to get mesh data for proxy plane");
            return MStatus::kFailure;
        }
        MSelectionList planeUvSelection;
        planeUvSelection.add(planePath, planeData->getUvShell(0).faces);
        MGlobal::clearSelectionList();
        MGlobal::setActiveSelectionList(planeUvSelection);
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -pu 0 -pv 0 -su %1 -sv %1").arg(scaleFactor)));
        planeSelections[i] = planeUvSelection;
        MGlobal::clearSelectionList();
    }

    ////////////////////////////////////////////////////
    ///Layout globally with the proxy planes
    MSelectionList proxyGlobalSelection = globalSelection;
    //Remove all groups
    for (const MSelectionList& groupSelection : topLevelGroupSelections)
    {
        proxyGlobalSelection.merge(groupSelection, MSelectionList::kRemoveFromList);
    }
    //Add all proxy planes
    for (const MSelectionList& planeSelection : planeSelections)
    {
        proxyGlobalSelection.merge(planeSelection);
    }

    MGlobal::setActiveSelectionList(proxyGlobalSelection);
    MGlobal::executeCommand(Commands::LAYOUT_UV);

    ////////////////////////////////////////////////////
    ///Scale the groups to match the proxy planes and move them into position
    for (int i = 0; i < topLevelGroups.size(); i++)
    {
        MGlobal::clearSelectionList();
        MGlobal::setActiveSelectionList(topLevelGroupSelections[i]);

        // UVGroup::AABB groupAabb = topLevelGroups[i]->getAABB();
        // double newScale = sqrt((groupAabb.xmax - groupAabb.xmin) * (groupAabb.ymax - groupAabb.ymin));
        // double scaleFactor = proxyPlaneScales[i] / newScale;
        // qDebug() << "Plane scale" << newScale << "group scale factor" << scaleFactor;
        // scaleFactor *= .995; //Apply a small padding so shells won't overlap

        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -pu 0 -pv 0 -su %1 -sv %1").arg(0.995)));

        //Get bottom left position of the plane
        float planeXMin = std::numeric_limits<float>::max();
        float planeYMin = std::numeric_limits<float>::max();
        for (MItMeshPolygon it(proxyPlanes[i].node()); !it.isDone(); it.next())
        {
            for (unsigned int v = 0; v < it.polygonVertexCount(); v++)
            {
                float uvPoint[2];
                it.getUV(v, uvPoint);

                if (uvPoint[0] < planeXMin) planeXMin = uvPoint[0];
                if (uvPoint[1] < planeYMin) planeYMin = uvPoint[1];
            }
        }

        if (planeXMin == std::numeric_limits<float>::max() || planeYMin == std::numeric_limits<float>::max())
        {
            MGlobal::displayError("Failed to get bottom left corner of proxy plane");
            return MStatus::kFailure;
        }

        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("polyEditUV -u %1 -v %2").arg(planeXMin).arg(planeYMin)));
    }

    ////////////////////////////////////////////////////
    ///Cleanup
    //Delete planes
    MSelectionList planesSelection;
    MGlobal::clearSelectionList();
    for (const MDagPath& planePath : proxyPlanes)
    {
        planesSelection.add(planePath.transform());
    }
    MGlobal::setActiveSelectionList(planesSelection);
    MGlobal::executeCommand(Commands::DELETE_SELECTION);

    //Restore initial shell scaling setting
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("optionVar -iv %1 %2").arg(OptionVars::SHELL_PRE_SCALING).arg(prevShellScalingSetting)));

    //Clear selection
    MGlobal::clearSelectionList();
    return MStatus::kSuccess;
}
