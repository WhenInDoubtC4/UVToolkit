#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <maya/MGlobal.h>

MainWindow::MainWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::OnButtonPressed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::OnButtonPressed()
{
    MGlobal::displayWarning("Hiiiiii");
}

MainWindowCmd::MainWindowCmd()
{

}

void* MainWindowCmd::creator()
{
    return new MainWindowCmd();
}

void MainWindowCmd::cleanup()
{
    if (_mainWindow) delete _mainWindow;
}

MStatus MainWindowCmd::doIt(const MArgList& argList)
{
    if (!_mainWindow)
    {
        _mainWindow = new MainWindow(MQtUtil::mainWindow());
        _mainWindow->show();
    }
    else
    {
        _mainWindow->raise();
    }

    return MStatus::kSuccess;
}
