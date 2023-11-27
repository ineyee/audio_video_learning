#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // 创建一个QApplication对象
    // 一个Qt应用程序中只有一个QApplication对象
    QApplication a(argc, argv);

    // 创建一个主窗口MainWindow对象
    MainWindow w;
    // 显示主窗口
    w.show();

    // 运行Qt应用程序
    return a.exec();
}
