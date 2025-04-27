#pragma once

#include <QStackedWidget>
#include <QEvent>
#include <QMenu>
#include <QAction>
#include <QDragEnterEvent>
#include <QMimeData>

#include "UVTreeWidgetItem.h"
#include "MeshManager.h"
#include "ShellTreeWidgetItem.h"
#include "UVTreeWidget.h"

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

    virtual void setupUi(QWidget* widget) override;

    void setGroup(UVGroup* group) { _uvGroup = group; };
    UVGroup* getGroup() const { return _uvGroup; }

protected:
    virtual QDrag* onDrag() override;

private:
    friend class GroupTreeWidgetItemEventFilter;

    Ui::GroupTreeWidgetItem* _ui;
    QWidget* _widget;
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

    template<typename T>
    T* getDroppedItem(const QByteArray& dropData);

    template<typename T>
    QSet<T*> expandDroppedItem(T* droppedItem);

    void onShellDrop(QDropEvent* dropEvent);
    void onGroupDrop(QDropEvent* dropEvent);

public slots:
    void onLineEditFinished();
    void onCustomContextMenuRequested(const QPoint& pos);
};
