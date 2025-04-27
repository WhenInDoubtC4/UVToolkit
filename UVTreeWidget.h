#pragma once

#include <QTreeWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>

#include "UVTreeWidgetItem.h"

class UVTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    UVTreeWidget(QWidget* parent = nullptr);

    void recreateWidgetsRecursive(UVTreeWidgetItem* root);

private slots:
    void onCustomContextMenuRequested(const QPoint& pos);

private:
    QPoint _dragStartPosition;
};
