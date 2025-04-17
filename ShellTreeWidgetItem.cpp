#include "ShellTreeWidgetItem.h"
#include "ui_ShellTreeWidgetItem.h"

ShellTreeWidgetItem::ShellTreeWidgetItem(QTreeWidget* parent)
    : UVTreeWidgetItem(parent)
    , _ui(new Ui::ShellTreeWidgetItem)
{

}

ShellTreeWidgetItem::ShellTreeWidgetItem(QTreeWidgetItem* parent)
    : UVTreeWidgetItem(parent)
    , _ui(new Ui::ShellTreeWidgetItem)
{

}

ShellTreeWidgetItem::~ShellTreeWidgetItem()
{
    delete _ui;
}

void ShellTreeWidgetItem::setupUi(QWidget* widget)
{
    _ui->setupUi(widget);

    MFnTransform transform(_meshData->getDagPath().transform());

    _ui->label->setText(QStringLiteral("<b>%1</b> [%2]").arg(MQtUtil::toQString(transform.partialPathName())).arg(_uvShellId));
}
