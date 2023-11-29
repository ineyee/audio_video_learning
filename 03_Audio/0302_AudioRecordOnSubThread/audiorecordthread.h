#ifndef AUDIORECORDTHREAD_H
#define AUDIORECORDTHREAD_H

#include <QThread>

class AudioRecordThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioRecordThread(QObject *parent = nullptr);

private:
    void run();

signals:

};

#endif // AUDIORECORDTHREAD_H
