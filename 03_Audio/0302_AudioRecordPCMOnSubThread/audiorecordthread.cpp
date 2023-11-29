// 这个线程就是音频录制线程，专门用来完成音频录制这个耗时操作

#include "audiorecordthread.h"

#include <QDebug>
#include <QFile>
#include <QThread>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#define INPUT_FORMAT_NAME "avfoundation"
#define DEVICE_NAME ":1"
#define OUTPUT_FILE_PATH "/Users/yiyi/Desktop/output.pcm"

AudioRecordThread::AudioRecordThread(QObject *parent)
    : QThread{parent}
{
    // 监听一下音频录制线程执行结束的时候主动释放一下该音频线程对应的内存
    connect(this, &AudioRecordThread::finished,
            this, &AudioRecordThread::deleteLater);
}

AudioRecordThread::~AudioRecordThread() {
    // 告诉音频录制线程录完最后一帧就不要录了，可以执行结束了
//    setStop(true);
    requestInterruption();

    // 调一下这两个函数，固定写法，让线程稍微等一下、等线程执行结束再死，必须写在_stop = true;下面，否则就执行不到_stop = true;这一句了
    quit();
    wait();

    qDebug() << "这里还是主线程：" << QThread::currentThread();
    qDebug() << "音频线程生命周期结束、内存销毁了：" << this;
}

// 线程只要一启动就会自动调用它的run函数，run函数里的代码就是在子线程里执行的
// 所以我们可以直接把耗时操作放在run函数里，这样耗时操作就是在子线程里执行了
void AudioRecordThread::run() {
   qDebug() << "音频线程执行开始、开始录音了：" << QThread::currentThread();

   // 3、获取输入格式对象
   const AVInputFormat *inputFormat = av_find_input_format(INPUT_FORMAT_NAME);
   if (inputFormat == nullptr) {
       qDebug() << "获取输入格式对象失败，请确认输入格式名称：" << INPUT_FORMAT_NAME;
       return;
   }

   // 4、打开设备
   // 第一个参数：格式上下文（将来我们就是通过这个格式上下文来操作设备），参数是个二级指针，我们只需要把一个一级指针的地址传进去就可以了
   // 这里的意思是我们调用avformat_open_input函数，函数内部就会给我们传递进去的一级指针赋上值，后面我们就可以通过这个格式上下文来操作设备了
   // 这种做法在C和C++里是非常常见的，因为函数的返回值只能有一个，所以如果我们需要多个返回值时就可以通过把参数搞成指针的方式，在函数内部给指针赋值来实现，OC里的error就经常是这种做法
   AVFormatContext *formatContext = nullptr;
   // 第二个参数：设备名称，可以看做我们通过命令行录音时-i后面的东西
   // 这个demo我们就不做那么复杂了，这里就直接写死要用哪个设备了
   // 第三个参数：就是我们的输入格式对象
   // 第四个参数：一些配置，我们可以暂时传空
   // 返回值：0 on success, a negative AVERROR on failure.
   int ret = avformat_open_input(&formatContext, DEVICE_NAME, inputFormat, nullptr);
   if (ret < 0) {
       // 代表打开设备失败，那我们怎么获取失败的信息呢？有一个专门的函数：av_strerror
       // 第一个参数：错误码，也就是我们上面的ret
       // 第二个参数：一个buffer，我们传个数组进去，放错误码的字符，1024个字节足够了
       char errBuf[1024];
       // 第三个参数：buffer的大小
       av_strerror(ret, errBuf, sizeof(errBuf));
       qDebug() << "打开设备失败：" << errBuf;
   }

   // 5、打开文件
   // 我们采集到的音频数据需要存储到一个文件里，会用到Qt的一个类QFile
   // 同时我们采集到的音频数据是最原始的PCM数据，这里我们还牵涉不到编码，所以要把最原始的PCM数据给存下来，只能存储到.pcm文件或者.wav文件里，两者的区别就是.pcm文件没有文件头只有纯音频数据，而.wav文件有44个字节的文件头 + 音频数据
   // 但是如果想存到.wav文件里，我们就得手动去给这个文件一个字节一个字节的写文件头，这比较麻烦，我们后面再做，这里就直接把存进.pcm文件了
   // 直接用路径创建一个栈区的文件对象
   QFile file(OUTPUT_FILE_PATH);
   // 打开文件，打开文件的模式WriteOnly：只写模式，如果文件不存在，就创建文件；如果文件存在，就清空文件内容
   if (!file.open(QFile::WriteOnly)) {
       qDebug() << "打开文件失败，请确认文件路径：" << OUTPUT_FILE_PATH;
       // 关闭设备
       avformat_close_input(&formatContext);
       return;
   }

   // 6、用设备采集数据存进文件
   // 能到这里，代表我们的设备也打开了、文件也打开了，一切的准备工作都做好了，我们可以开始采集音频了
   // 注意采样率是告诉麦克风去按某个节奏采样，而我们的代码采集音频可不是按采样率一个样本一个样本采集的，而以帧为单位来采集的，这也是为什么我们调用的函数名为av_read_frame，
   // 你可以认为帧其实就是一个缓冲区，采样率那么高，我们总不能那么频繁地去采集音频然后往文件里写一个一个的样本吧，那磁盘寿命就缩短了
   // 这里我们假定采集240帧数据就停止采集，后面我们会去实现一个每隔10ms采集一帧的例子
   int count = 1;
   // 采集音频的API——av_read_frame——只允许我们把采集到的数据写入到AVPacket对象中——我们可以不把AVPacket翻译成一个包对象、直接翻译成一个帧对象比较好理解、其实一帧不就是很多个样本组成的数据包嘛——不能直接写到文件里
   // 所以我们每次采集一帧的数据其实都需要对应一个AVPacket帧对象，我们这里也是定义一个栈区的数据帧对象（当然你定义成指针和堆区对象也可以，只不过这里就是临时用一下这个对象，出了这个函数就不用了，
   // 所以没必要定义成指针，否则你还得自己去管理内存，这也可以回答我之前的疑问：对象到底定义成栈的还是堆的？只是局部用那就栈，省去自己管理内存的麻烦；如果不是一个函数局部用就堆，注意管理好内存），把采集到的每帧音频都存进去
   // 调用采集音频的函数时，只需要把这个帧对象的地址传进去就可以了，采集音频的函数内部就会往这个帧对象里写采集到的数据了
   // 注意现在我们写的所有的代码都是同步的，还没有涉及到多线程，所以代码绝对是顺序执行的，也就是说绝对是完完整整采集完一帧数据、然后再把这帧数据写入到文件里之后，才会开始下一帧数据的采集和存储
//   while (_stop == false) {
    while (isInterruptionRequested() == false) {
       AVPacket pkt;
       // 正如音频采集这个函数名av_read_frame所示，这个函数每调用一次就是采集一帧的音频数据
       // 从录音设备中采集一帧数据，返回值为0，代表这次采集成功
       ret = av_read_frame(formatContext, &pkt);
       if (ret == 0) { // 这一次采集成功
           qDebug() << "第" << count << "次采集成功";
           count++;
           // 将采集到的数据写入文件
           // pkt.data是个unsigned char *，即一个unsigned char的数组，*和数组一个意思嘛，都存储着那个数组的首地址，这里函数接收的类型是const char *，我们可以强转一下，问题不大，因为都是char类型，一个字节一个字节地往里写
           file.write((const char *)pkt.data, pkt.size);
       } else if (ret == AVERROR(EAGAIN)) { // Resource temporarily unavailable：资源临时不可用
           continue;
       } else {
           char errbuf[1024];
           av_strerror(ret, errbuf, sizeof(errbuf));
           qDebug() << "第" << count << "次采集出错：" << errbuf;
           break;
       }
       // 注意这句话跟pkt在栈区还是堆区没关系，只要你定义了pkt那就得这样释放一下pkt内部的资源
       av_packet_unref(&pkt);
   }

   // 7、释放资源
   // 关闭文件
   file.close();
   // 关闭设备
   avformat_close_input(&formatContext);

   qDebug() << "音频线程执行结束、停止录音了：" << QThread::currentThread();
}

//void AudioRecordThread::setStop(bool stop) {
//    _stop = stop;
//}

