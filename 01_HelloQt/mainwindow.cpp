#include "mainwindow.h"
#include <QDebug>
#include <QPushButton>
#include "mybutton.h"
#include "sender.h"
#include "receiver.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    qDebug() << "MainWindow::MainWindow";

    widgetBasicUse();
    widgetBasicUse1();

    signalAndSlot();
    signalAndSlot1();
    signalAndSlot2();
    signalAndSlot3();
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow::~MainWindow";
}

// 一、控件的基本使用
void MainWindow::widgetBasicUse() {
    // 设置窗口的标题
    setWindowTitle("主窗口");

    // 设置窗口的位置
    // 以父控件的左上角为坐标原点
    // 没有父控件，就以屏幕的左上角为坐标原点
    move(100, 100);

    // 设置窗口大小
    // 不包括标题栏区域
    // resize函数设置窗口大小后，窗口依然可以拖动改变大小
//    resize(600, 600);

    // 设置窗口大小
    // 不包括标题栏区域
    // setFixedSize函数设置窗口大小后，窗口不可以拖动改变大小
    setFixedSize(600, 600);


    // 创建按钮
    // 注意：new出来的Qt控件是不需要程序员手动delete的，Qt内部会自动管理内存：当父控件销毁时，会顺带销毁子控件
    QPushButton *button = new QPushButton();
    // 先设置按钮的父控件
    button->setParent(this);
    // 设置按钮的位置
    // 以父控件的左上角为坐标原点
    // 没有父控件，就以屏幕的左上角为坐标原点
    button->move(50, 50);
    // 设置按钮的大小
    button->resize(100, 50);
    // 设置按钮的文本
    button->setText("登录");
}

void MainWindow::widgetBasicUse1() {
    // 用我们自定义的button来验证下“Qt内部会自动管理内存：当父控件销毁时，会顺带销毁子控件”
    MyButton *button = new MyButton();
    button->setParent(this);
    button->move(200, 50);
    button->resize(100, 50);
    button->setText("注册");
}

// 二、信号与槽
void MainWindow::signalAndSlot() {
    // 上面我们已经知道怎么在窗口上添加按钮了，那怎么给按钮添加事件呢？这就要用到信号与槽了，信号与槽里有四大角色
    // 1、信号：比如点击按钮就会发出一个点击信号、双击按钮就会发出一个双击信号等，信号其实也是一个函数，只不过是一个只有声明没有实现的函数
    // 2、槽：一般也叫槽函数，是用来处理信号的函数
    // 信号与槽是多对多的关系，即一个信号可以对应多个槽，多个信号也可以对应一个槽
    // 3、信号的发送者：是一个对象、一个持有信号的对象，一般用来发出信号
    // 4、信号的接收者：是一个对象、一个持有槽的对象，一般用来接收并处理信号
    // 我们可以通过connect函数把信号的发送者、信号、信号的接收者、槽连接在一起：connect(信号的发送者, 信号, 信号的接收者, 槽);
    // 我们也可以通过disconnect函数来断开四大角色之间的连接：disconnect(信号的发送者, 信号, 信号的接收者, 槽);

    QPushButton *button = new QPushButton();
    button->setParent(this);
    button->move(50, 150);
    button->resize(100, 50);
    button->setText("关闭");
    // 这里：
    // button对象就是信号的发送者
    // 点击button对象时，button对象会发出一个信号是clicked，因为是传递函数这种大内存所以采用了指针传递
    // 主窗口对象就是信号的接收者，因为我们是想关闭主窗口嘛
    // 主窗口对象在收到button对象发出的clicked信号时想要关闭，于是槽就是它的close函数，因为是传递函数这种大内存所以采用了指针传递
    connect(button, &QPushButton::clicked, this, &MainWindow::close);
}

void MainWindow::signalAndSlot1() {
    // 上面的例子中我们使用的是系统提供的信号与槽，那我们该如何自定义信号与槽呢？记住关键就是四大角色
    // 1、创建信号的发送者，信号的发送者必须继承自QObject，只有继承自QObject才能自定义信号和槽
    // 2、创建信号的接收者和处理者，信号的接收者和处理者必须继承自QObject，只有继承自QObject才能自定义信号和槽
    // 其实Qt中的控件最终都是继承自QObject，比如上面的QPushButton、MainWindow
    // 3、自定义信号，详见sender.h里
    // 4、自定义槽，详见receiver.h和receiver.cpp里

    Sender *sender = new Sender();
    Receiver *receiver = new Receiver();
    connect(sender, &Sender::exit, receiver, &Receiver::handleExit);
    connect(sender, &Sender::exit1, receiver, &Receiver::handleExit1);

    // 让信号发送者发出信号
    emit sender->exit();
    int ret = emit sender->exit1(10, 20);
    qDebug() << ret;

    // 需要注意的是，我们上面说过“new出来的Qt控件是不需要程序员手动delete的，Qt内部会自动管理内存：当父控件销毁时，会顺带销毁子控件”，
    // 只说的是控件，即继承自QWidget的东西，其它东西我们还是得手动管理内存
    delete sender;
    delete receiver;
}

void MainWindow::signalAndSlot2() {
    // 我们除了可以用connect函数把信号与槽给连接起来，还可以把两个信号给连接起来：connect(信号的发送者1, 信号1, 信号的发送者2, 信号2);
    // 这样当信号1发出时就会触发信号2也发出
    // 至于会触发什么槽，还是看信号1和信号2各自连接了哪些槽
}

void MainWindow::signalAndSlot3() {
    // 我们除了可以用信号的接收者和槽来处理信号，还可以直接用Lambda表达式来处理信号
    QPushButton *button = new QPushButton();
    button->setParent(this);
    button->move(200, 150);
    button->resize(100, 50);
    button->setText("打印");
    connect(button, &QPushButton::clicked, [] () -> void {
       qDebug() << "Lambda表达式";
    });
}


