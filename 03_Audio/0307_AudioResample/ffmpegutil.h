#ifndef FFMPEGUTIL_H
#define FFMPEGUTIL_H

extern "C" {
#include <libswresample/swresample.h>
}

class FFmpegUtil
{
public:
    FFmpegUtil();

    static void audioResample(
            // 输入参数
            const char *inFilePath, int64_t inChLayout, int inAr, AVSampleFormat inF,
            // 输出参数
            const char *outFilePath, int64_t outChLayout, int outAr, AVSampleFormat outF
            );
};

#endif // FFMPEGUTIL_H
