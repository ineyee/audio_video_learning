#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

// 这一个Demo的目标是：在子线程中进行音频播放，播放上一步录制到的原始PCM数据
// 既然是播放，这一个Demo就用不到FFmpeg，只有SDL就够了，所以我们先去.pro文件里配置一下SDL库

// 一、开发播放功能的主要步骤是：
//
// 1、初始化SDL的子系统，我们这里要播放音频就得初始化SDL的音频子系统
// SDL分成好多个子系统（subsystem）：
// Video：显示和窗口管理
// Audio：音频设备管理
// Joystick：游戏摇杆控制
// Timers：定时器
// ...
// 将来有对应的退出SDL的子系统
//
// 2、打开设备
// 将来有对应的关闭设备
//
// 3、打开文件
// 将来有对应的关闭文件
//
// 4、开始播放
// 注意点一：播放音频是个耗时操作，一定要在子线程里执行
// 注意点二：播放音频的核心逻辑其实就是
// （1）从文件里读取特定字节数的音频数据出来，存进文件缓冲区
// （2）然后音频播放库（如SDL）就会从文件缓冲区里读取数据存入它的音频缓冲区，音频缓冲区里一旦有数据了、音频播放库（如SDL）就会把数据传递给音频播放设备（如SDL的音频子系统）的内存、音频播放设备（如SDL的音频子系统）就开始播放其内存里的数据了、此时音频缓冲区里的数据就已经没用了
// （3）然后音频播放设备（如SDL的音频子系统）就一直播一直播、等快播完这波数据时、音频播放库（如SDL）就会再次触发回到向我们拉取数据，于是又开始了下一波从文件里读数据、存进文件缓冲区、存进音频缓冲区、
// 此时上一波数据应该还没播放完、但是新的一波数据已经放在音频缓冲区里准备好了、音频播放库（如SDL）一等到音频播放设备（如SDL的音频子系统）内存里没数据了、就会把音频缓冲区里的新数据传递到给音频播放设备（如SDL的音频子系统）的内存、继续播放
// （4）就这样从文件里读一波数据、播放一波数据、连续不断、直到读完播完
// 【文件（磁盘）】 ----音频数据----> 【文件缓冲区（程序内存）】 ----音频数据----> 【音频缓冲区（程序内存）】 ----音频数据----> 【音频播放设备的内存】
// 注意点三：
// （1）文件读取线程和音频数据拉取线程是两个不同的子线程，但是两者都需要访问文件缓冲区，怎么搞？把文件缓冲区搞成全局的就可以了
// （2）文件缓冲区的大小怎么设置？通常搞成和音频播放库（如SDL）音频缓冲区的大小一样就可以了，这样我们每从文件里读一波数据，就可以刚好塞满音频缓冲区，以字节为单位
// 注意点四：如何保证文件读取线程和音频数据拉取线程之间的串行关系，以确保我们确实是读一波数据、播放一波数据，再读取下一波数据播放，而不出现播放错乱？
// 很简单，我们只要再搞一个全局的变量，来记录文件缓冲区里到底还剩下多少数据即可，这样两个线程就都能访问到这个变量
// 对于文件读取线程来说，只要这个变量是大于0的，那就代表文件缓冲区里还有数据，那文件读取线程就卡住不要再读取数据了，先等音频数据拉取线程把文件缓冲区里的数据消耗完再读取
// 对于音频数据拉取线程来说，只要这个变量是小于等于0的，那就什么都不做、放行去让文件读取线程继续去读取下一波数据，否则就读取文件缓冲区里的数据往音频缓冲区里塞，直到把文件缓冲区里的数据读完、变量小于等于0、从而放行
// 注意点五：记得处理最后一波数据还没播完，sdl播放资源就提前销毁的问题
// 注意文件读取线程只会等待音频数据拉取线程从文件缓冲区里搬数据这个操作，而不会等待播放器播放音频这个操作，也就是说播放器播放的同时，读文件线程已经在读取下一波数据了
// 因此就很有可能出现播放器还在播放最后一波数据，但是文件读取线程已经异步地判断到没有数据要读了，要销毁sdl播放资源了，这就会导致有一部分音频没正常播放出来
// 处理方式就是当读完最后一波数据后，播放器正在播放最后一波数据时，如果文件读取线程已经异步地判断到没有数据要读了，此时我们让文件读取线程睡眠“最后一波数据的时长”，然后再销毁sdl播放资源就可以了
//
// 5、释放资源（跟打开时倒着来）
// 关闭文件
// 关闭设备
// 退出SDL的子系统

// 二、需要用到SDL的1个库：
// 1、SDL
