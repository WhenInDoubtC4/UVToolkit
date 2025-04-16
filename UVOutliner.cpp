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

    /////////////////////////////////////////////////////////
    /// Add callbacks
    QObject::connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &UVOutliner::onTreeWidgetItemSelectionChanged);

    //_selectionChangedCallbackId = MEventMessage::addEventCallback("SelectionChanged", &UVOutliner::onSelectionChanged_wrapper, reinterpret_cast<void*>(this));
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

    for (MeshData* data : _meshData) delete data;

    delete ui;
}

QTreeWidget* UVOutliner::getTreeWidget()
{
    return ui->treeWidget;
}

void UVOutliner::addMesh(MeshData* mesh)
{
    //Add connections
    QObject::connect(mesh, &MeshData::uvShellAdded, this, &UVOutliner::onUvShellAdded);

    //Get all UV shells on the mesh
    mesh->initUvShells();

    _meshData << mesh;
}

UVTreeWidgetItem* UVOutliner::addItem(MeshData* meshData, unsigned int uvShellId, QTreeWidgetItem* parent)
{
    qDebug() << "Adding item to tree" << meshData->getDagPath().fullPathName().asChar();

    UVTreeWidgetItem* item;
    if (!parent) item = new UVTreeWidgetItem(ui->treeWidget);
    else item = new UVTreeWidgetItem(parent);

    //item->setDagPath(meshDagPath);
    item->setMeshData(meshData);
    item->setUvShellId(uvShellId);

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
        auto uvItem = dynamic_cast<UVTreeWidgetItem*>(*it);
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
        auto uvItem = dynamic_cast<UVTreeWidgetItem*>(selectedItem);
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
        auto uvItem = dynamic_cast<UVTreeWidgetItem*>(*it);
        if (!uvItem) continue;

        MeshData::UVData uvShell(uvItem->getMeshData()->getUvShell(uvItem->getUvShellId()));
        MObject uvsInShell = uvShell.uvs;
        MObject facesInShell = uvShell.faces;

        MDagPath meshDagPath(uvItem->getMeshData()->getDagPath());

        //MObject uvsInShell = getUvsInShell(uvItem->getDagPath(), uvItem->getUvShellId());
        //MObject facesInShell = getFacesInShell(uvItem->getDagPath(), uvItem->getUvShellId());

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

    addMesh(new MeshData(path));
}

void UVOutliner::onNodeRemoved(MObject& object)
{
    if (object.isNull() || !object.hasFn(MFn::kMesh)) return;

    MFnDagNode node(object);
    MString meshName = node.partialPathName();
    qDebug() << "Mesh removed, node name" << meshName.asChar();

    MeshData* meshToRemove = nullptr;
    for (MeshData* data : _meshData)
    {
        if (data->getMeshName() != meshName) continue;

        //This will only remove one mesh at a time which can potentially be bad. Unsure if it will ever trigger multiple removals though
        meshToRemove = data;
        break;
    }

    if (!meshToRemove) return;
    _meshData.remove(meshToRemove);
    removeItem(meshToRemove->getDagPath());
    delete meshToRemove;
}

void UVOutliner::onSceneUpdated()
{
    qDebug() << "Scene updated";
}


