#ifndef RE_H
#define RE_H

#include <QObject>

class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(QObject *parent = nullptr);

// 1、自定义的槽需要写在public slots:下面，注意这里public不能少
// 槽函数就既需要声明也需要实现了
// 2、信号与槽都可以有参数和返回值：
// 信号的参数会传递给槽的参数，注意两者的参数个数（信号的参数要 >= 槽的参数，因为槽可以选择性的接收信号的参数）和类型
// 槽的返回值会传递给信号的返回值，注意两者的返回值类型
public slots:
    void handleExit();
    int handleExit1(int i1, int i2);
};

#endif // RE_H
