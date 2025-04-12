#include "UVOutliner.h"
#include "ui_UVOutliner.h"

UVOutliner::UVOutliner(QWidget* parent)
    : MayaQWidgetDockableMixin(parent)
    , ui(new Ui::UVOutliner)
{
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

    QObject::connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &UVOutliner::onTreeWidgetItemSelectionChanged);

    _selectionChangedCallbackId = MEventMessage::addEventCallback("SelectionChanged", &UVOutliner::onSelectionChanged_wrapper, reinterpret_cast<void*>(this));

    //Whenever a new node is added to the dependency graph
    //Listen for transforms here otherwise it will grab the shape node and not the main object
    _nodeAddedCallbackId = MDGMessage::addNodeAddedCallback(MNodeFunction_wrapper<&UVOutliner::onNodeAdded>, "transform", this);

    //Whenever a new node is removed from the dependency graph
    _nodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(MNodeFunction_wrapper<&UVOutliner::onNodeRemoved>, "transform", this);

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

UVTreeWidgetItem* UVOutliner::addItem(const MDagPath& meshDagPath, unsigned int uvShellId, QTreeWidgetItem* parent)
{
    UVTreeWidgetItem* item;
    if (!parent) item = new UVTreeWidgetItem(ui->treeWidget);
    else item = new UVTreeWidgetItem(parent);

    item->setDagPath(meshDagPath);
    item->setUvShellId(uvShellId);

    auto wrapper = new QWidget(ui->treeWidget);
    wrapper->setContentsMargins(0, 0, 0, 0);
    item->setupUi(wrapper);

    ui->treeWidget->setItemWidget(item, 0, wrapper);

    return item;
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
        selectUVShell(uvItem->getDagPath(), uvItem->getUvShellId(), true);
    }
    _isPerformingSelection = false;
}

MObject UVOutliner::getUvsInShell(const MDagPath& dagPath, unsigned int shellIndex)
{
    MFnMesh mesh(dagPath);

    unsigned int numShells;
    MIntArray shellIndices;
    mesh.getUvShellsIds(shellIndices, numShells);

    if (shellIndex >= numShells)
    {
        MGlobal::displayError("UV shell index mismatch on " + dagPath.fullPathName());
        return MObject::kNullObj;
    }

    //Determine which UVs on the mesh belong to this shell
    MIntArray uvIndices;
    for (unsigned int i = 0; i < shellIndices.length(); i++)
    {
        if (shellIndices[i] == shellIndex) uvIndices.append(i);
    }

    MFnSingleIndexedComponent component;
    MObject shellComponent = component.create(MFn::kMeshMapComponent);
    component.addElements(uvIndices);

    return shellComponent;
}

MObject UVOutliner::getFacesInShell(const MDagPath& dagPath, unsigned int shellIndex)
{
    MFnMesh mesh(dagPath);

    unsigned int numShells;
    MIntArray shellIndices;
    mesh.getUvShellsIds(shellIndices, numShells);

    if (shellIndex >= numShells)
    {
        MGlobal::displayError("UV shell index mismatch on " + dagPath.fullPathName());
        return MObject::kNullObj;
    }

    //Start by getting the UVs that belong to this shell
    MIntArray uvIndices;
    for (unsigned int i = 0; i < shellIndices.length(); i++)
    {
        if (shellIndices[i] == shellIndex) uvIndices.append(i);
    }

    //Determine which faces (polygons) the UVs correspond to
    QSet<int> faceIndices;
    for (unsigned int f = 0; f < mesh.numPolygons(); f++)
    {
        for (unsigned int v = 0; v < mesh.polygonVertexCount(f); v++)
        {
            int polygonUvId;
            mesh.getPolygonUVid(f, v, polygonUvId);

            for (unsigned int i = 0; i < uvIndices.length(); i++)
            {
                if (polygonUvId == uvIndices[i])
                {
                    faceIndices << f;
                    break;
                }
            }
        }
    }

    MIntArray faceIndices_intArr;
    for (const int& faceIndex : faceIndices)
    {
        faceIndices_intArr.append(faceIndex);
    }

    MFnSingleIndexedComponent component;
    MObject shellComponent = component.create(MFn::kMeshPolygonComponent);
    component.addElements(faceIndices_intArr);

    return shellComponent;
}

void UVOutliner::selectUVShell(const MDagPath& meshDagPath, unsigned int shellIndex, bool mergeWithExisting)
{
    qDebug() << "Selected " << meshDagPath.fullPathName().asChar();

    MObject shellComponent = getFacesInShell(meshDagPath, shellIndex);

    MSelectionList selList;
    if (mergeWithExisting)
    {
        MGlobal::getActiveSelectionList(selList);
    }
    selList.add(meshDagPath, shellComponent, mergeWithExisting);

    MGlobal::setActiveSelectionList(selList);
}

void UVOutliner::onSelectionChanged_wrapper(void* clientData)
{
    auto instance = reinterpret_cast<UVOutliner*>(clientData);
    if (!instance) return;

    instance->onSelectionChanged();
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

        MObject uvsInShell = getUvsInShell(uvItem->getDagPath(), uvItem->getUvShellId());
        MObject facesInShell = getFacesInShell(uvItem->getDagPath(), uvItem->getUvShellId());

        if (selList.hasItem(uvItem->getDagPath(), uvsInShell) || selList.hasItem(uvItem->getDagPath(), facesInShell))
        {
            uvItem->setSelectionState(UVTreeWidgetItem::FullySelected);
            //Make it actually selected since the states are visual only
            uvItem->setSelected(true);
        }
        else if (selList.hasItemPartly(uvItem->getDagPath(), uvsInShell))
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
    if (object.isNull() || !object.hasFn(MFn::kDagNode)) return;

    MFnDagNode node(object);
    if (!node.hasObj(MFn::kMesh)) return;

    qDebug() << "Mesh added";
}

void UVOutliner::onNodeRemoved(MObject& object)
{
    if (object.isNull() || !object.hasFn(MFn::kDagNode)) return;

    MFnDagNode node(object);
    if (!node.hasObj(MFn::kMesh)) return;

    qDebug() << "Mesh removed";
}

void UVOutliner::onSceneUpdated()
{
    qDebug() << "Scene updated";
}


