#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

// 一、音频重采样是什么？
// 把一段PCM数据转换成另一段PCM数据，如果这两段PCM数据的采样率、采样格式、声道数并不完全一样，那就是音频重采样。

// 二、为什么要进行音频重采样？
// 我们录下来的原始PCM数据的采样率、采样格式（包括采样大小、用整型还是float存储、大小端）、通道数是硬件录音设备的默认参数，当然也有可能是我们设置的参数
// 但是我们如果想对这段原始PCM数据数据进行编码压缩，不同的编码器却只能接收特定采样率、采样格式、通道数的PCM数据
// 所以两者很有可能对不上号，因此我们就得对最原始的PCM数据进行重采样，采样成编码器能接收的PCM数据
//
// 也就是说【我们在学习音频编解码之前必须先学会音频重采样，音频重采样是音频编解码的前置条件】

// 三、怎么进行音频重采样？
// 本篇的目标就是完成24000_s16le_1.pcm -> 48000_f32le_2.pcm重采样
//
// 首先音频重采样需要用到FFmpeg的两个库，去.pro文件里配置一下：
// swresample
// avutil
//
// 其它更具体的内容，详见ffmpegutil.cpp

// 四、音频重采样易错的点
// 1、重采样完成后，我们怎么验证重采样的对不对呢？
// 其实能用命令行正常播放出来是不严谨的，因为数据就算差那么几十个字节我们耳朵也是听不出来
// 更严谨的做法，我们可以去查看一下转换后的数据量对不对
// 比如24000_s16le_1.pcm的文件大小是290,816字节，那重采样后的48000_f32le_2.pcm的文件大小就应该是290,816 * 8 = 2,326,528 字节
// 因为采样率是原来的2倍、位深度也是原来的2倍、通道数也是原来的2倍，总大小可不就是原来的8倍嘛
// 但是重采样后的音频总时长是不会变的，你不能说原来是10秒的音频，重采样后变成20秒了，这就离谱了
//
// 当然查看文件大小也是大致估计，不要差太多就行了，因为并不是所有的重采样都像我们上面这个例子这样刚刚好就是原来的整数倍，具体的例子见下面“输出缓冲区大小的设定”

// 2、输出缓冲区样本数的设定（不然也可能丢音频数据）
// 我们的代码里输入文件24000_s16le_1.pcm的采样率为24000Hz，然后我们设定了输入缓冲区的大小是能存放1024个样本
// 但是我们设定输出文件48000_f32le_2.pcm的采样率是48000Hz，采样率变成了原来的两倍，就意味着将来文件里的样本数也是原来的两倍，
// 所以这次重采样其实就是通过一些算法在往原来的两个样本与样本之间插一个样本，
// 因此如果我们还是设定输出缓冲区的样本数也是1024的话，那这一次重采样操作就无法存下原来的1024个样本 +插入的1024个样本了
// 所以输入缓冲区样本数、输入文件的采样率和输出缓冲区样本数、输出文件的采样率之间其实是存在下面关系的：
//
// 输入缓冲区样本数    输入文件的采样率
// -------------- = --------------
// 输出缓冲区样本数    输出文件的采样率
//
// 即：输出缓冲区样本数 = 输入缓冲区样本数 * 输出文件的采样率 / 输入文件的采样率
//
// 当然在我们这个例子中：输出缓冲区样本数 = 1024 * 48000 / 24000 = 2048
// 刚好能整除的尽
//
// 但是如果我们要求输出文件的采样率为44100呢？
// 输出缓冲区样本数 = 1024 * 44100 / 24000 = 1024 * 1.8375 = 1881.6
// 算出个小数来
// 但缓冲区样本数怎么可能是小数，所以我们一般会向上取整、此时就不要是2的幂了，不然误差就太大了，直接向上取整搞成1882个样本数也差不了太多就行，从而确保一次重采样后输出缓冲区能放得下所有的样本
// 也正是因为这一点，我们上满才说“查看文件大小也是大致估计”，重采样确实会有误差
//
// 这个公式就不消我们自己去算了，还要管什么向上取整，FFMpeg已经给我们提供好了一个函数直接用就行，详见ffmpegutil.cpp

// 3、输出缓冲区数据残留问题的处理
// FFmpeg的函数注释里也说明了，由于某些原因重采样函数并不是那么完美地会把数据严谨地给我们写入到输出文件里，在最后一次处理的时候输出缓冲区可能会有数据残留，
// 我们需要处理一下，否则输出文件就会少掉一部分音频数据
// 详见ffmpegutil.cpp
