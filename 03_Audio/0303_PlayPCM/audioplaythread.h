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

};

#endif // AUDIOPLAYTHREAD_H
