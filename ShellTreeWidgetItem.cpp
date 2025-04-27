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
    _widget = widget;
    _ui->setupUi(widget);

    MFnTransform transform(_meshData->getDagPath().transform());

    _ui->label->setText(QStringLiteral("<b>%1</b> [%2]").arg(MQtUtil::toQString(transform.partialPathName())).arg(_uvShellId));

    setupDragAndDrop(widget, _ui->iconLabel);

    _isUiInit = true;
}

void ShellTreeWidgetItem::setUvShellId(unsigned int id)
{   
    _uvShellId = id;

    if (!_isUiInit) return;

    MFnTransform transform(_meshData->getDagPath().transform());

    _ui->label->setText(QStringLiteral("<b>%1</b> [%2]").arg(MQtUtil::toQString(transform.partialPathName())).arg(_uvShellId));
}

QLabel* ShellTreeWidgetItem::getIcon() const
{
    return _ui->iconLabel;
}

QDrag* ShellTreeWidgetItem::onDrag()
{
    auto drag = new QDrag(_widget);
    auto mimeData = new QMimeData();

    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream << reinterpret_cast<unsigned long long>(this);

    mimeData->setData(MimeTypes::SHELL_TREE_WIDGET_ITEM, byteArray);
    drag->setMimeData(mimeData);

    return drag;
}
