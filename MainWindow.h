#pragma once

#include <QDialog>
#include <QWidget>

#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MQtUtil.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
    void OnButtonPressed();

};

class MainWindowCmd : public MPxCommand
{
public:
    MainWindowCmd();
    virtual ~MainWindowCmd() {};

    virtual MStatus doIt(const MArgList& argList);

    inline static const char* kCmdName = "launchMainWindow";

    static void* creator();

    static void cleanup();

private:
    inline static QWidget* _mainWindow = nullptr;
};
