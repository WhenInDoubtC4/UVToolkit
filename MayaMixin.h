//Maya2025/Python/Lib/site-packages/maya/app/general ported to C++
#pragma once

#include <QObject>
#include <QWidget>
#include <QMainWindow>
#include <QDockWidget>
#include <QCloseEvent>
#include <QDateTime>

#include <maya/MQtUtil.h>
#include <maya/MGlobal.h>

class MayaQWidgetBaseMixin : public QWidget
{
    Q_OBJECT
public:
    MayaQWidgetBaseMixin(QWidget* parent = nullptr);
    virtual ~MayaQWidgetBaseMixin() {};

    void initForMaya();
    void makeMayaStandaloneWindow();
    void show();
    void setVisible(bool makeVisible);

protected:
    bool _disableAutoParentingToMainWindow = false;
};

class MayaQWidgetDockableMixin : public MayaQWidgetBaseMixin
{
    Q_OBJECT
public:
    MayaQWidgetDockableMixin(QWidget* parent = nullptr);
    virtual ~MayaQWidgetDockableMixin();

    enum Area
    {
        A_TOP,
        A_LEFT,
        A_RIGHT,
        A_BOTTOM,
    };

    enum AllowedArea
    {
        TOP,
        LEFT,
        RIGHT,
        BOTTOM,
        ALL,
    };

    void setDockableParameters(bool dockable, bool floating, Area area, AllowedArea allowedArea, int width, int height, int x, int y, bool disableAutoParentingToMainWindow);

    bool isDockable();
    bool isVisible();

    void show();
    void hide();

    QWidget* getMayaControl();

    static void cleanup();

    QString getControlName();
    bool eventFilter(QObject* watched, QEvent* event);

signals:
    void closeEventTriggered();
    void windowStateChanged();

private:
    const QMap<Area, QString> AREA_MAP = {
        {Area::A_TOP, "top"},
        {Area::A_LEFT, "left"},
        {Area::A_RIGHT, "right"},
        {Area::A_BOTTOM, "bottom"}
    };

    QString _workspaceControlName;

    inline static QList<QString> _allControls;

private slots:
    void runDeleteCommand();
};
