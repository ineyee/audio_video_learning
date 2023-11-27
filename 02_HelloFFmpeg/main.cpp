#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

// FFmpeg是C语言编写的框架
// 所以C++里如果想调用C语言的函数，得让这些函数按C语言的方式去编译，否则这些函数就是用C++的方式编译的，两种编译方式的函数
// 符号是不一样的，会导致找不到函数的实现
extern "C" {
#include <libavcodec/avcodec.h>
}

int main(int argc, char *argv[])
{
    // 打印版本号
    qDebug() << av_version_info(); // 6.0

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

// 这一篇，我们的目标是把FFmpeg顺利集成到这个QT项目中，能打印出FFmpeg的版本号来就算成功。
//
// 打开FFmpeg的安装目录：/usr/local/Cellar/ffmpeg，里面有三个关键的文件夹：
// bin（binary）：里面存放的是FFmpeg提供给我们的一些可执行文件，将来我们可以通过命令行来使用这些可执行文件
// include：里面存放的是FFmpeg各种函数的头文件，将来我们就是通过导入这里面的头文件来使用这些函数
// lib（library）：里面存放的是FFmpeg各种函数的实现，当然都打包成了库，一份静态库（.a）和一份动态库（.dylib）

// 知道了这三个目录，接下来我们就去.pro文件（项目配置文件）里去配置一下项目吧。
