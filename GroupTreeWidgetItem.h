#pragma once

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
    Ui::GroupTreeWidgetItem* _ui;
    MeshManager::UVGroup* _uvGroup;
};
