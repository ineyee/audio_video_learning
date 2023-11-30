// 这个线程就是音频播放线程，专门用来完成音频播放这个耗时操作

#include "audioplaythread.h"

#include <SDL2/SDL.h>
#include <QDebug>
#include <QFile>

// 假设我们要播放我们桌面上的一个in.pcm文件，这个文件就是我们上一步录下来的.pcm文件，这个文件是用我的MacMini自带的0号下标设备录的
// PCM数据的参数如下
#define PCM_FILE_PATH "/Users/yiyi/Desktop/in.pcm"
// 采样率48000
#define PCM_SAMPLE_RATE 48000
// 采样格式f32le：里面包含采样大小、整数存还是小数存、大小端这些信息
#define PCM_SAMPLE_FORMAT AUDIO_F32LSB
// 声道数1
#define PCM_CHANNELS 1

// 音频播放库音频缓冲区的大小，这个音频缓冲区的大小是以样本为单位的、其实以样本为单位不就跟以字节为单位一样嘛、每个样本大小我们也是传进去了的（注意不是帧，录音的时候才是一帧一帧以帧为单位）
// 必须得是2的幂，根据普遍的开发经验来说，通常都是给512或1024，一般也不会搞得太大，这样第一次播放的时候响应速度也会快些，同时也可以避免播完了上一波数据、下一波数据还没准备好的问题
// 所以按我们现在这个PCM数据的参数来计算，一个样本的大小 = 32bit = 4Byte，音频缓冲区的大小 = 4 * 1024 = 4096Byte
#define AUDIO_BUFFER_LENGTH 1024

// 采样大小，这个宏会取出AUDIO_F32LSB里的16或32来
#define PCM_SAMPLE_SIZE (PCM_SAMPLE_FORMAT & 0xFF)
// 每个样本占用多少个字节
#define BYTES_PER_SAMPLE ((PCM_SAMPLE_SIZE * PCM_CHANNELS) / 8)
// 文件缓冲区的大小，单位为字节（注意不是帧，录音的时候才是一帧一帧以帧为单位）
// 文件缓冲区的大小可以等于音频缓冲区的大小，我们这里就是这么设置的，这样我们每从文件里读一波数据，就可以刚好塞满音频缓冲区
// 但不一定非得等于音频缓冲区的大小
// 可以小于音频缓冲区的大小，但这样就得读好几次文件才能填满音频缓冲区
// 可以大于音频缓冲区的大小——比如搞个两三倍，但如果设置太大的话，读取一次数据的时间可能过长，就有可能导致播完了上一波数据、下一波数据还没准备好的问题
// 录音那里是的没有文件缓冲区的，是直接从设备采集到数据就存进文件里了
// 但其实Qt提供的那个AVPacket对象就像是一个文件缓冲区，我们也是先把从设备采集到的数据存进AVPacket，然后再把AVPacket里的数据存进文件嘛，只不过这个缓冲区的不是我们自己定义的，它的大小不是我们控制的而已，
// 录音时也可能仅仅是在Qt里有AVPacket这么个自带的文件缓冲区，其它平台录音时可能也是要得我们自己定义文件缓冲区的
#define FILE_BUFFER_SIZE (BYTES_PER_SAMPLE * AUDIO_BUFFER_LENGTH)

// 全局的文件缓冲区
// 搞成全局的是为了保证两个线程都得能访问到
// 搞成指针是为了做下标偏移
char *fileBufferData;
// 某一次从文件里真正读取到了多少个字节的数据，即某一次文件缓冲区里真正有多少个字节的数据
// length不同于size，size是固定不变的，length可能小于size
int fileBufferLength = 0;

AudioPlayThread::AudioPlayThread(QObject *parent)
    : QThread{parent}
{
    // 监听一下音频播放线程执行结束的时候主动释放一下该音频线程对应的内存
    connect(this, &AudioPlayThread::finished,
            this, &AudioPlayThread::deleteLater);
}

AudioPlayThread::~AudioPlayThread() {
    // 告诉音频播放线程拿完这一波数据就不要拿了，可以执行结束了
    requestInterruption();

    // 调一下这两个函数，固定写法，让线程稍微等一下、等线程执行结束再死，必须写在requestInterruption();下面，否则就执行不到requestInterruption();这一句了
    quit();
    wait();

    qDebug() << "音频播放线程销毁了" << this;
}


// 这里是音频拉取线程：【文件缓冲区（程序内存）】 ----音频数据----> 【音频缓冲区（程序内存）】
// 【音频播放设备】主动向【我们的应用程序】拉取数据的回调，我们只要一调用SDL_PauseAudio(0)开始播放这个函数，SDL内部就会不断地触发它的“spec.callback”函数来向我们的应用程序要数据
//
// 第一个参数userdata：我们暂时不用关心
// 第二个参数stream：这个stream就是音频设备的inpuStream，即音频缓冲区，我们需要把文件缓冲区里面的PCM数据写进这个音频缓冲区里，以便SDL把数据传递给音频子系统、然后播放
// 第三个参数len：【音频播放设备】每次向【我们的应用程序】拉取数据时，希望我们给音频缓冲区写入多少个字节的的数据——一般都是希望填满音频缓冲区，
// 也就是说【音频播放设备】每次调用该回调会希望拉取4096个字节的数据来填满音频缓冲区，
// 但是具体【我们的应用程序】能不能满足【音频播放设备】的希望，那就不一定了，比如刚开始的音频数据是比较多的，【音频播放设备】第一次向
// 我们的【我们的应用程序】要数据的确要到了4096个字节，第二次要也要到了，
// 但是第三次音频数据本身不够4096个字节了，只剩下2个字节了，【我们的应用程序】就满足不了【音频播放设备】的希望了，那就填不满的音频缓冲区了
void sdl_pull_data_callback(void *userdata, Uint8 *stream, int len) {
    // sdl每次来要数据之前，先清空一下sdl的音频缓冲区，以免上一次读取的数据残留，导致播的乱七八糟，直接写成0就可以了
    // sdl播放的是设备的内存里的数据，这份数据其实没用了，所以才能清0，要是sdl播的就是音频缓冲区的数据，那即将播完时来向我们要数据时，我们清空数据就是错的了
    // 写成0相当于是静音处理
    SDL_memset(stream, 0, len);

    // 代表文件读取线程还没把数据写进文件缓冲区里，那就什么也不做，相当于是在等待文件读取线程把数据写进文件缓冲区
    // 因为两个线程是并行执行的嘛，这个线程执行到这就return相当于啥也没做，相当于这个线程是在空跑，可不就是相当于是在等待文件读取线程把数据写进文件缓冲区嘛
    if (fileBufferLength <= 0) {
        return;
    }

    // 到底给音频缓冲区写多少个字节，这个要取一下音频缓冲区的大小和我们真实读到一波数据大小的最小值，避免写得太多
    // 比如这里我们音频缓冲区的大小为4096个字节，那刚开始的时候fileBufferLength可能是4096个字节，但是读到最后2个字节的时候，fileBufferLength就是2了，就不能再往音频缓冲区写入4096个字节了
    // 同理，如果我们设置一波数据是要从文件里读取4096 * 4个字节，那fileBufferLength就是4096 * 4个字节，我们往音频缓冲区里写就应该写比较小的len，而不能直接把4096 * 4个字节全部写进去
    // 当然我们这个例子里只不过刚好设置了fileBufferLength就是音频缓冲区的大小len，所以直接写入fileBufferLength字节也可以，但是为了后续的扩展，还是写成取最小值的方式比较好
    int length = len < fileBufferLength ? len : fileBufferLength;

    // 能走到这里，代表文件读取线程已经把数据写进文件缓冲区里了
    // sdl就可以从文件缓冲区里拉取length个字节的数据到音频缓冲区了
    // 音频缓冲区里一旦有了数据，sdl就会把数据传递给音频播放设备（如SDL的音频子系统）的内存、音频播放设备就开始播放其内存里的数据了、此时音频缓冲区里的数据就已经没用了
    // 但是sdl里的处理是，音频播放设备正在播放时，这个函数会先卡主，不立马返回，直到即将播完时，这个函数才返回
    // 最后一个参数是软件设置的音量、我们设置为最大就可以了
    SDL_MixAudio(stream, (Uint8 *)fileBufferData, length, SDL_MIX_MAXVOLUME);

    // 这个 += 也是同样的道理，也是因为有可能我们会设置一次从文件读取的字节数大于sdl音频缓冲区的大小，那音频播放设备就得分几次才能把全局data数组里的数据读完了
    // 我们就得每次改变一下音频播放设备每次从fileBufferData的新的下标处开始读
    // 当然我们这个例子里是dataList刚好就是音频缓冲区的大小，一次就刚好能读完，不用 += 也行，但为了以后的扩展性还是用 += 吧
    fileBufferData += length;
    // 音频设备每次会往音频缓冲区里写入len字节，因此我们这里就让fileBufferLength减去一点
    // 其实这里我们直接让fileBufferLength = 0也可以，因为现在这个demo我们设置了一次从文件读取的字节数就等于音频缓冲区的大小
    // 但也有可能我们会设置一次从文件读取的字节数大于音频缓冲区的大小，那音频设备就得分几次才能把全局data数组里的数据读完了，就得像这里一样减了
    // 这样就可以激活下面的文件读取线程从文件里读取下一波数据了
    fileBufferLength -= length;
}

// 这里是文件读取线程：【文件（磁盘）】 ----音频数据----> 【文件缓冲区（程序内存）】
void AudioPlayThread::run() {
    // 1、初始化SDL的子系统，我们这里要播放音频就得初始化SDL的音频子系统
    int ret = SDL_Init(SDL_INIT_AUDIO);
    if (ret < 0) {
        qDebug() << "初始化SDL的子系统失败" << SDL_GetError();
        return;
    }

    // 2、打开设备
    // 第一个参数是配置SDL的参数，它是一个指针，我们当然可以直接定义一个指针传进去，但是为了减少手动内存管理，这里我们定义成一个栈区的对象、把这个对象的地址传进去
    SDL_AudioSpec spec;
    // （1）我们需要告诉【音频播放设备】它要播放的PCM数据的各个参数，这样它才知道该怎么播放PCM数据
    spec.freq = PCM_SAMPLE_RATE;
    spec.format = PCM_SAMPLE_FORMAT;
    spec.channels = PCM_CHANNELS;
    // （2）然后我们还需要设置一下音频播放库音频缓冲区的大小，这个音频缓冲区的大小是以样本为单位的（注意不是帧，录音的时候才是一帧一帧以帧为单位）
    // 这个音频缓冲区其实就是sdl_pull_data_callback这个回调函数里的stream指针所指向的一块内存
    spec.samples = AUDIO_BUFFER_LENGTH;
    // （3）然后我们还需要再设置一下【音频播放设备】主动向【我们的应用程序】拉取音频数据的回调，因为我们采取的是拉流的模式来播放音频
    // SDL播放音频有两种模式：
    // 推流（Push）：【我们的应用程序】主动推送数据给【音频播放设备】
    // 拉流（Pull）：【音频播放设备】主动向【我们的应用程序】拉取数据
    // 用的比较多的是第2种拉流的方式，因此这就要求我们的应用程序要准备好音频数据供音频设备来拉取
    spec.callback = sdl_pull_data_callback;
    // 第二个参数暂时不用管，传空即可，去打开设备
    ret = SDL_OpenAudio(&spec, nullptr);
    if (ret < 0) {
        qDebug() << "打开设备失败" << SDL_GetError();

        // 退出SDL的子系统
        SDL_Quit();

        return;
    }

    // 3、打开文件
    QFile file(PCM_FILE_PATH);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "打开文件失败，请确认文件路径：" << PCM_FILE_PATH;

        // 关闭设备
        SDL_CloseAudio();

        // 退出SDL的子系统
        SDL_Quit();

        return;
    }

    // 4、开始播放
    // 我们只要一调用SDL_PauseAudio(0)开始播放这个函数，SDL内部就会不断地触发它的“spec.callback”函数来向我们的应用程序拉取数据
    // 这里我们得先让它播起来，然后再开始从文件里读数据
    // 因为如果先从文件里读数据，等读到数据再开始播的话，理论上中间可能会有一小段卡顿
    // 所以我们宁可让前面空播一下，也不要中间的一小段卡顿
    SDL_PauseAudio(0);

    // 播放音频的核心逻辑其实就是：
    // （1）从文件里读取特定字节数的音频数据出来，存进文件缓冲区
    // 只要外界点击了“开始播放”，如果没有人点击“停止播放”，那我们这里就一直从文件里读取数据往文件缓冲区里存
    // 这里我们定义一个“临时的文件缓冲区”，然后上面还有一个指针类型“全局的文件缓冲区”，之所以要定义上面那个指针类型“全局的文件缓冲区”，是因为“全局的文件缓冲区”的下标需要偏移
    // 因为这个函数里有while循环，所以这个函数在没读完数据前是不会结束的，因此tempFileBufferData的栈区内存也不会被回收，所以我们可以放心地用上面那个全局的buffer指针指向这块栈内存来使用它里面的数据
    char tempFileBufferData[FILE_BUFFER_SIZE];
    while (isInterruptionRequested() == false) {
        // 从文件里读取FILE_BUFFER_LENGTH个字节的数据存放到tempFileBufferData里
        // 返回值是这次从文件里真正读取到了多少个字节的数据，因为可能读到最后只剩下2个字节的数据了，不够FILE_BUFFER_LENGTH个字节了
        fileBufferLength = file.read(tempFileBufferData, FILE_BUFFER_SIZE);
        qDebug() << "从文件里读取了" << fileBufferLength << "个字节的数据";

        if (fileBufferLength <= 0) { // 这一波读取出问题了或者读取完数据了，结束while循环，不再读取
            // 如果正在播放最后一波数据的时候，就释放了播放器资源，导致有部分数据没播放完全
            // 因为当先写线程和上面的线程是并行执行的，所以很有可能会出现这样一种场景：
            // 当我们读到最后一波数据的时候，上面读线程把fileBufferLength减的小于等于0了，并且音频播放设备也正在播放最后一波的数据——假设最后一波数据有100ms的时长
            // 音频播放设备才播放到80ms，结果这个线程执行到这里的判断了，判断到fileBufferLength <= 0了，于是就break掉while循环执行下面的关闭音频设备、退出SDL的子系统，
            // 这样音频就会被关闭掉，后面的20ms音频就无法正常播放出来了，所以我们这里更严谨的处理时延时一小段时间再break，至于延时多长时间呢？就延长最后一波数据的时长就可以了
            //
            // 计算一下最后一波数据的时长：
            // fileBufferLength是最后一波数据的字节数
            // BYTES_PER_SAMPLE是每个样本的字节数
            // 所以leftSampleCount就是最后一波数据的样本数
            int leftSampleCount = fileBufferLength / BYTES_PER_SAMPLE;
            // 而采样率是PCM_SAMPLE_RATE，采样率的倒数就是一个样本的时长——即1 / PCM_SAMPLE_RATE
            // 所以leftS就是最后一波数据的秒数，leftMs就是最后一波数据的毫秒数
            int leftS = leftSampleCount / PCM_SAMPLE_RATE;
            int leftMs = leftS / 1000;
            qDebug() << "最后一波音频数据时长：" << leftMs << "ms";
            // 睡眠leftMs，让音频设备正常播放完最后一波数据，再break掉while循环、再去下面关闭音频设备
            SDL_Delay(leftMs);
            break;
        } else { // 这一波数据读取成功了，让全局的文件缓冲区指向新读取的这波数据
            fileBufferData = tempFileBufferData;
        }

        // 这一波读取成功就不要再读了，当前线程立马进入等待，好让音频数据拉取线程去读取tempFileBufferData里的数据，再次提醒这个线程和上面的读线程是两个线程，两者是并行执行的
        // 那怎么等呢？
        // 很简单，搞个while死循环啥也不做在这一直等就行了，让上面的音频数据拉取线程去读取tempFileBufferData里的数据，上面的音频数据拉取线程读取完数据后我们会把fileBufferLength - len，
        // 也就是说等上面的音频数据拉取线程读取完数据后fileBufferLength将 <= 0，这里的死循环就打破了，就可以从文件里读取下一波数据了
        while (fileBufferLength > 0) {

        }
    }

    // 5、释放资源
    // 关闭文件
    file.close();

    // 关闭音频设备
    SDL_CloseAudio();

    // 退出SDL的子系统
    SDL_Quit();
}
