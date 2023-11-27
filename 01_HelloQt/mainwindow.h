#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
    void widgetBasicUse();
    void widgetBasicUse1();
    void signalAndSlot();
    void signalAndSlot1();
    void signalAndSlot2();
    void signalAndSlot3();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};
#endif // MAINWINDOW_H
