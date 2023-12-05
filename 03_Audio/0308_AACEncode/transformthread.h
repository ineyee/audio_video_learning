#ifndef TRANSFORMTHREAD_H
#define TRANSFORMTHREAD_H

#include <QThread>

class TransformThread : public QThread
{
    Q_OBJECT
public:
    explicit TransformThread(QObject *parent = nullptr);

private:
    void run();

signals:

};

#endif // TRANSFORMTHREAD_H
