#pragma once

#include <QLabel>
#include <QMimeData>

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

    virtual void setupUi(QWidget* widget) override;

    void setMeshData(MeshData* data) { _meshData = data; };
    MeshData* getMeshData() const { return _meshData; };

    void setUvShellId(unsigned int id);
    unsigned int getUvShellId() const { return _uvShellId; };

    QLabel* getIcon() const;

protected:
    virtual QDrag* onDrag() override;

private:
    Ui::ShellTreeWidgetItem* _ui;
    bool _isUiInit = false;
    QWidget* _widget;

    MeshData* _meshData;
    unsigned int _uvShellId;
};
