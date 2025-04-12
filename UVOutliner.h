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

#include "MayaMixin.h"

namespace Ui {
class UVOutliner;
class UVTreeWidgetItem;
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

    UVTreeWidgetItem* addItem(const MDagPath& meshDagPath, unsigned int uvShellId, QTreeWidgetItem* parent = nullptr);

private:
    Ui::UVOutliner* ui;
    QWidget* _wrapper;

    MCallbackId _selectionChangedCallbackId;

    MObject getUvsInShell(const MDagPath& dagPath, unsigned int shellIndex);
    MObject getFacesInShell(const MDagPath& dagPath, unsigned int shellIndex);

    void selectUVShell(const MDagPath& meshDagPath, unsigned int shellIndex, bool mergeWithExisting = false);

    void onSelectionChanged();
    static void onSelectionChanged_wrapper(void* clientData);

    inline static bool _isPerformingSelection = false;

private slots:
    void onTreeWidgetItemSelectionChanged();
};

class UVTreeWidgetItem : public QTreeWidgetItem
{
    //Q_OBJECT

public:
    UVTreeWidgetItem(QTreeWidget* parent);
    UVTreeWidgetItem(QTreeWidgetItem* parent);
    virtual ~UVTreeWidgetItem();

    enum SelectionState
    {
        NotSelected,
        PartiallySelected,
        FullySelected
    };

    SelectionState getSelectionState() const { return _selectionState; };
    void setSelectionState(const SelectionState& state)
    {
        treeWidget()->update();
        treeWidget()->viewport()->update();
        _selectionState = state;
    };

    void setDagPath(const MDagPath& dagPath) { _dagPath = dagPath; };
    void setUvShellId(unsigned int id) { _uvShellId = id; };

    MDagPath getDagPath() const { return _dagPath; };
    unsigned int getUvShellId() const { return _uvShellId; };

    void setupUi(QWidget* widget);

private:
    Ui::UVTreeWidgetItem* _ui;

    SelectionState _selectionState;

    MDagPath _dagPath;
    unsigned int _uvShellId;
};

// Custom delegate to paint the items based on their selection state
class UVTreeWidgetItemDelegate : public QStyledItemDelegate
{
public:
    UVTreeWidgetItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
