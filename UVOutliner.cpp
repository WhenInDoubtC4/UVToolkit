#include "UVOutliner.h"
#include "ui_UVOutliner.h"

UVOutliner::UVOutliner(QWidget* parent)
    : MayaQWidgetDockableMixin(parent)
    , ui(new Ui::UVOutliner)
{
    /////////////////////////////////////////////////////////
    /// UI Setup
    //Explicitly set the window title for the global object because the ui is loaded into the wrapepr
    setWindowTitle("UV Outliner");

    _wrapper = new QWidget(this);
    ui->setupUi(_wrapper);

    setDockableParameters(true, true, MayaQWidgetDockableMixin::Area::A_LEFT, MayaQWidgetDockableMixin::AllowedArea::ALL, 300, 300, 0, 0, true);
    QWidget* centralWidget = getMayaControl();
    MQtUtil::addWidgetToMayaLayout(_wrapper, centralWidget);

    ui->treeWidget->setColumnWidth(0, 200);
    ui->treeWidget->setColumnWidth(1, 30);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->treeWidget->setItemDelegate(new UVTreeWidgetItemDelegate(ui->treeWidget));

    /////////////////////////////////////////////////////////
    /// Add callbacks
    QObject::connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &UVOutliner::onTreeWidgetItemSelectionChanged);
    QObject::connect(ui->refreshButton, &QPushButton::clicked, this, &UVOutliner::onRefreshButtonClicked);
    QObject::connect(ui->groupButton, &QPushButton::clicked, this, &UVOutliner::onGroupButtonClicked);
    QObject::connect(ui->layoutAllButton, &QPushButton::clicked, this, &UVOutliner::onLayoutAllButtonClicked);
    QObject::connect(ui->recursiveLayoutAllButton, &QPushButton::clicked, this, &UVOutliner::onRecursiveLayoutAllButtonClicked);
    QObject::connect(ui->treeWidget, &QWidget::customContextMenuRequested, this, &UVOutliner::onTreeWidgetContextMenuRequested);

    QObject::connect(MeshManager::getInst(), &MeshManager::meshUvShellAdded, this, &UVOutliner::onUvShellAdded);
    QObject::connect(MeshManager::getInst(), &MeshManager::meshUvShellIndexChanged, this, &UVOutliner::onUvShellIndexChanged);
    QObject::connect(MeshManager::getInst(), &MeshManager::meshUvShellRemoved, this, &UVOutliner::onUvShellRemoved);
    QObject::connect(MeshManager::getInst(), &MeshManager::meshUvDataRefreshed, this, &UVOutliner::onUvDataRefreshed);
    QObject::connect(MeshManager::getInst(), &MeshManager::groupCreated, this, &UVOutliner::onGroupCreated);
    QObject::connect(MeshManager::getInst(), &MeshManager::groupDeleted, this, &UVOutliner::onGroupDeleted);
    QObject::connect(MeshManager::getInst(), &MeshManager::uvShellAddedToGroup, this, &UVOutliner::onUvShellAddedToGroup);
    QObject::connect(MeshManager::getInst(), &MeshManager::uvShellRemovedFromGroup, this, &UVOutliner::onUvShellRemovedFromGroup);

    _selectionChangedCallbackId = MEventMessage::addEventCallback("SelectionChanged", MBasicFunction_wrapper<&UVOutliner::onSelectionChanged>, this);

    //Whenever a new node is added to the dependency graph
    //Listen for transforms here otherwise it will grab the shape node and not the main object
    _nodeAddedCallbackId = MDGMessage::addNodeAddedCallback(MNodeFunction_wrapper<&UVOutliner::onNodeAdded>, "transform", this);

    //Whenever a new node is removed from the dependency graph
    _nodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(MNodeFunction_wrapper<&UVOutliner::onNodeRemoved>, "mesh", this);

    //Called after any operation that changes which files are loaded
    _sceneUpdatedCallbackId = MSceneMessage::addCallback(MSceneMessage::kSceneUpdate, MBasicFunction_wrapper<&UVOutliner::onSceneUpdated>, this);

    /////////////////////////////////////////////////////////
    /// Init existing meshes
    addExistingMeshesAndGroups();
}

UVOutliner::~UVOutliner()
{
    qDebug() << "UV outliner deleted";

    MMessage::removeCallback(_selectionChangedCallbackId);

    MMessage::removeCallback(_nodeAddedCallbackId);
    MMessage::removeCallback(_nodeRemovedCallbackId);

    MMessage::removeCallback(_sceneUpdatedCallbackId);

    delete ui;
}

QTreeWidget* UVOutliner::getTreeWidget()
{
    return ui->treeWidget;
}

ShellTreeWidgetItem* UVOutliner::addItem(MeshData* meshData, unsigned int uvShellId, QTreeWidgetItem* parent)
{
    qDebug() << "Adding uv shell" << uvShellId << "for mesh" << meshData->getDagPath().fullPathName().asChar();

    ShellTreeWidgetItem* item;
    if (!parent) item = new ShellTreeWidgetItem(ui->treeWidget);
    else item = new ShellTreeWidgetItem(parent);

    item->setMeshData(meshData);
    item->setUvShellId(uvShellId);

    auto wrapper = new QWidget(ui->treeWidget);
    wrapper->setContentsMargins(0, 0, 0, 0);
    item->setupUi(wrapper);

    ui->treeWidget->setItemWidget(item, 0, wrapper);

    return item;
}

GroupTreeWidgetItem* UVOutliner::addItem(UVGroup* group, QTreeWidgetItem* parent)
{
    GroupTreeWidgetItem* item;
    if (!parent) item = new GroupTreeWidgetItem(ui->treeWidget);
    else item = new GroupTreeWidgetItem(parent);

    item->setGroup(group);

    auto wrapper = new QWidget(ui->treeWidget);

    wrapper->setContentsMargins(0, 0, 0, 0);
    item->setupUi(wrapper);

    ui->treeWidget->setItemWidget(item, 0, wrapper);

    return item;
}

void UVOutliner::removeItem(MeshData* mesh) const
{
    QSet<ShellTreeWidgetItem*> itemsToRemove;
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(*it);
        if (!uvItem)
        {
            qDebug() << "Cast failed on item";
            continue;
        }

        if (uvItem->getMeshData() != mesh)
        {
            qDebug() << "Shell" << uvItem->getUvShellId() << "," << uvItem->getMeshData()->getDagPath().fullPathName().asChar() << "does not belong to mesh";
            continue;
        }

        qDebug() << "Removing uv shell" << uvItem->getUvShellId() << "for mesh" << mesh->getDagPath().fullPathName().asChar();

        if (uvItem->parent())
        {
            itemsToRemove << uvItem;
        }
        else
        {
            itemsToRemove << uvItem;
        }
    }

    //Do not edit the tree while actively iterating through it
    for (ShellTreeWidgetItem* item : itemsToRemove)
    {
        if (item->parent())
        {
            item->parent()->removeChild(item);
        }
        else
        {
            int index = ui->treeWidget->indexOfTopLevelItem(item);
            ui->treeWidget->takeTopLevelItem(index);
        }

        delete item;
    }
}

void UVOutliner::removeItem(MeshData* mesh, unsigned int uvShellId)
{
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(*it);
        if (!uvItem) continue;

        if (uvItem->getMeshData() == mesh && uvItem->getUvShellId() == uvShellId)
        {
            if (uvItem->parent())
            {
                uvItem->parent()->removeChild(uvItem);
            }
            else
            {
                int index = ui->treeWidget->indexOfTopLevelItem(uvItem);
                ui->treeWidget->takeTopLevelItem(index);
            }

            delete uvItem;
            break;
        }
    }
}

void UVOutliner::onTreeWidgetItemSelectionChanged()
{
    //TODO: This might have to be removed as this gets more complex

    //Set this flag as this process will set off the SelectionChanged event
    _isPerformingSelection = true;
    MGlobal::clearSelectionList();

    //Clear selection states
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto uvItem = dynamic_cast<UVTreeWidgetItem*>(*it);
        if (!uvItem) continue;
        uvItem->setSelectionState(UVTreeWidgetItem::NotSelected);
    }

    //Deselect
    if (ui->treeWidget->selectedItems().empty())
    {
        qDebug() << "Deselect";
        _isPerformingSelection = false;
        return;
    }

    for (QTreeWidgetItem* selectedItem : ui->treeWidget->selectedItems())
    {
        //Select an individual shell
        auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(selectedItem);
        if (uvItem)
        {
            uvItem->setSelectionState(UVTreeWidgetItem::FullySelected, false, true);
            selectUVShell(uvItem->getMeshData(), uvItem->getUvShellId(), true);
        }

        //Select an entire group
        auto groupItem = dynamic_cast<GroupTreeWidgetItem*>(selectedItem);
        if (groupItem)
        {
            groupItem->setSelectionState(UVTreeWidgetItem::FullySelected, true, false);
            selectUVShellsRecursively(groupItem);
        }

    }
    _isPerformingSelection = false;
}

void UVOutliner::onRefreshButtonClicked()
{
    //Remove everything
    ui->treeWidget->clear();

    addExistingMeshesAndGroups();
}

void UVOutliner::onGroupButtonClicked()
{
    MGlobal::executeCommand(GroupShellsCmd::kCmdName);
}

void UVOutliner::onLayoutAllButtonClicked()
{
    MGlobal::executeCommand(LayoutAllCmd::kCmdName);
}

void UVOutliner::onRecursiveLayoutAllButtonClicked()
{
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("%1 %2").arg(LayoutAllCmd::kCmdName).arg(LayoutAllCmd::kRecursiveFlagName)));
}

void UVOutliner::onTreeWidgetContextMenuRequested(const QPoint& pos)
{
    QMenu contextMenu(ui->treeWidget);

    auto expandAllAction = new QAction("Expand all", ui->treeWidget);
    auto collapseAllAction = new QAction("Collapse all", ui->treeWidget);

    contextMenu.addAction(expandAllAction);
    contextMenu.addAction(collapseAllAction);

    QObject::connect(expandAllAction, &QAction::triggered, ui->treeWidget, &QTreeWidget::expandAll);
    QObject::connect(collapseAllAction, &QAction::triggered, ui->treeWidget, &QTreeWidget::collapseAll);

    contextMenu.exec(ui->treeWidget->mapToGlobal(pos));
}

void UVOutliner::onUvShellAdded(MeshData* meshData, unsigned int uvShellId)
{
    UVGroup* group = MeshManager::getInst()->getShellGroup(meshData, uvShellId);

    GroupTreeWidgetItem* target = nullptr;
    if (group)
    {
        //Find the group
        for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
        {
            auto groupItem = dynamic_cast<GroupTreeWidgetItem*>(*it);
            if (!groupItem) continue;

            if (groupItem->getGroup() == group)
            {
                target = groupItem;
                break;
            }
        }
    }
    else
    {
        qDebug() << "Shell" << uvShellId << "on mesh" << meshData->getDagPath().fullPathName().asChar() << "does not belong to a gropup";
    }

    addItem(meshData, uvShellId, target);
}

void UVOutliner::onUvShellIndexChanged(MeshData* meshData, unsigned int oldIndex, unsigned int newIndex)
{
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(*it);
        if (!uvItem) continue;

        if (uvItem->getMeshData() == meshData && uvItem->getUvShellId() == oldIndex)
        {
            uvItem->setUvShellId(newIndex);
            break;
        }
    }
}

void UVOutliner::onUvShellRemoved(MeshData* meshData, unsigned int index)
{
    removeItem(meshData, index);
}

void UVOutliner::onUvDataRefreshed(MeshData* mesh)
{
    //Remove everything related to this mesh
    removeItem(mesh);
}

void UVOutliner::onGroupCreated(UVGroup* group)
{
    if (!group->getParent())
    {
        //Add as top level item
        addItem(group);
    }
    else
    {
        //Find the parent item
        GroupTreeWidgetItem* item = nullptr;
        for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
        {
            auto groupItem = dynamic_cast<GroupTreeWidgetItem*>(*it);
            if (!groupItem) continue;

            if (groupItem->getGroup() == group->getParent())
            {
                item = groupItem;
                break;
            }
        }

        addItem(group, item);
    }
}

void UVOutliner::onGroupDeleted(UVGroup* group)
{
    //Figure out which item the group belonged to
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto groupItem = dynamic_cast<GroupTreeWidgetItem*>(*it);
        if (!groupItem) continue;

        if (groupItem->getGroup() != group) continue;

        QSet<ShellTreeWidgetItem*> childShells;
        QSet<GroupTreeWidgetItem*> childGroups;
        for (int i = 0; i < groupItem->childCount(); i++)
        {
            QTreeWidgetItem* childItem = groupItem->child(i);

            if (auto childShell = dynamic_cast<ShellTreeWidgetItem*>(childItem); childShell)
            {
                childShells << childShell;
            }
            else if (auto childGroup = dynamic_cast<GroupTreeWidgetItem*>(childItem); childGroup)
            {
                childGroups << childGroup;
            }
        }

        //All child UV shells are reparented to the world
        for (ShellTreeWidgetItem* uvItem : childShells)
        {
            groupItem->removeChild(uvItem);
            ui->treeWidget->addTopLevelItem(uvItem);

            //Recreate the wrapper widget
            auto wrapper = new QWidget(ui->treeWidget);
            wrapper->setContentsMargins(0, 0, 0, 0);
            uvItem->setupUi(wrapper);

            ui->treeWidget->setItemWidget(uvItem, 0, wrapper);
        }

        //All chuld child  are reparented to the parent group, or the world
        for (GroupTreeWidgetItem* childGroup : childGroups)
        {
            groupItem->removeChild(childGroup);

            if (groupItem->parent())
            {
                groupItem->parent()->addChild(childGroup);
            }
            else
            {
                ui->treeWidget->addTopLevelItem(childGroup);
            }

            //Recreate the wrapper widget
            auto wrapper = new QWidget(ui->treeWidget);
            wrapper->setContentsMargins(0, 0, 0, 0);
            childGroup->setupUi(wrapper);

            ui->treeWidget->setItemWidget(childGroup, 0, wrapper);

            //Recreate the wrapper widgets for all children
            recreateShellWrapeprWidgetsRecursive(childGroup);
        }

        //Remove the group item
        if (groupItem->parent())
        {
            groupItem->parent()->removeChild(groupItem);
        }
        else
        {
            int itemIndex = ui->treeWidget->indexOfTopLevelItem(groupItem);
            ui->treeWidget->takeTopLevelItem(itemIndex);
        }

        break;
    }
}

void UVOutliner::recreateShellWrapeprWidgetsRecursive(GroupTreeWidgetItem* root)
{
    for (int i = 0; i < root->childCount(); i++)
    {
        QTreeWidgetItem* childItem = root->child(i);

        if (auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(childItem); uvItem)
        {
            auto wrapper = new QWidget(ui->treeWidget);
            wrapper->setContentsMargins(0, 0, 0, 0);
            uvItem->setupUi(wrapper);

            ui->treeWidget->setItemWidget(uvItem, 0, wrapper);
        }
        else if (auto groupItem = dynamic_cast<GroupTreeWidgetItem*>(childItem); groupItem)
        {
            recreateShellWrapeprWidgetsRecursive(groupItem);
        }
    }
}

void UVOutliner::reparentUvShellItem(UVGroup* group, MeshData* mesh, unsigned int shellIndex, bool addToGroup)
{
    qDebug() << "UV outliner: reparenting UV shell item" << mesh->getDagPath().fullPathName().asChar() << shellIndex;

    if (!group)
    {
        qDebug() << "Group is not valid!!!";
    }

    //Find the target group
    GroupTreeWidgetItem* item = nullptr;
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto groupItem = dynamic_cast<GroupTreeWidgetItem*>(*it);
        if (!groupItem) continue;

        if (groupItem->getGroup() == group)
        {
            item = groupItem;
            break;
        }
    }

    if (!item) return;

    //Find the specific UV shell item
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto shellItem = dynamic_cast<ShellTreeWidgetItem*>(*it);
        if (!shellItem) continue;

        if (shellItem->getMeshData() == mesh && shellItem->getUvShellId() == shellIndex)
        {
            //Remove from the existing parent
            if (shellItem->parent())
            {
                shellItem->parent()->removeChild(shellItem);
            }
            else
            {
                int shellItemIndex = ui->treeWidget->indexOfTopLevelItem(shellItem);
                ui->treeWidget->takeTopLevelItem(shellItemIndex);
            }

            //Add to a newly created group
            if (addToGroup)
            {
                item->addChild(shellItem);
            }
            //Remove from a group (add it back as top level)
            else
            {
                ui->treeWidget->addTopLevelItem(shellItem);
            }

            //Recreate the wrapper widget
            auto wrapper = new QWidget(ui->treeWidget);
            wrapper->setContentsMargins(0, 0, 0, 0);
            shellItem->setupUi(wrapper);

            ui->treeWidget->setItemWidget(shellItem, 0, wrapper);

            break;
        }
    }
    if (addToGroup) item->setExpanded(true);
}

void UVOutliner::onUvShellAddedToGroup(UVGroup* group, MeshData* mesh, unsigned int shellIndex)
{
    qDebug() << "UV Outliner: Shell added to group" << mesh->getDagPath().fullPathName().asChar() << shellIndex;

    reparentUvShellItem(group, mesh, shellIndex, true);
}

void UVOutliner::onUvShellRemovedFromGroup(UVGroup* group, MeshData* mesh, unsigned int shellIndex)
{
    reparentUvShellItem(group, mesh, shellIndex, false);
}

void UVOutliner::selectUVShell(MeshData* meshData, unsigned int shellIndex, bool mergeWithExisting)
{
    MDagPath meshDagPath(meshData->getDagPath());

    qDebug() << "Selected " << meshDagPath.fullPathName().asChar();

    MObject shellComponent = meshData->getUvShell(shellIndex).faces;

    MSelectionList selList;
    if (mergeWithExisting)
    {
        MGlobal::getActiveSelectionList(selList);
    }
    selList.add(meshDagPath, shellComponent, mergeWithExisting);

    MGlobal::setActiveSelectionList(selList);
}

void UVOutliner::selectUVShellsRecursively(GroupTreeWidgetItem* root)
{
    for (int i = 0; i < root->childCount(); i++)
    {
        auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(root->child(i));

        if (uvItem)
        {
            selectUVShell(uvItem->getMeshData(), uvItem->getUvShellId(), true);
        }

        auto groupItem = dynamic_cast<GroupTreeWidgetItem*>(root->child(i));
        if (groupItem)
        {
            selectUVShellsRecursively(groupItem);
        }
    }
}

void UVOutliner::addExistingMeshesAndGroups()
{
    MeshManager::getInst()->readdExistingGroups();

    for (MeshData* meshData : MeshData::getMeshData())
    {   
        for (unsigned int i = 0; i < meshData->getNumUvShells(); i++)
        {
            onUvShellAdded(meshData, i);
        }
    }

    MeshManager::getInst()->addUntrackedMeshes();
}

void UVOutliner::onSelectionChanged()
{
    //Prevent infinite loop when the selection change is performed by the tree widget
    if (_isPerformingSelection) return;

    qDebug() << "Selection changed";

    MSelectionList selList;
    MGlobal::getActiveSelectionList(selList);

    //Deselection
    if (selList.isEmpty())
    {
        ui->treeWidget->clearSelection();
        for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
        {
            auto uvItem = dynamic_cast<UVTreeWidgetItem*>(*it);
            if (!uvItem) continue;

            uvItem->setSelectionState(UVTreeWidgetItem::NotSelected);
        }
        return;
    }

    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(*it);
        if (!uvItem) continue;

        const MeshData::UVData& uvShell = uvItem->getMeshData()->getUvShell(uvItem->getUvShellId());

        MDagPath meshDagPath(uvItem->getMeshData()->getDagPath());

        if (selList.hasItem(meshDagPath, uvShell.uvs) ||
            selList.hasItem(meshDagPath, uvShell.vertices) ||
            selList.hasItem(meshDagPath, uvShell.faces) ||
            selList.hasItem(meshDagPath, uvShell.edges))
        {
            uvItem->setSelectionState(UVTreeWidgetItem::FullySelected, false, true);
            //Make it actually selected since the states are visual only
            uvItem->setSelected(true);
        }
        else if (selList.hasItemPartly(meshDagPath, uvShell.uvs) ||
                   selList.hasItemPartly(meshDagPath, uvShell.vertices) ||
                   selList.hasItemPartly(meshDagPath, uvShell.faces) ||
                   selList.hasItemPartly(meshDagPath, uvShell.edges))
        {
            uvItem->setSelectionState(UVTreeWidgetItem::PartiallySelected, false, true);
        }
        else
        {
            uvItem->setSelectionState(UVTreeWidgetItem::NotSelected);
        }
     }
}

void UVOutliner::onNodeAdded(MObject& object)
{
    if (object.isNull() || !object.hasFn(MFn::kTransform)) return;

    MFnDagNode node(object);
    MDagPath path;
    node.getPath(path);

    qDebug() << "Path name" << path.fullPathName().asChar();

    if (!path.hasFn(MFn::kMesh)) return;

    MeshManager::getInst()->addMesh(new MeshData(path));
}

void UVOutliner::onNodeRemoved(MObject& object)
{
    if (object.isNull() || !object.hasFn(MFn::kMesh)) return;

    MFnDagNode node(object);
    MString meshName = node.partialPathName();
    qDebug() << "Mesh removed, node name" << meshName.asChar();

    MeshData* meshToRemove = MeshManager::getInst()->removeMeshByName(meshName);
    if (!meshToRemove) return;
    removeItem(meshToRemove);
    delete meshToRemove;
}

void UVOutliner::onSceneUpdated()
{
    qDebug() << "Scene updated";
}


