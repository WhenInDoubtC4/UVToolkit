#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStyledItemDelegate>
#include <QPainter>

#include <maya/MQtUtil.h>
#include <maya/MDagPath.h>

namespace Ui
{
class UVTreeWidgetItem;
}

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
