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
    // 注意点一：文件格式转换操作是个耗时操作，一定要在子线程里执行。
    TransformThread *transformThread = new TransformThread(this);
    transformThread->start();
}
