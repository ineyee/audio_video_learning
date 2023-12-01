#ifndef AUDIOPLAYTHREAD_H
#define AUDIOPLAYTHREAD_H

#include <QThread>

class AudioPlayThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioPlayThread(QObject *parent = nullptr);
    ~AudioPlayThread();

private:
    void run();

signals:
    // 播放时长一变，当前线程就往主线程发信号，ms为单位
    void playedAudioDurationChangned(unsigned long long newPlayedAudioDuration);

};

#endif // AUDIOPLAYTHREAD_H
