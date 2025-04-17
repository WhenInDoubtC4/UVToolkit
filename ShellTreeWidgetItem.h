#pragma once

#include "UVTreeWidgetItem.h"
#include "MeshData.h"

namespace Ui
{
class ShellTreeWidgetItem;
}

class ShellTreeWidgetItem : public UVTreeWidgetItem
{
public:
    ShellTreeWidgetItem(QTreeWidget* parent);
    ShellTreeWidgetItem(QTreeWidgetItem* parent);
    virtual ~ShellTreeWidgetItem();

    void setupUi(QWidget* widget);

    void setMeshData(MeshData* data) { _meshData = data; };
    MeshData* getMeshData() const { return _meshData; };

    void setUvShellId(unsigned int id);
    unsigned int getUvShellId() const { return _uvShellId; };

private:
    Ui::ShellTreeWidgetItem* _ui;
    bool _isUiInit = false;

    MeshData* _meshData;
    unsigned int _uvShellId;
};
