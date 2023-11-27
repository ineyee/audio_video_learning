#include "mybutton.h"
#include <QDebug>

MyButton::MyButton(QPushButton *parent)
    : QPushButton{parent}
{
    qDebug() << "MyButton::MyButton";
}

MyButton::~MyButton()
{
    qDebug() << "MyButton::~MyButton";
}
