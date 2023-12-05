#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "transformthread.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_transformButton_clicked()
{
    // 音频解码操作也是一个耗时操作，我们也给它专门搞一个线程
    TransformThread *transformThread = new TransformThread(this);
    transformThread->start();
}
