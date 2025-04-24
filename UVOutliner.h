#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QStyledItemDelegate>
#include <QAbstractItemModel>
#include <QPainter>
#include <QMenu>
#include <QAction>

#include <maya/MQtUtil.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MSelectionList.h>
#include <maya/MEventMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MItDag.h>
#include <maya/MNodeMessage.h>
#include <maya/MDagMessage.h>

#include "MayaMixin.h"
#include "Global.h"
#include "ShellTreeWidgetItem.h"
#include "GroupTreeWidgetItem.h"
#include "MeshData.h"
#include "MeshManager.h"
#include "GroupShellsCmd.h"
#include "LayoutAllCmd.h"

namespace Ui
{
class UVOutliner;
}

class UVOutliner : public MayaQWidgetDockableMixin
{
    Q_OBJECT

public:
    explicit UVOutliner(QWidget* parent = nullptr);
    virtual ~UVOutliner();

    QTreeWidget* getTreeWidget();

    ShellTreeWidgetItem* addItem(MeshData* meshData, unsigned int uvShellId, QTreeWidgetItem* parent = nullptr);
    GroupTreeWidgetItem* addItem(UVGroup* group, QTreeWidgetItem* parent = nullptr);
    void removeItem(MeshData* mesh) const;
    void removeItem(MeshData* mesh, unsigned int uvShellId);

private:
    Ui::UVOutliner* ui;
    QWidget* _wrapper;

    void selectUVShell(MeshData* meshData, unsigned int shellIndex, bool mergeWithExisting = false);
    void selectUVShellsRecursively(GroupTreeWidgetItem* root);
    void addExistingMeshesAndGroups();
    void reparentUvShellItem(UVGroup* group, MeshData* mesh, unsigned int shellIndex, bool addToGroup = true);

    inline static bool _isPerformingSelection = false;

    MCallbackId _selectionChangedCallbackId;
    MCallbackId _nodeAddedCallbackId;
    MCallbackId _nodeRemovedCallbackId;
    MCallbackId _sceneUpdatedCallbackId;

    void onSelectionChanged();
    void onNodeAdded(MObject& object);
    void onNodeRemoved(MObject& object);
    void onSceneUpdated();

private slots:
    void onTreeWidgetItemSelectionChanged();
    void onRefreshButtonClicked();
    void onGroupButtonClicked();
    void onLayoutAllButtonClicked();
    void onTreeWidgetContextMenuRequested(const QPoint& pos);
    void onUvShellAdded(MeshData* meshData, unsigned int uvShellId);
    void onUvShellIndexChanged(MeshData* meshData, unsigned int oldIndex, unsigned int newIndex);
    void onUvShellRemoved(MeshData* meshData, unsigned int index);
    void onUvDataRefreshed(MeshData* mesh);
    void onGroupCreated(UVGroup* group);
    void onUvShellAddedToGroup(UVGroup* group, MeshData* mesh, unsigned int shellIndex);
    void onUvShellRemovedFromGroup(UVGroup* group, MeshData* mesh, unsigned int shellIndex);
};
