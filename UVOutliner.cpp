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

    ui->treeWidget->setItemDelegate(new UVTreeWidgetItemDelegate(ui->treeWidget));

    addExistingMeshes();

    /////////////////////////////////////////////////////////
    /// Add callbacks
    QObject::connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &UVOutliner::onTreeWidgetItemSelectionChanged);
    QObject::connect(MeshManager::getInst(), &MeshManager::meshUvShellAdded, this, &UVOutliner::onUvShellAdded);
    QObject::connect(MeshManager::getInst(), &MeshManager::groupCreated, this, &UVOutliner::onGroupCreated);
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
}

UVOutliner::~UVOutliner()
{
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
    qDebug() << "Adding item to tree" << meshData->getDagPath().fullPathName().asChar();

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

GroupTreeWidgetItem* UVOutliner::addItem(MeshManager::UVGroup* group, QTreeWidgetItem* parent)
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

void UVOutliner::removeItem(const MDagPath& meshDagPath) const
{
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(*it);
        if (!uvItem) continue;

        if (uvItem->getMeshData()->getDagPath().fullPathName() != meshDagPath.fullPathName()) continue;

        if (uvItem->parent())
        {
            uvItem->parent()->removeChild(uvItem);
        }
        else
        {
            QModelIndex itemIndex = ui->treeWidget->indexFromItem(uvItem);
            ui->treeWidget->takeTopLevelItem(itemIndex.row());
        }

        break;
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
        auto uvItem = dynamic_cast<ShellTreeWidgetItem*>(selectedItem);
        if (!uvItem) continue;

        uvItem->setSelectionState(UVTreeWidgetItem::FullySelected);
        selectUVShell(uvItem->getMeshData(), uvItem->getUvShellId(), true);
    }
    _isPerformingSelection = false;
}

void UVOutliner::onUvShellAdded(MeshData* meshData, unsigned int uvShellId)
{
    addItem(meshData, uvShellId);
}

void UVOutliner::onGroupCreated(MeshManager::UVGroup* group)
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

void UVOutliner::reparentUvShellItem(MeshManager::UVGroup* group, MeshData* mesh, unsigned int shellIndex, bool addToGroup)
{
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

void UVOutliner::onUvShellAddedToGroup(MeshManager::UVGroup* group, MeshData* mesh, unsigned int shellIndex)
{
    // GroupTreeWidgetItem* item = nullptr;
    // for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    // {
    //     auto groupItem = dynamic_cast<GroupTreeWidgetItem*>(*it);
    //     if (!groupItem) continue;

    //     if (groupItem->getGroup() == group)
    //     {
    //         item = groupItem;
    //         break;
    //     }
    // }

    // if (!item) return;

    // for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    // {
    //     auto shellItem = dynamic_cast<ShellTreeWidgetItem*>(*it);
    //     if (!shellItem) continue;

    //     if (shellItem->getMeshData() == mesh && shellItem->getUvShellId() == shellIndex)
    //     {
    //         if (shellItem->parent())
    //         {
    //             shellItem->parent()->removeChild(shellItem);
    //         }
    //         else
    //         {
    //             int shellItemIndex = ui->treeWidget->indexOfTopLevelItem(shellItem);
    //             ui->treeWidget->takeTopLevelItem(shellItemIndex);
    //         }

    //         item->addChild(shellItem);

    //         //Recreate the wrapper widget
    //         auto wrapper = new QWidget(ui->treeWidget);
    //         wrapper->setContentsMargins(0, 0, 0, 0);
    //         shellItem->setupUi(wrapper);

    //         ui->treeWidget->setItemWidget(shellItem, 0, wrapper);
    //         break;
    //     }
    // }
    // item->setExpanded(true);

    reparentUvShellItem(group, mesh, shellIndex, true);
}

void UVOutliner::onUvShellRemovedFromGroup(MeshManager::UVGroup* group, MeshData* mesh, unsigned int shellIndex)
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

void UVOutliner::addExistingMeshes()
{
    for (MeshData* meshData : MeshManager::getInst()->getMeshData())
    {
        for (unsigned int i = 0; i < meshData->getNumUvShells(); i++)
        {
            addItem(meshData, i);
        }
    }
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

        MeshData::UVData uvShell(uvItem->getMeshData()->getUvShell(uvItem->getUvShellId()));
        MObject uvsInShell = uvShell.uvs;
        MObject facesInShell = uvShell.faces;

        MDagPath meshDagPath(uvItem->getMeshData()->getDagPath());

        if (selList.hasItem(meshDagPath, uvsInShell) || selList.hasItem(meshDagPath, facesInShell))
        {
            uvItem->setSelectionState(UVTreeWidgetItem::FullySelected);
            //Make it actually selected since the states are visual only
            uvItem->setSelected(true);
        }
        else if (selList.hasItemPartly(meshDagPath, uvsInShell))
        {
            uvItem->setSelectionState(UVTreeWidgetItem::PartiallySelected);
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
    removeItem(meshToRemove->getDagPath());
    delete meshToRemove;
}

void UVOutliner::onSceneUpdated()
{
    qDebug() << "Scene updated";
}


