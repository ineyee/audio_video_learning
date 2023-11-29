#include "mainwindow.h"

#include <QApplication>

// FFmpeg是C语言编写的框架
// 所以C++里如果想调用C语言的函数，得让这些函数C语言的方式去编译，否则这些函数就是用C++的方式编译的，两种编译方式的函数
// 符号是不一样的，会导致找不到函数的实现
extern "C" {
#include <libavdevice/avdevice.h>
}

int main(int argc, char *argv[])
{
    // 2、注册设备
    avdevice_register_all();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

// 一、开发录音功能的主要步骤是：
//
// 1、添加麦克风使用权限的申请
// 这个需要我们自己在项目的根目录下创建一个Info.plist文件，然后还需要在.pro文件里配置一下，注意先删掉之前的构建重新运行才能生效
//
// 2、注册设备：avdevice_register_all
// 在整个程序的运行过程中，只需要执行1次注册设备的代码，没必要反复注册，而且我们最好是在App启动的时候就注册好设备
// 因此会放在main.cpp里去注册设备
//
// 3、获取输入格式对象：av_find_input_format
// 所谓获取输入格式对象，其实就是获取相应平台下的多媒体系统库，比如avfoundation、dshow等
// 我们只需要给一个正确的输入格式名称就能获取到相应的输入格式对象
//
// 4、打开设备：avformat_open_input
// 将来有对应的关闭设备
//
// 5、打开文件：file.open(QFile::WriteOnly)
// 将来有对应的关闭文件
//
// 6、用设备采集数据存进文件：av_read_frame
//
// 7、释放资源（跟打开时倒着来）
// 关闭文件
// 关闭设备

// 二、需要用到FFmpeg的3个库：
// 1、avdevice：包含设备相关的API
// 2、avformat：包含格式相关的API
// 3、avutil：包含工具相关的API（比如错误处理）
