#include "mainwindow.h"
#include <QDebug>
#include <QPushButton>
#include "mybutton.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    qDebug() << "MainWindow::MainWindow";

    widgetBasicUse();
    widgetBasicUse1();
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow::~MainWindow";
}

// 一、控件的基本使用
void MainWindow::widgetBasicUse() {
    // 设置窗口的标题
    setWindowTitle("主窗口");

    // 设置窗口的位置
    // 以父控件的左上角为坐标原点
    // 没有父控件，就以屏幕的左上角为坐标原点
    move(100, 100);

    // 设置窗口大小
    // 不包括标题栏区域
    // resize函数设置窗口大小后，窗口依然可以拖动改变大小
//    resize(600, 600);

    // 设置窗口大小
    // 不包括标题栏区域
    // setFixedSize函数设置窗口大小后，窗口不可以拖动改变大小
    setFixedSize(600, 600);


    // 创建按钮
    // 注意：new出来的Qt控件是不需要程序员手动delete的，Qt内部会自动管理内存：当父控件销毁时，会顺带销毁子控件
    QPushButton *button = new QPushButton();
    // 先设置按钮的父控件
    button->setParent(this);
    // 设置按钮的位置
    // 以父控件的左上角为坐标原点
    // 没有父控件，就以屏幕的左上角为坐标原点
    button->move(50, 50);
    // 设置按钮的大小
    button->resize(100, 50);
    // 设置按钮的文本
    button->setText("登录");
}

void MainWindow::widgetBasicUse1() {
    // 用我们自定义的button来验证下“Qt内部会自动管理内存：当父控件销毁时，会顺带销毁子控件”
    MyButton *button = new MyButton();
    button->setParent(this);
    button->move(200, 50);
    button->resize(100, 50);
    button->setText("注册");
}

