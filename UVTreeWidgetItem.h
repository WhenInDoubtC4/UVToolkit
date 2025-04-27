#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>

#include <maya/MQtUtil.h>
#include <maya/MDagPath.h>
#include <maya/MFnTransform.h>

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
    void setSelectionState(const SelectionState& state, bool propagateToChildren = false, bool propagateToParents = false);

    bool isDropTarget() const { return _isDropTarget; };
    void setDropTarget(bool dropTarget) { _isDropTarget = dropTarget; };

    virtual void setupUi(QWidget* widget) = 0;
    virtual void setupDragAndDrop(QWidget* widget, QWidget* dragHandle);

    void setupWrapperWidget(QTreeWidget* parent);

protected:
    virtual QDrag* onDrag() { return nullptr; };

private:
    friend class UVTreeWidgetItemEventFilter;

    SelectionState _selectionState;
    bool _isDropTarget = false;
};

// Custom delegate to paint the items based on their selection state
class UVTreeWidgetItemDelegate : public QStyledItemDelegate
{
public:
    UVTreeWidgetItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

class UVTreeWidgetItemEventFilter : public QObject
{
public:
    UVTreeWidgetItemEventFilter(UVTreeWidgetItem* parent, QWidget* widget, QWidget* dragHandle);

private:
    UVTreeWidgetItem* _parent;
    QWidget* _dragHandle;

    QPoint _dragStartPosition;
    bool _handlePressed = false;
    bool _dragStarted = false;

    bool eventFilter(QObject* watched, QEvent* event);
};
