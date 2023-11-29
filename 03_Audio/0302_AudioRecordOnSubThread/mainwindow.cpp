#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QThread>

// 导入我们自定义的音频录制线程
#include "audiorecordthread.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    qDebug() << "窗口销毁了";
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    // 但是我们会发现如果我们直接在这个函数里进行音频录制，App就会卡住，鼠标一直在那转圈
    // 这是因为当前线程为主线程，而音频录制是一个耗时操作，所以如果把音频录制放在主线程里来做就会卡住UI线程——即主线程
    // 因此我们需要把音频录制这个耗时操作放到子线程里去做，这需要用到Qt的一个类QThread
    // 但实际开发中更多的我们不会直接去使用QThread，而是自定义一个Thread来继承自QThread使用
    // 同时我们会发现QThread的构造函数里有一个parent对象，我们完全可以把当前界面作为QThread的parent，parent的作用就是
    // 当parent销毁时也会销毁当前这个对象，所以但凡有parent的地方我们都可以这么做，不限于QWidget，这样我们就不用手动管理
    // 子线程对象的生命周期了
    qDebug() << "这里是主线程：" << QThread::currentThread();

    AudioRecordThread *audioRecordThread = new AudioRecordThread(this);
    audioRecordThread->start();
}
