#include "receiver.h"
#include <QDebug>

Receiver::Receiver(QObject *parent)
    : QObject{parent}
{

}

// 实现槽函数，编写处理信号的代码
void Receiver::handleExit() {
    qDebug() << "Receiver::handleExit()";
}

// 实现槽函数，编写处理信号的代码
int Receiver::handleExit1(int i1, int i2) {
    qDebug() << "Receiver::handleExit1(int i1, int i2)" << i1 << i2;
    return i1 + i2;
}
