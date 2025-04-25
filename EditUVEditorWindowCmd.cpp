#include "EditUVEditorWindowCmd.h"

EditUVEditorWindowCmd::EditUVEditorWindowCmd()
    : MPxCommand()
{

}

void* EditUVEditorWindowCmd::creator()
{
    return new EditUVEditorWindowCmd();
}

MSyntax EditUVEditorWindowCmd::syntax()
{
    MSyntax syntax;
    syntax.addFlag("-r", "-refresh");
    return syntax;
}

MStatus EditUVEditorWindowCmd::doIt(const MArgList& argList)
{
    MArgDatabase argData(syntax(), argList);
    if (argData.isFlagSet("-refresh"))
    {
        qDebug() << "Refresh the UV editor";
        return MStatus::kSuccess;
    }

    QWidget* uvEditorWidget = MQtUtil::findControl("polyTexturePlacementPanel1");
    uvEditorWidget->dumpObjectTree();

    QWidget* uvEditorWindow = MQtUtil::findWindow("polyTexturePlacementPanel1Window");

    auto stackedWidget = uvEditorWindow->findChildren<QStackedWidget*>().last();

    if (!stackedWidget)
    {
        qDebug() << "Could not find stacked widget";
        return MStatus::kFailure;
    }

    auto mdiArea = new UVEditorOverlayWindow(stackedWidget);
    mdiArea->show();
    mdiArea->raise();

    auto eventFilter = new OverlayEventFilter(stackedWidget, mdiArea);

    return MStatus::kSuccess;
}

OverlayEventFilter::OverlayEventFilter(QWidget* stackedWidget, UVEditorOverlayWindow* mdiArea)
    : QObject(stackedWidget)
    , _stackedWidget(stackedWidget)
    , _mdiArea(mdiArea)
{
    _parentWindow = _stackedWidget->window();

    _stackedWidget->installEventFilter(this);
    if (_parentWindow) _parentWindow->installEventFilter(this);
}

bool OverlayEventFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _stackedWidget)
    {
        switch (event->type())
        {
        //Show and hide the viewport within the widget instead of the entire widget to prevent propagating events and infinite loops
        case QEvent::Hide:
            _mdiArea->setViewportVisible(false);
            break;
        case QEvent::Show:
            _mdiArea->setViewportVisible(true);
            break;
        case QEvent::WindowDeactivate:
            _mdiArea->setViewportVisible(false);
            break;
        case QEvent::WindowActivate:
            _mdiArea->setViewportVisible(true);
            break;
        case DOCK_STATE_CHANGED_EVENT:
            //Check if the parent window changed
            if (_stackedWidget->window() == _parentWindow) break;

            //Reinstall the event filter on the new window
            _parentWindow->removeEventFilter(this);
            _parentWindow = _stackedWidget->window();
            if (_parentWindow) _parentWindow->installEventFilter(this);

            break;
        case QEvent::Resize:
            _mdiArea->resize(_stackedWidget->size());
            break;
        case QEvent::Move:
            _mdiArea->move(_stackedWidget->mapToGlobal(QPoint(0, 0)));
            break;
        }
    }
    else if (watched == _parentWindow && event->type() == QEvent::Move)
    {
        _mdiArea->move(_stackedWidget->mapToGlobal(QPoint(0, 0)));
        return true;
    }

    return false;
}
