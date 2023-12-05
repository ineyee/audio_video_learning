#include "transformthread.h"
#include "ffmpegutil.h"

#include <QDebug>

#define IN_FILE_PATH "/Users/yiyi/Desktop/48000_s16le_2.pcm"
#define OUT_FILE_PATH "/Users/yiyi/Desktop/out.aac"

TransformThread::TransformThread(QObject *parent)
    : QThread{parent}
{

}

void TransformThread::run() {
    qDebug() << "编码开始";

    FFmpegUtil::aacEncode(
            // 输入参数
            IN_FILE_PATH, AV_CH_LAYOUT_STEREO, 48000, AV_SAMPLE_FMT_S16,
            // 输出参数
            OUT_FILE_PATH
            );

    qDebug() << "编码结束";
}
