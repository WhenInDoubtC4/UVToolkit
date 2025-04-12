#pragma once

#include <QWidget>
#include <QDialog>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMouseEvent>

#include <maya/MQtUtil.h>

namespace Ui {
class UVEditorOverlayWindow;
}

class UVEditorOverlayWindow : public QDialog
{
    Q_OBJECT

public:
    explicit UVEditorOverlayWindow(QWidget *parent = nullptr);
    ~UVEditorOverlayWindow();

    void setViewportVisible(bool visible = true);

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Ui::UVEditorOverlayWindow *ui;
};
