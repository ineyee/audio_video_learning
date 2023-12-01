#ifndef FFMPEGUTIL_H
#define FFMPEGUTIL_H

#include <_types/_uint16_t.h>
#include <_types/_uint32_t.h>

// .wav文件通常有44个字节的文件头，后面跟的就是PCM数据，.wav文件头里除了那几个固定的字符串是大端存储、其它的都是小端存储
//
// 1、[0, 3]4个字节：固定是字符串“RIFF”，代表.wav文件是基于RIFF标准的文件格式
// 2、[4, 7]4个字节：代表整个.wav文件除了（[0, 3]4个字节 + [4, 7]4个字节）以外的大小，即整个.wav文件的大小 - 4个字节 - 4个字节
//
// 3、[8, 11]4个字节：固定是字符串“WAVE”，代表文件的类型是“.wav”
//
// 4、[12, 15]4个字节：固定是字符串“fmt”，代表接下来的一段数据是要存储音频数据的采样率、采样大小、声道数等参数了，因为“fmt”只占3个字节，所以另外1个字节会被自动填充为空格
// 5、[16, 19]4个字节：代表“要描述音频数据的采样率、采样大小、声道数等参数”这段数据的大小，其实固定为16个字节 = 6 + 7 + 8 + 9 + 10 + 11几个部分加起来的大小
// 6、[20, 21]2个字节：AudioFormat，代表音频的格式，可以理解为文件最后面存储的是什么音频数据，1代表是“以int类型存储PCM数据、即采样格式类似于s16le之类的”，3代表是“以float类型存储PCM数据、即采样格式类似于f32le之类的”
// 这个一定要搞对，否则转出来的.wav文件播放起来会有噪音
// 7、[22, 23]2个字节：NumChannels，代表音频的声道数，1是单声道，2是双声道
// 8、[24, 27]4个字节：SampleRate，代表音频的采样率
// 9、[28, 31]4个字节：ByteRate，代表音频的字节率 = 比特率 / 8 = 采样率 * 采样大小 * 声道数 / 8 = SampleRate * BitsPerSample * NumChannels / 8
// 10、[32, 33]2个字节：BlockAlign，固定的计算公式 = 一个样本的位数 * 通道数 / 8 = BitsPerSample * NumChannels / 8
// 11、[34, 35]2个字节：BitsPerSample，代表音频的采样大小
//
// 12、[36, 39]4个字节：固定为“data”，代表接下来的一段数据是要存储PCM数据了
// 13、[40, 43]4个字节：代表后面跟着的PCM数据一共有多少个字节
// 14、后面的若干个字节：PCM数据
//
// 现在的CPU基本上默认都是小端模式了，所以我们搞个类，只要把值赋值给这个类的属性（例如把16这个值赋值给paramDataSize这个属性），存进文件的时候默认就会以小端的方式存储
// 我们根本就不用管什么大端小端问题，而如果我们搞一个44字节的数组，自己一个字节一个字节往里写数据的话，还得考虑大端小端问题
class WAVHeader {
public:
    // const char riffID[4] = "RIFF";和const char *riffID = "RIFF";都是错误的，因为它们后面都默认跟着个"\0"，都是5个字节，所以只能定义成char数组
    const char riffID[4] = {'R', 'I', 'F', 'F'};
    // ✅1、写完PCM数据后需要回来更新一把 = 整个.wav文件的大小 - 4个字节 - 4个字节
    // 但是我们现在这个demo是直接把已有的.pcm文件转成.wav文件，所以不需要这么做，可以直接读取到.pcm文件的大小就能得到这个值了，这个做法其实针对的是音频录制的时候就直接存进.wav文件里的场景
    // 4个字节的大小，我们不要用int，因为int在不同的平台下可能占不同的字节，而uint32_t这类不管什么平台下都是32位——即4个字节
    uint32_t riffDataSize;

    // format
    const char format[4] = {'W', 'A', 'V', 'E'};

    // fmt
    const char fmtID[4] = {'f', 'm', 't', ' '};
    // 音频参数的总大小
    uint32_t paramDataSize = 16;
    uint16_t audioFormat; // ✅手动设置1：构建的时候传进来
    uint16_t numChannels; // ✅手动设置2：构建的时候传进来
    uint32_t sampleRate; // ✅手动设置3：构建的时候传进来
    uint32_t byteRate; // ✅手动设置4：构建的时候传进来
    uint16_t blockAlign; // 可以由上面三个计算得来
    uint16_t bitsPerSample; // 可以由上面三个计算得来

    // data
    const char pcmID[4] = {'d', 'a', 't', 'a'};
    // ✅2、写完PCM数据后需要回来更新一把 = pcm数据的大小
    // 但是我们现在这个demo是直接把已有的.pcm文件转成.wav文件，所以不需要这么做，可以直接读取到.pcm文件的大小就能得到这个值了，这个做法其实针对的是音频录制的时候就直接存进.wav文件里的场景
    uint32_t pcmDataSize;
};

class FFmpegUtil
{
public:
    FFmpegUtil();

    static void pcm2wav(WAVHeader &header, const char *pcmPath, const char *wavPath);
};

#endif // FFMPEGUTIL_H
