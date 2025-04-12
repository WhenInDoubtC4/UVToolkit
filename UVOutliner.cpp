#include "UVOutliner.h"
#include "ui_UVOutliner.h"
#include "ui_UVTreeWidgetItem.h"

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
}

UVOutliner::~UVOutliner()
{
    MEventMessage::removeCallback(_selectionChangedCallbackId);

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

UVTreeWidgetItem::UVTreeWidgetItem(QTreeWidget* parent)
    : QTreeWidgetItem(parent)
    , _ui(new Ui::UVTreeWidgetItem)
{

}

UVTreeWidgetItem::UVTreeWidgetItem(QTreeWidgetItem* parent)
    : QTreeWidgetItem(parent)
    , _ui(new Ui::UVTreeWidgetItem)
{

}

UVTreeWidgetItem::~UVTreeWidgetItem()
{
    delete _ui;
}

void UVTreeWidgetItem::setupUi(QWidget* widget)
{
    _ui->setupUi(widget);

    _ui->label->setText(QStringLiteral("<b>%1</b> [%2]").arg(MQtUtil::toQString(_dagPath.partialPathName())).arg(_uvShellId));
}

UVTreeWidgetItemDelegate::UVTreeWidgetItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{

}

void UVTreeWidgetItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;

    //Get the item at this index
    auto treeWidget = dynamic_cast<const QTreeWidget*>(opt.widget);
    QTreeWidgetItem* item = treeWidget->itemFromIndex(index);
    auto uvItem = dynamic_cast<UVTreeWidgetItem*>(item);

    if (uvItem)
    {
        //Override the selection background based on selection state
        switch (uvItem->getSelectionState())
        {
        case UVTreeWidgetItem::FullySelected:
            //Use the default selection color, force selection state
            opt.state |= QStyle::State_Selected;
            break;
        case UVTreeWidgetItem::PartiallySelected: {
            //Force selection state first
            opt.state |= QStyle::State_Selected;

            //Then use a darker shade of the selection color
            QColor mayaHighlightColor = opt.palette.color(QPalette::Highlight);
            opt.palette.setColor(QPalette::Highlight, mayaHighlightColor.darker(150));
            opt.palette.setColor(QPalette::HighlightedText, opt.palette.color(QPalette::HighlightedText).darker(110));
            break;
        }
        case UVTreeWidgetItem::NotSelected:
            //Explicitly remove selection highlight if not selected
            opt.state &= ~QStyle::State_Selected;
            break;
        }
    }

    // Call the parent class to do the actual painting with our modified options
    QStyledItemDelegate::paint(painter, opt, index);

    // Draw a visual indicator for partially selected items
    if (uvItem && uvItem->getSelectionState() == UVTreeWidgetItem::PartiallySelected)
    {
        // Draw a colored border around partially selected items
        painter->save();
        painter->setPen(QPen(opt.palette.color(QPalette::Highlight), 2));
        painter->drawRect(opt.rect.adjusted(1, 1, -1, -1));
        painter->restore();
    }
}
