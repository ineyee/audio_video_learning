#include "transformthread.h"
#include "ffmpegutil.h"

#include <QDebug>

#define IN_FILE_PATH "/Users/yiyi/Desktop/24000_s16le_1.pcm"
#define OUT_FILE_PATH "/Users/yiyi/Desktop/out.pcm"

TransformThread::TransformThread(QObject *parent)
    : QThread{parent}
{

}

void TransformThread::run() {
    qDebug() << "重采样开始";

    // 输入文件的参数
    int64_t in_ch_layout = AV_CH_LAYOUT_MONO;
    int in_ar = 24000;
    AVSampleFormat in_f = AV_SAMPLE_FMT_S16;

    // 输出文件的参数
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    int out_ar = 48000;
    AVSampleFormat out_f = AV_SAMPLE_FMT_FLT;

    FFmpegUtil::audioResample(
                IN_FILE_PATH, in_ch_layout, in_ar, in_f,
                OUT_FILE_PATH, out_ch_layout, out_ar, out_f
                );

    qDebug() << "重采样结束";
}
