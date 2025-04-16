#include "UVTreeWidgetItem.h"
#include "ui_UVTreeWidgetItem.h"

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

    MFnTransform transform(_meshData->getDagPath().transform());

    _ui->label->setText(QStringLiteral("<b>%1</b> [%2]").arg(MQtUtil::toQString(transform.partialPathName())).arg(_uvShellId));
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
