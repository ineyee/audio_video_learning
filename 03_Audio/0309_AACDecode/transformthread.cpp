#include "transformthread.h"
#include "ffmpegutil.h"

#include <QDebug>

#define IN_FILE_PATH "/Users/yiyi/Desktop/in.aac"
#define OUT_FILE_PATH "/Users/yiyi/Desktop/out.pcm"

TransformThread::TransformThread(QObject *parent)
    : QThread{parent}
{

}

void TransformThread::run() {
    qDebug() << "解码开始";

    int64_t outChLayout;
    int outAr;
    AVSampleFormat outFs;
    FFmpegUtil::aacDecode(
            // 输入参数
            IN_FILE_PATH,
            // 输出参数
            OUT_FILE_PATH, &outChLayout, &outAr, &outFs
            );
    qDebug() << "解码后PCM数据的采样率：" << outAr;
    qDebug() << "解码后PCM数据的采样格式：" << av_get_sample_fmt_name(outFs);
    qDebug() << "解码后PCM数据的声道数：" << av_get_channel_layout_nb_channels(outChLayout);

    qDebug() << "解码结束";
}
