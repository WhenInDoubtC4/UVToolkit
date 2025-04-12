#include "MayaMixin.h"

MayaQWidgetBaseMixin::MayaQWidgetBaseMixin(QWidget* parent)
    : QWidget(parent)
{
    initForMaya();
}

void MayaQWidgetBaseMixin::initForMaya()
{
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_DeleteOnClose);
}

void MayaQWidgetBaseMixin::makeMayaStandaloneWindow()
{
    QWidget* originalParent = parentWidget();

    if (!_disableAutoParentingToMainWindow)
    {
        auto mainWindow = dynamic_cast<QMainWindow*>(MQtUtil::mainWindow());
        setParent(mainWindow);
    }

    if (dynamic_cast<QDockWidget*>(this))
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    }
    else
    {
        setWindowFlags(Qt::Window);
    }

    if (originalParent) delete originalParent;
}

void MayaQWidgetBaseMixin::show()
{
    if (!parentWidget()) makeMayaStandaloneWindow();

    QWidget::show();
}

void MayaQWidgetBaseMixin::setVisible(bool makeVisible)
{
    if (makeVisible && !parentWidget())
    {
        makeMayaStandaloneWindow();
    }

    QWidget::setVisible(makeVisible);
}

MayaQWidgetDockableMixin::MayaQWidgetDockableMixin(QWidget* parent)
    : MayaQWidgetBaseMixin(parent)
{
    if (parent && parent == MQtUtil::mainWindow())
    {
        setParent(nullptr);
    }

    setAttribute(Qt::WA_DeleteOnClose);
}

MayaQWidgetDockableMixin::~MayaQWidgetDockableMixin()
{
    runDeleteCommand();
}

bool MayaQWidgetDockableMixin::isDockable()
{
    if (_workspaceControlName.length() > 0)
    {
        int result;
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -q -exists %1").arg(_workspaceControlName)), result);
        return static_cast<bool>(result);
    }

    return false;
}

void MayaQWidgetDockableMixin::setDockableParameters(bool dockable, bool floating, Area area, AllowedArea allowedArea, int width, int height, int x, int y, bool disableAutoParentingToMainWindow)
{
    _disableAutoParentingToMainWindow = disableAutoParentingToMainWindow;

    if (dockable || isDockable())
    {
        if (!isDockable())
        {
            if (x == 0) x = 250;
            if (y == 0) y = 250;

            QSize widgetSizeHint;
            QSize uninitializedSize(640, 480);
            if (size() == uninitializedSize)
            {
                widgetSizeHint = sizeHint();
            }
            else
            {
                widgetSizeHint = size();
            }

            if (width == 0)
            {
                width = widgetSizeHint.width();
            }
            if (height == 0)
            {
                height = widgetSizeHint.height();
            }

            QString workspaceControlName = QStringLiteral("\"%1%2WorkspaceControl\"").arg(objectName()).arg(QDateTime::currentMSecsSinceEpoch());
            if (floating)
            {
                if (minimumWidth() == 0)
                {
                    MString result;
                    MGlobal::executeCommand(MQtUtil::toMString(
                                                QStringLiteral("workspaceControl -label \"%1\" -retain true -loadImmediately true -floating true -initialWidth %2 -widthProperty free -initialHeight %3 -heightProperty free %4")
                                                    .arg(windowTitle())
                                                    .arg(width)
                                                    .arg(height)
                                                    .arg(workspaceControlName)), result);
                    workspaceControlName = MQtUtil::toQString(result);
                }
                else
                {
                    MString result;
                    MGlobal::executeCommand(MQtUtil::toMString(
                                                QStringLiteral("workspaceControl -label \"%1\" -retain true -loadImmediately true -floating true -initialWidth %2 -widthProperty free -minimumWidth %3 -initialHeight %4 -heightProperty free %5")
                                                    .arg(windowTitle())
                                                    .arg(width)
                                                    .arg(minimumWidth())
                                                    .arg(height)
                                                    .arg(workspaceControlName)), result);
                    workspaceControlName = MQtUtil::toQString(result);
                }
            }
            else
            {
                //If parented to the Maya main window or nothing, dock into the Maya main window
                if (!parentWidget() || parentWidget() == MQtUtil::mainWindow())
                {
                    if (minimumWidth() == 0)
                    {
                        MString result;
                        MGlobal::executeCommand(MQtUtil::toMString(
                                                    QStringLiteral("workspaceControl -label \"%1\" -retain true -loadImmediately true -dockToMainWindow %2 %3 -initialWidth %4 -widthProperty free -initialHeight %5 -heightProperty free %6")
                                                        .arg(windowTitle())
                                                        .arg(AREA_MAP[area])
                                                        .arg(false)
                                                        .arg(width)
                                                        .arg(height)
                                                        .arg(workspaceControlName)), result);
                        workspaceControlName = MQtUtil::toQString(result);
                    }
                    else
                    {
                        MString result;
                        MGlobal::executeCommand(MQtUtil::toMString(
                                                    QStringLiteral("workspaceControl -label \"%1\" -retain true -loadImmediately true -dockToMainWindow %2 %3 -initialWidth %4 -widthProperty free -minimumWidth %5 -initialHeight %6 -heightProperty free %7")
                                                        .arg(windowTitle())
                                                        .arg(AREA_MAP[area])
                                                        .arg(false)
                                                        .arg(width)
                                                        .arg(minimumWidth())
                                                        .arg(height)
                                                        .arg(workspaceControlName)), result);
                        workspaceControlName = MQtUtil::toQString(result);
                    }
                }
                //Otherwise, the parent should be within a workspace control - need to go up the hierarchy to find it
                else
                {
                    bool foundParentWorkspaceControl = false;
                    QWidget* nextParent = parentWidget();
                    while (nextParent)
                    {
                        QString dockToWorkspaceControlName = nextParent->objectName();
                        int result;
                        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -q -exists %1").arg(dockToWorkspaceControlName)), result);
                        if (static_cast<bool>(result))
                        {
                            if (minimumWidth() == 0)
                            {
                                MString result;
                                MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -label \"%1\" -retain true -loadImmediately true -dockToControl %2 %3 -initialWidth %4 -widthProperty free -initialHeight %5 -heightProperty free %6")
                                                                               .arg(windowTitle())
                                                                               .arg(workspaceControlName)
                                                                               .arg(AREA_MAP[area])
                                                                               .arg(width)
                                                                               .arg(height)
                                                                               .arg(workspaceControlName)), result);
                                workspaceControlName = MQtUtil::toQString(result);
                            }
                            else
                            {
                                MString result;
                                MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -label \"%1\" -retain true -loadImmediately true -dockToControl %2 %3 -initialWidth %4 -widthProperty free -minimumWidth %5 -initialHeight %6 -heightProperty free %7")
                                                                               .arg(windowTitle())
                                                                               .arg(workspaceControlName)
                                                                               .arg(AREA_MAP[area])
                                                                               .arg(width)
                                                                               .arg(minimumWidth())
                                                                               .arg(height)
                                                                               .arg(workspaceControlName)), result);
                                workspaceControlName = MQtUtil::toQString(result);
                            }
                            foundParentWorkspaceControl = true;
                            break;
                        }
                        else
                        {
                            nextParent = nextParent->parentWidget();
                        }
                    }

                    //If parent workspace control cannot be found, just make the workspace control a floating window
                    if (!foundParentWorkspaceControl)
                    {
                        if (minimumWidth() == 0)
                        {
                            MString result;
                            MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -label \"%1\" -retain true -loadImmediately true -floating true -initialWidth %2 -widthProperty free -initialHeight %3 -heightProperty free %4")
                                                                           .arg(windowTitle())
                                                                           .arg(width)
                                                                           .arg(height)
                                                                           .arg(workspaceControlName)), result);
                            workspaceControlName = MQtUtil::toQString(result);
                        }
                        else
                        {
                            MString result;
                            MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -label \"%1\" -retain true -loadImmediately true -floating true -initialWidth %2 -widthProperty free -minimumWidth %3 -initialHeight %4 -heightProperty free %5")
                                                                           .arg(windowTitle())
                                                                           .arg(width)
                                                                           .arg(minimumWidth())
                                                                           .arg(height)
                                                                           .arg(workspaceControlName)), result);
                            workspaceControlName = MQtUtil::toQString(result);
                        }
                    }
                }
            }

            QWidget* currentParent = MQtUtil::getCurrentParent();
            QWidget* mixin = MQtUtil::findControl(MQtUtil::toMString(objectName()));
            if (mixin)
            {
                MQtUtil::addWidgetToMayaLayout(mixin, currentParent);
            }

            _workspaceControlName = workspaceControlName;
        }
    }
    //Handle standalone window
    else
    {
        if (!dockable && isDockable())
        {
            QPoint dockPos = parentWidget()->pos();
            if (x == 0) x = dockPos.x();
            if (y == 0) y = dockPos.y();
            if (width == 0) width = this->width();
            if (height == 0) height = this->height();

            //Turn into a standalone window and reposition
            bool currentVisibility = isVisible();
            makeMayaStandaloneWindow();
            setVisible(currentVisibility);
        }

        //Handle position and sizing
        if (width > 0 || height > 0)
        {
            if (width == 0) width = this->width();
            if (height == 0) height = this->height();
            resize(width, height);
        }
        if (x > 0 || y > 0)
        {
            if (x == 0) x = this->x();
            if (y == 0) y = this->y();
            move(x, y);
        }
    }

    _allControls << _workspaceControlName;
}

void MayaQWidgetDockableMixin::show()
{
    if (!parentWidget())
    {
        makeMayaStandaloneWindow();
    }

    if (_workspaceControlName.length() > 0)
    {
        int exists_result;
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -q -exists %1").arg(_workspaceControlName)), exists_result);
        if (static_cast<bool>(exists_result))
        {
            int visible_result;
            MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -q -visible %1").arg(_workspaceControlName)), visible_result);
            if (static_cast<bool>(visible_result))
            {
                MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -e -restore %1").arg(_workspaceControlName)));
            }
            else
            {
                MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -e -visible true %1").arg(_workspaceControlName)));
            }
        }
    }
    else
    {
        QWidget::setVisible(false);
    }
}

void MayaQWidgetDockableMixin::hide()
{
    if (_workspaceControlName.length() > 0)
    {
        int exists_result;
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -q -exists %1").arg(_workspaceControlName)), exists_result);
        if (static_cast<bool>(exists_result))
        {
            MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -e -visible false %1").arg(_workspaceControlName)));
        }
    }
    else
    {
        QWidget::setVisible(false);
    }
}

bool MayaQWidgetDockableMixin::isVisible()
{
    if (_workspaceControlName.isEmpty()) return false;

    int exists_result;
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -q -exists %1").arg(_workspaceControlName)), exists_result);
    if (!static_cast<bool>(exists_result)) return false;

    int result;
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -q -visible %1").arg(_workspaceControlName)), result);
    return static_cast<bool>(result);
}

void MayaQWidgetDockableMixin::runDeleteCommand()
{
    int exists_result;
    MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("workspaceControl -q -exists %1").arg(_workspaceControlName)), exists_result);
    if (static_cast<bool>(exists_result))
    {
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("deleteUI -control %1").arg(_workspaceControlName)));
    }
}

QWidget* MayaQWidgetDockableMixin::getMayaControl()
{
    return MQtUtil::findControl(MQtUtil::toMString(_workspaceControlName));
}

QString MayaQWidgetDockableMixin::getControlName()
{
    return _workspaceControlName;
}

void MayaQWidgetDockableMixin::cleanup()
{
    while (!_allControls.isEmpty())
    {
        QString next = _allControls.takeLast();
        MGlobal::executeCommand(MQtUtil::toMString(QStringLiteral("deleteUI -control %1").arg(next)));
    }
}
