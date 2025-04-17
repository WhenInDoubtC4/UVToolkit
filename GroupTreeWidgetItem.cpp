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

void GroupTreeWidgetItem::setupUi(QWidget* widget)
{
    _ui->setupUi(widget);

    QString groupName = _uvGroup->getName();
    if (groupName.isEmpty()) groupName = QStringLiteral("uvGroup%1").arg(_uvGroup->getId());

    _ui->label->setText(groupName);
}
