#include "GroupTreeWidgetItem.h"
#include "ui_GroupTreeWidgetItem.h"

GroupTreeWidgetItem::GroupTreeWidgetItem(QTreeWidget* parent)
    : UVTreeWidgetItem(parent)
    , _ui(new Ui::GroupTreeWidgetItem)
{

}

GroupTreeWidgetItem::GroupTreeWidgetItem(QTreeWidgetItem* parent)
    : UVTreeWidgetItem(parent)
    , _ui(new Ui::GroupTreeWidgetItem)
{

}

GroupTreeWidgetItem::~GroupTreeWidgetItem()
{
    delete _ui;
}

QString GroupTreeWidgetItem::getSafeGroupName()
{
    QString groupName = _uvGroup->getName();
    if (groupName.isEmpty()) groupName = QStringLiteral("uvGroup%1").arg(_uvGroup->getId());

    return groupName;
}

void GroupTreeWidgetItem::setupUi(QWidget* widget)
{
    _widget = widget;
    _ui->setupUi(widget);
    _ui->label->setText(getSafeGroupName());

    widget->setAcceptDrops(true);
    setupDragAndDrop(widget, _ui->groupLabel);

    auto eventFilter = new GroupTreeWidgetItemEventFilter(this, widget);
    widget->installEventFilter(eventFilter);
    QObject::connect(_ui->lineEdit, &QLineEdit::editingFinished, eventFilter, &GroupTreeWidgetItemEventFilter::onLineEditFinished);
    QObject::connect(_ui->lineEdit, &QLineEdit::inputRejected, eventFilter, &GroupTreeWidgetItemEventFilter::onLineEditFinished);

    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(widget, &QWidget::customContextMenuRequested, eventFilter, &GroupTreeWidgetItemEventFilter::onCustomContextMenuRequested);
}

QDrag* GroupTreeWidgetItem::onDrag()
{
    auto drag = new QDrag(_widget);
    auto mimeData = new QMimeData();

    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream << reinterpret_cast<unsigned long long>(this);

    mimeData->setData(MimeTypes::GROUP_TREE_WIDGET_ITEM, byteArray);
    drag->setMimeData(mimeData);

    return drag;
}

GroupTreeWidgetItemEventFilter::GroupTreeWidgetItemEventFilter(GroupTreeWidgetItem* parent, QWidget* widget)
    : QObject(widget)
    , _parent(parent)
    , _widget(widget)
{

}

bool GroupTreeWidgetItemEventFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::DragEnter)
    {
        auto dragEnterEvent = dynamic_cast<QDragEnterEvent*>(event);
        if (!dragEnterEvent) return false;
        if (dragEnterEvent->mimeData()->hasFormat(MimeTypes::SHELL_TREE_WIDGET_ITEM) ||
            dragEnterEvent->mimeData()->hasFormat(MimeTypes::GROUP_TREE_WIDGET_ITEM))
        {
            dragEnterEvent->acceptProposedAction();
        }

        return false;
    }

    if (event->type() == QEvent::Drop)
    {
        auto dropEvent = dynamic_cast<QDropEvent*>(event);
        if (!dropEvent) return false;

        if (dropEvent->mimeData()->hasFormat(MimeTypes::SHELL_TREE_WIDGET_ITEM)) onShellDrop(dropEvent);
        else if (dropEvent->mimeData()->hasFormat(MimeTypes::GROUP_TREE_WIDGET_ITEM)) onGroupDrop(dropEvent);
        return false;
    }

    if (event->type() != QEvent::MouseButtonDblClick) return false;

    if (_parent->_ui->stackedWidget->currentIndex() != 0) return false;

    _parent->_ui->stackedWidget->setCurrentIndex(1);
    _parent->_ui->lineEdit->setText(_parent->getSafeGroupName());
    _parent->_ui->lineEdit->setFocus();
    _parent->_ui->lineEdit->selectAll();

    //Eat the event so that the tree doesn't get collapsed or expanded
    return true;
}

template<typename T>
T* GroupTreeWidgetItemEventFilter::getDroppedItem(const QByteArray& dropData)
{
    QDataStream stream(dropData);

    unsigned long long addr;
    stream >> addr;
    auto item = reinterpret_cast<T*>(addr);

    return item;
}

template<typename T>
QSet<T*> GroupTreeWidgetItemEventFilter::expandDroppedItem(T* droppedItem)
{
    if (!droppedItem) return {};

    //Mirror Maya's outliner behavior:
    //Scenario 1/4: Nothing is selected. Reparent the dropped item
    //Sencario 2/4: Only the dropped item is selected. Reparent the dropped item
    //Sencario 3/4: There is some other stuff selected, but the dropped item is not among them. Reparent the dropped item ONLY (which is not intuitive but whatever at least make it consistent)
    //Scenario 4/4: There is some other stuff selected AND the dropped item IS one of them. Reparent all

    //See what else is selected and if the dropped item is one of them
    bool selectionContainsItem = false;
    QSet<T*> result;
    for (QTreeWidgetItem* selectedItem : _parent->treeWidget()->selectedItems())
    {
        auto selectedT = dynamic_cast<T*>(selectedItem);
        if (selectedT) result << selectedT;
        if (selectedT == droppedItem) selectionContainsItem = true;
    }

    //Reparent just this one, otherwise all of them
    if (!selectionContainsItem)
    {
        result.clear();
        result << droppedItem;
    }

    return result;
}

void GroupTreeWidgetItemEventFilter::onShellDrop(QDropEvent* dropEvent)
{
    auto droppedUvItem = getDroppedItem<ShellTreeWidgetItem>(dropEvent->mimeData()->data(MimeTypes::SHELL_TREE_WIDGET_ITEM));
    QSet<ShellTreeWidgetItem*> selectedUvItems = expandDroppedItem<ShellTreeWidgetItem>(droppedUvItem);

    for (ShellTreeWidgetItem* item : selectedUvItems)
    {
        //See if it belongs to a group already
        UVGroup* group = MeshManager::getInst()->getShellGroup(item->getMeshData(), item->getUvShellId());
        if (group)
        {
            group->removeUvShell(item->getMeshData(), item->getUvShellId());
        }

        _parent->getGroup()->addUvShell(item->getMeshData(), item->getUvShellId());
    }
}

void GroupTreeWidgetItemEventFilter::onGroupDrop(QDropEvent* dropEvent)
{
    auto droppedGroupItem = getDroppedItem<GroupTreeWidgetItem>(dropEvent->mimeData()->data(MimeTypes::GROUP_TREE_WIDGET_ITEM));

    //See if the end user, in their infinite wisdom, dropped the group on itself, a parent on its child or a child on its parent
    if (droppedGroupItem == _parent ||
        droppedGroupItem->getGroup()->getParent() == _parent->getGroup() ||
        droppedGroupItem->getGroup() == _parent->getGroup()->getParent()) return;

    QSet<GroupTreeWidgetItem*> selectedGroupItems = expandDroppedItem<GroupTreeWidgetItem>(droppedGroupItem);

    for (GroupTreeWidgetItem* item : selectedGroupItems)
    {
        //Itself can still technically be selected
        if (droppedGroupItem == _parent ||
            droppedGroupItem->getGroup()->getParent() == _parent->getGroup() ||
            droppedGroupItem->getGroup() == _parent->getGroup()->getParent()) continue;

        item->getGroup()->move(_parent->getGroup());
    }
}

void GroupTreeWidgetItemEventFilter::onLineEditFinished()
{
    _parent->_ui->stackedWidget->setCurrentIndex(0);

    QString newName = _parent->_ui->lineEdit->text();
    if (newName.isEmpty()) return;

    _parent->_uvGroup->setName(newName);
    _parent->_ui->label->setText(newName);
}

void GroupTreeWidgetItemEventFilter::onCustomContextMenuRequested(const QPoint& pos)
{
    qDebug() << "Custom context menu requested";

    QMenu contextMenu(_widget);

    auto layoutAction = new QAction("Layout", _widget);
    auto recursiveLayoutAction = new QAction("Layout recursively", _widget);
    auto deleteAction = new QAction("Delete", _widget);

    contextMenu.addAction(layoutAction);
    contextMenu.addAction(recursiveLayoutAction);
    contextMenu.addSeparator();
    contextMenu.addAction(deleteAction);

    QObject::connect(layoutAction, &QAction::triggered, this, [=]()
    {
        qDebug() << "Layout action triggered";
        _parent->getGroup()->layout();
    });

    QObject::connect(recursiveLayoutAction, &QAction::triggered, this, [=]()
    {
        _parent->getGroup()->layoutRecursively();
    });

    QObject::connect(deleteAction, &QAction::triggered, this, [=]()
    {
        MeshManager::getInst()->deleteGroup(_parent->getGroup());
    });

    contextMenu.exec(_widget->mapToGlobal(pos));
}
