#ifndef SE_H
#define SE_H

#include <QObject>

class Sender : public QObject
{
    Q_OBJECT
public:
    explicit Sender(QObject *parent = nullptr);

// 1、自定义的信号需要写在signals:下面
// 前面我们说过信号其实也是一个函数，但是信号函数只需要声明，不需要实现
// 2、信号与槽都可以有参数和返回值：
// 信号的参数会传递给槽的参数，注意两者的参数个数（信号的参数要 >= 槽的参数，因为槽可以选择性的接收信号的参数）和类型
// 槽的返回值会传递给信号的返回值，注意两者的返回值类型
signals:
    void exit();
    int exit1(int i1, int i2);
};

#endif // SE_H
