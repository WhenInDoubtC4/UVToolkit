#include "UVTreeWidget.h"

UVTreeWidget::UVTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{

    QObject::connect(this, &QTreeWidget::customContextMenuRequested, this, &UVTreeWidget::onCustomContextMenuRequested);
}

void UVTreeWidget::recreateWidgetsRecursive(UVTreeWidgetItem* root)
{
    root->setupWrapperWidget(this);

    for (int i = 0; i < root->childCount(); i++)
    {
        auto childItem = dynamic_cast<UVTreeWidgetItem*>(root->child(i));
        if (!childItem) continue;

        recreateWidgetsRecursive(childItem);
    }
}

void UVTreeWidget::onCustomContextMenuRequested(const QPoint& pos)
{
    QMenu contextMenu(this);

    auto expandAllAction = new QAction("Expand all", this);
    auto collapseAllAction = new QAction("Collapse all", this);

    contextMenu.addAction(expandAllAction);
    contextMenu.addAction(collapseAllAction);

    QObject::connect(expandAllAction, &QAction::triggered, this, &QTreeWidget::expandAll);
    QObject::connect(collapseAllAction, &QAction::triggered, this, &QTreeWidget::collapseAll);

    contextMenu.exec(mapToGlobal(pos));
}
