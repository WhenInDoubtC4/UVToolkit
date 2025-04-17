#pragma once

#include <QStackedWidget>
#include <QEvent>

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

    void setGroup(MeshManager::UVGroup* group) { _uvGroup = group; };
    MeshManager::UVGroup* getGroup() const { return _uvGroup; };

private:
    friend class GroupTreeWidgetItemEventFilter;

    Ui::GroupTreeWidgetItem* _ui;
    MeshManager::UVGroup* _uvGroup;

    QString getSafeGroupName();
};

class GroupTreeWidgetItemEventFilter : public QObject
{
    Q_OBJECT

public:
    GroupTreeWidgetItemEventFilter(GroupTreeWidgetItem* parent, QWidget* qObjectParent);

private:
    GroupTreeWidgetItem* _parent;

    bool eventFilter(QObject* watched, QEvent* event);

public slots:
    void onLineEditFinished();
};
