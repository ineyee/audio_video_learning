#ifndef AUDIORECORDTHREAD_H
#define AUDIORECORDTHREAD_H

#include <QThread>

class AudioRecordThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioRecordThread(QObject *parent = nullptr);
    ~AudioRecordThread();
//    void setStop(bool stop);

private:
    void run();
    // 条件变量，去打破录音线程的循环录制条件
//    bool _stop = false;
    // 因为时长可能比较长，所以我们定义成long long，以ms为单位
    unsigned long long audioDuration;

signals:
    // 录音时长一变，当前线程就往主线程发信号
    void audioDurationChangned(unsigned long long newAudioDuration);

};

#endif // AUDIORECORDTHREAD_H
