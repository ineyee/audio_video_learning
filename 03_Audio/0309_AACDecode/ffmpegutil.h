#ifndef FFMPEGUTIL_H
#define FFMPEGUTIL_H

extern "C" {
#include <libavformat/avformat.h>
}

class FFmpegUtil
{
public:
    FFmpegUtil();
    // 后面几个参数搞成指针，由外界传地址进来，因为我们是想在函数内部解码结束后告诉外界这几个值是多少
    static void aacDecode(
            // 输入参数，.aac文件、.m4a文件等
            const char *inFilePath,
            // 输出参数，.pcm文件等
            const char *outFilePath,
            // 输出的音频的相关参数，不然外界是不知道.pcm该怎么用
            int64_t *outChLayout, int *outAr, AVSampleFormat *outFs
            );
};

#endif // FFMPEGUTIL_H
