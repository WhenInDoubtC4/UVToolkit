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
    _ui->setupUi(widget);
    _ui->label->setText(getSafeGroupName());

    auto eventFilter = new GroupTreeWidgetItemEventFilter(this, widget);
    widget->installEventFilter(eventFilter);
    QObject::connect(_ui->lineEdit, &QLineEdit::editingFinished, eventFilter, &GroupTreeWidgetItemEventFilter::onLineEditFinished);
    QObject::connect(_ui->lineEdit, &QLineEdit::inputRejected, eventFilter, &GroupTreeWidgetItemEventFilter::onLineEditFinished);

    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(widget, &QWidget::customContextMenuRequested, eventFilter, &GroupTreeWidgetItemEventFilter::onCustomContextMenuRequested);
}

GroupTreeWidgetItemEventFilter::GroupTreeWidgetItemEventFilter(GroupTreeWidgetItem* parent, QWidget* widget)
    : QObject(widget)
    , _parent(parent)
    , _widget(widget)
{

}

bool GroupTreeWidgetItemEventFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::MouseButtonDblClick) return false;

    if (_parent->_ui->stackedWidget->currentIndex() != 0) return false;

    _parent->_ui->stackedWidget->setCurrentIndex(1);
    _parent->_ui->lineEdit->setText(_parent->getSafeGroupName());
    _parent->_ui->lineEdit->setFocus();
    _parent->_ui->lineEdit->selectAll();

    //Eat the event so that the tree doesn't get collapsed or expanded
    return true;
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

    contextMenu.exec(_widget->mapToGlobal(pos));
}
