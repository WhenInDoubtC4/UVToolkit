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
}

GroupTreeWidgetItemEventFilter::GroupTreeWidgetItemEventFilter(GroupTreeWidgetItem* parent, QWidget* qObjectParent)
    : QObject(qObjectParent)
    , _parent(parent)
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
