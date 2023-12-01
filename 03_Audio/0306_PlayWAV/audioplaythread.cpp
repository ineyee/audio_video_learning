// 这个线程就是音频播放线程，专门用来完成音频播放这个耗时操作

#include "audioplaythread.h"

#include <SDL2/SDL.h>
#include <QDebug>
#include <QFile>
#include <QTime>
#include <iostream>

// sdl_pull_data_callback函数的第一个参数就是用来在读写线程之间传递数据用的
typedef void (PlayedDurationChanged)(unsigned long long totalLen);
class UserData {
public:
    std::function<PlayedDurationChanged> callback;
};
// 总播放的数据量因为时长可能比较长，所以我们定义成long long
unsigned long long totalLen = 0;

// 假设我们要播放我们桌面上的一个in.wav文件，这个文件就是我们上一步录下来的.wav文件，这个文件是用我的MacMini自带的0号下标设备录的
#define WAV_FILE_PATH "/Users/yiyi/Desktop/in.wav"

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
Uint8 *fileBufferData;
// 某一次从文件里真正读取到了多少个字节的数据，即某一次文件缓冲区里真正有多少个字节的数据
// length不同于size，size是固定不变的，length可能小于size
Uint32 fileBufferLength = 0;

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

    // 累加播放的数据量，字节为单位
    totalLen += length;
    if (((UserData *)userdata)->callback != nullptr) {
        ((UserData *)userdata)->callback(totalLen);
    }
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
    // 设置各种参数的时候不用我们写死了，而是可以从.wav文件的文件头里读取出来设置，SDL已经提供了相关的API
    // 第一个参数是配置SDL的参数，它是一个指针，我们当然可以直接定义一个指针传进去，但是为了减少手动内存管理，这里我们定义成一个栈区的对象、把这个对象的地址传进去
    SDL_AudioSpec spec;
    // 加载.wav文件
    // （1）.wav文件的路径
    // （2）把spec的地址传进去，这个方法内部会把.wav文件头的参数解析出来写到spec里面去
    // （3）把tempFileBufferData的地址传进去，这个方法内部会把.wav文件里所有的PCM数据一次性写到tempFileBufferData里面去，
    // 这里我们演示一下把所有的PCM数据一次性读取出来播放、全局文件缓冲区需要偏移下标的写法，当然我们也可以像之前那样读取一段、播放一段
    // （4）把allDataLength的地址传进去，这个方法内部会把读取到的PCM数据的大小写到allDataLength里面去
    // 这里不要直接传全局的文件缓冲区进去，tempFileBufferData其实是全部的数据，而全局的文件缓冲区其实只是指向这个tempFileBufferData，而且音频设备的读线程每读一个缓冲区大小的数据之后，全局的文件缓冲区这个指针还要往后偏移的
    // 这里tempFileBufferData会远远大于【音频设备】音频缓冲区的大小，所以绝对会用到全局的文件缓冲区的偏移来让读线程一段一段读tempFileBufferData里的数据
    Uint8 *tempFileBufferData = nullptr;
    // 某一次从文件里拿到的真实数据长度
    Uint32 allDataLength = 0;
    if (SDL_LoadWAV(WAV_FILE_PATH, &spec, &tempFileBufferData, &allDataLength) == nullptr) {
        qDebug() << "加载.wav文件失败" << SDL_GetError();

        // 退出SDL的子系统来释放资源不要忘了
        SDL_Quit();

        return;
    }
    // 然后我们还需要设置一下音频播放库音频缓冲区的大小，这个音频缓冲区的大小是以样本为单位的（注意不是帧，录音的时候才是一帧一帧以帧为单位）
    // 这个音频缓冲区其实就是sdl_pull_data_callback这个回调函数里的stream指针所指向的一块内存
    spec.samples = AUDIO_BUFFER_LENGTH;
    // 然后我们还需要再设置一下【音频播放设备】主动向【我们的应用程序】拉取音频数据的回调，因为我们采取的是拉流的模式来播放音频
    // SDL播放音频有两种模式：
    // 推流（Push）：【我们的应用程序】主动推送数据给【音频播放设备】
    // 拉流（Pull）：【音频播放设备】主动向【我们的应用程序】拉取数据
    // 用的比较多的是第2种拉流的方式，因此这就要求我们的应用程序要准备好音频数据供音频设备来拉取
    spec.callback = sdl_pull_data_callback;

    UserData userData;
    userData.callback = [this, spec] (unsigned long long totalLen) -> void {
        int bytePerSecond = spec.freq * SDL_AUDIO_BITSIZE(spec.format) * spec.channels / 8;
        // ms
        long long ms = 1000.0 * totalLen / bytePerSecond;

        // 发出信号
        emit playedAudioDurationChangned(ms);
    };
    spec.userdata = &userData;

    // 让全局文件缓冲区指向全部的PCM数组
    fileBufferData = tempFileBufferData;
    fileBufferLength = allDataLength;

    // 第二个参数暂时不用管，传空即可，去打开设备
    ret = SDL_OpenAudio(&spec, nullptr);
    if (ret < 0) {
        qDebug() << "打开设备失败" << SDL_GetError();

        // 退出SDL的子系统
        SDL_Quit();

        return;
    }

    // 3、打开文件
    QFile file(WAV_FILE_PATH);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "打开文件失败，请确认文件路径：" << WAV_FILE_PATH;

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

    // 从文件里一个样本一个样本的读取数据，这一步也不需要了，因为这里我们早就把所有的PCM数据读取到了，这里只需要保证一下最后一波数据的正常播放即可
    // 【音频设备】音频缓冲区的大小，单位为字节 = 音频缓冲区能容纳的样本数 * 每个样本的位深度 * 通道数 / 8
    int bufferSize = (AUDIO_BUFFER_LENGTH * (SDL_AUDIO_BITSIZE(spec.format) * spec.channels / 8));
    // 每个样本占用多少个字节 = 每个样本的位深度 * 通道数 / 8
    int bytePerSample = ((SDL_AUDIO_BITSIZE(spec.format) * spec.channels) / 8);
    while (isInterruptionRequested() == false) {
        // 这里用continue替换掉之前的while循环，不然那个等待会导致当前线程在我们点击了暂停播放时无法及时判断到isInterruptionRequested() == true
        if (fileBufferLength > 0) {
            continue;
        }

        if (fileBufferLength < bufferSize) { // 上面的读线程读取到最后一波数据了
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
            int leftSampleCount = fileBufferLength / bytePerSample;
            // 而采样率是PCM_SAMPLE_RATE，采样率的倒数就是一个样本的时长——即1 / PCM_SAMPLE_RATE
            // 所以leftS就是最后一波数据的秒数，leftMs就是最后一波数据的毫秒数
            int leftS = leftSampleCount / spec.freq;
            int leftMs = leftS / 1000;
            qDebug() << "最后一波音频数据时长：" << leftMs << "ms";
            // 睡眠leftMs，让音频设备正常播放完最后一波数据，再break掉while循环、再去下面关闭音频设备
            SDL_Delay(leftMs);
            break;
            break;
        } else if (fileBufferLength <= 0) { // 这一波读取出问题了或者读取完数据了，结束while循环结束读取
            // 这里就是上面原来的if判断
            break;
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
