#include "UVEditorOverlayWindow.h"
#include "ui_UVEditorOverlayWindow.h"

UVEditorOverlayWindow::UVEditorOverlayWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UVEditorOverlayWindow)
{
    ui->setupUi(this);

    //Make the window frameless and transparent
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setBackgroundRole(QPalette::NoRole);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    ui->mdiArea->setBackground(QBrush(QColor(255, 0, 0, 127)));
    ui->mdiArea->setActivationOrder(QMdiArea::CreationOrder);

    //QWidget* control = MQtUtil::findControl("polyTexturePlacementPanel1");

    parent->children().at(1)->installEventFilter(this);

    auto subWindow = new QWidget(ui->mdiArea);
    subWindow->setMinimumSize(QSize(200, 200));
    subWindow->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    ui->mdiArea->addSubWindow(subWindow);

    subWindow->raise();
}

UVEditorOverlayWindow::~UVEditorOverlayWindow()
{
    delete ui;
}

void UVEditorOverlayWindow::setViewportVisible(bool visible)
{
    ui->mdiArea->setVisible(visible);
}

bool UVEditorOverlayWindow::eventFilter(QObject* watched, QEvent* event)
{
    //Do not process events if the viewport is not visible
    if (!ui->mdiArea->isVisible()) QObject::eventFilter(watched, event);

    if (!(event->type() == QEvent::MouseButtonPress ||
          event->type() == QEvent::MouseButtonRelease ||
          event->type() == QEvent::MouseMove ||
          event->type() == QEvent::MouseButtonDblClick))
    {
        return false;
    }

    auto mouseEvent = static_cast<QMouseEvent*>(event);
    QPoint localPos = mouseEvent->pos();

    //Check if the event is over a sub window
    for (QMdiSubWindow* subWindow : ui->mdiArea->subWindowList())
    {
        if (!subWindow->isVisible() || !subWindow->geometry().contains(localPos)) continue;

        QPoint subWindowPos = subWindow->mapFrom(this,  localPos);
        QMouseEvent newEvent(mouseEvent->type(), subWindowPos, mapToGlobal(localPos), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers(), mouseEvent->pointingDevice());

        QApplication::sendEvent(subWindow, &newEvent);

        if (newEvent.isAccepted())  return true;
    }

    return QObject::eventFilter(watched, event);
}
