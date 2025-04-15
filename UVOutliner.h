#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QStyledItemDelegate>
#include <QAbstractItemModel>
#include <QPainter>

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
#include "UVTreeWidgetItem.h"
#include "MeshData.h"

namespace Ui
{
class UVOutliner;
}

//FWD
class UVTreeWidgetItem;

class UVOutliner : public MayaQWidgetDockableMixin
{
    Q_OBJECT

public:
    explicit UVOutliner(QWidget* parent = nullptr);
    virtual ~UVOutliner();

    QTreeWidget* getTreeWidget();

    void addMesh(MeshData* mesh);
    UVTreeWidgetItem* addItem(const MDagPath& meshDagPath, unsigned int uvShellId, QTreeWidgetItem* parent = nullptr);
    void removeItem(const MDagPath& meshDagPath) const;

private:
    Ui::UVOutliner* ui;
    QWidget* _wrapper;

    MObject getUvsInShell(const MDagPath& dagPath, unsigned int shellIndex);
    MObject getFacesInShell(const MDagPath& dagPath, unsigned int shellIndex);

    void selectUVShell(const MDagPath& meshDagPath, unsigned int shellIndex, bool mergeWithExisting = false);

    inline static bool _isPerformingSelection = false;

    MCallbackId _selectionChangedCallbackId;
    MCallbackId _nodeAddedCallbackId;
    MCallbackId _nodeRemovedCallbackId;
    MCallbackId _sceneUpdatedCallbackId;

    void onSelectionChanged();
    void onNodeAdded(MObject& object);
    void onNodeRemoved(MObject& object);
    void onSceneUpdated();

    QSet<MeshData*> _meshData;

private slots:
    void onTreeWidgetItemSelectionChanged();
    void onUvShellAdded(MDagPath& mesh, unsigned int uvShellId);
};
