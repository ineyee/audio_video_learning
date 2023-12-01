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

// 有了上一篇pcm2wav的知识基础，这一篇我们的目标就是：录制到PCM数据，直接存进.wav文件，而非存进.pcm文件，因为.pcm文件没法发送给别人播放
// 所以如果我们的目标是输出一个.wav文件的话，用这种做法输出.wav文件，相对于上一篇的“先录制成.pcm文件再转换成.wav文件”的做法，效率要更高一些

// 然后我们再添加个记录录音时长功能，注意不要用timer去记录录音时长，因为timer和录音线程是在两个线程里的，两者之间的同步性很成问题，
// 所以用timer记录出来的时间绝对不准，我们应该直接用采集到的每帧音频时长累加起来才是最准确的
// 当然“采集到的每帧音频时长”是在音频录制线程里才能获取到的，我们如果想把这个数据交给主线程，可以采用回调或者信号槽的方式，这里我们就采用信号槽来实现一下
// 不过需要注意的是这种做法只能用在记录PCM数据的情况下，如果是传输编码后的数据就没法这么算，因为编码后数据的存储方式都应改变了

// label注意搞下等宽字体，不然录音时长会看着闪来闪去

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
// 6、开始录音：av_read_frame
// 直接录成.wav文件的核心逻辑无非就是在之前录音的基础上前面加一步、后面加一步：
// （1）在采集音频数据到文件里之前，先给文件写进去一个.wav文件头
// （2）从设备采集一帧的数据，存进文件AVPacket（类似于文件缓冲区的作用）
// （3）将AVPacket里的数据写入文件
// （4）就这样从设备采集一帧、往文件里写入一帧、如此循环
// （5）在采集完音频数据之后，返回去更新一下.wav文件头里的两个size参数
//
// 7、释放资源（跟打开时倒着来）
// 关闭文件
// 关闭设备

// 二、需要用到FFmpeg的3个库：
// 1、avdevice：包含设备相关的API
// 2、avformat：包含格式相关的API
// 3、avutil：包含工具相关的API（比如错误处理）