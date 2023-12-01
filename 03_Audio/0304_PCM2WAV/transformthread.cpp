#include "transformthread.h"
#include "ffmpegutil.h"

#define PCM_PATH "/Users/yiyi/Desktop/in.pcm"
#define WAV_PATH "/Users/yiyi/Desktop/in.wav"

TransformThread::TransformThread(QObject *parent)
    : QThread{parent}
{

}

void TransformThread::run() {
    WAVHeader header;
    header.sampleRate = 48000;
    header.audioFormat = 3;
    header.bitsPerSample = 32;
    header.numChannels = 1;
    FFmpegUtil::pcm2wav(header, PCM_PATH, WAV_PATH);
}
