#ifndef AUDIORECORDTHREAD_H
#define AUDIORECORDTHREAD_H

#include <QThread>

class AudioRecordThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioRecordThread(QObject *parent = nullptr);
//    void setStop(bool stop);

private:
    void run();
    // 条件变量，去打破录音线程的循环录制条件
//    bool _stop = false;

signals:

};

#endif // AUDIORECORDTHREAD_H
