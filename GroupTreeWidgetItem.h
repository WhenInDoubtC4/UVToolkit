#pragma once

#include <QStackedWidget>
#include <QEvent>
#include <QMenu>
#include <QAction>

#include "UVTreeWidgetItem.h"
#include "MeshManager.h"

namespace Ui
{
class GroupTreeWidgetItem;
}

class GroupTreeWidgetItem : public UVTreeWidgetItem
{
public:
    GroupTreeWidgetItem(QTreeWidget* parent);
    GroupTreeWidgetItem(QTreeWidgetItem* parent);
    virtual ~GroupTreeWidgetItem();

    void setupUi(QWidget* widget);

    void setGroup(UVGroup* group) { _uvGroup = group; };
    UVGroup* getGroup() const { return _uvGroup; }

private:
    friend class GroupTreeWidgetItemEventFilter;

    Ui::GroupTreeWidgetItem* _ui;
    UVGroup* _uvGroup;
    QString getSafeGroupName();
};

class GroupTreeWidgetItemEventFilter : public QObject
{
    Q_OBJECT

public:
    GroupTreeWidgetItemEventFilter(GroupTreeWidgetItem* parent, QWidget* widget);

private:
    GroupTreeWidgetItem* _parent;
    QWidget* _widget;

    bool eventFilter(QObject* watched, QEvent* event);

public slots:
    void onLineEditFinished();
    void onCustomContextMenuRequested(const QPoint& pos);
};
