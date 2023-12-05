#ifndef FFMPEGUTIL_H
#define FFMPEGUTIL_H

extern "C" {
#include <libavformat/avformat.h>
}

class FFmpegUtil
{
public:
    FFmpegUtil();

    // 这里我们只演示直接对.pcm文件进行AAC编码，所以需要输入文件的参数
    // 但其实AAC编码的输入文件不一定非得是.pcm文件，比如你是.wav文件，那就不需要这三个参数了
    // 甚至我们也可以把其它类型的文件如.opus文件、.aac文件、.mp3文件等来做ACC编码，同样也不需要这三个参数
    //
    // 其实无论任何文件都可以拿来做ACC编码（其它编码也是同理），因为无论输入文件是什么格式，编码的本质无非就是拿出文件里的PCM数据来编码嘛，
    // 只不过.pcm和.wav文件里的PCM数据是最原始的，而.opus文件、.aac文件、.mp3文件里的PCM数据是已经经过压缩的PCM数据再来压缩而已、会越压越小
    static void aacEncode(
            // 输入参数
            const char *inFilePath, int64_t inChLayout, int inAr, AVSampleFormat inF,
            // 输出参数
            const char *outFilePath
            );
};

#endif // FFMPEGUTIL_H
