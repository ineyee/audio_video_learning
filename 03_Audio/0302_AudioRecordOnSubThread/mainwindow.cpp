#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QThread>

// 导入我们自定义的音频录制线程
#include "audiorecordthread.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    qDebug() << "窗口销毁了";
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    // 录音注意点一：录音是个耗时操作，一定要在子线程里执行
    // 但是我们会发现如果我们直接在这个函数里进行音频录制，App就会卡住，鼠标一直在那转圈
    // 这是因为当前线程为主线程，而音频录制是一个耗时操作，所以如果把音频录制放在主线程里来做就会卡住UI线程——即主线程
    // 因此我们需要把音频录制这个耗时操作放到子线程里去做，这需要用到Qt的一个类QThread
    // 但实际开发中更多的我们不会直接去使用QThread，而是自定义一个Thread来继承自QThread使用
    // 同时我们会发现QThread的构造函数里有一个parent对象，我们完全可以把当前界面作为QThread的parent，parent的作用就是
    // 当parent销毁时也会销毁当前这个对象，所以但凡有parent的地方我们都可以这么做，不限于QWidget，这样我们就不用手动管理
    // 子线程对象的生命周期了
    qDebug() << "按钮这里是主线程：" << QThread::currentThread();

    // 这次我们做一个小优化，不让音频线程固定采集240次了，而是我们点击停止录音才停止采集，可以由我们手动来控制
    if (_isAudioRecording == false) { // 点击了“开始录音”
        // 1、这里我们只需要创建一个音频录制线程，并启动就可以了
        // 2、线程只要一启动就会自动调用它的run函数，run函数里的代码就是在子线程里执行的
        // 3、然后等run函数里的代码执行完，这个线程就算执行结束了
        // 4、但是需要注意的是线程执行结束和线程的生命周期结束是两回事，线程执行结束只是线程里的任务执行完了，这个线程的状态变成了
        // finished，这个线程就不能再用了，但是这个线程对象在堆区的内存还在，也就是说它的生命周期也远没有结束，因为它的生命周期
        // 是和当前MainWindow一样的
        _isAudioRecording = true;

        _audioRecordThread = new AudioRecordThread(this);
        _audioRecordThread->start();
        // 这里我们也监听一下，修复一下录音过程中出错了提前return导致按钮文字还是保持“停止录音”的bug
        // 当然我们也可以放在AudioRecordThread构造函数那里的监听改，但毕竟改文字是和UI相关的，而那个音频线程类
        // 就是跟录音相关的一系列东西，所以还是单独放在这里再监听一次吧
        connect(_audioRecordThread, &AudioRecordThread::finished,
                _audioRecordThread, [this] () -> void {
                    _isAudioRecording = false;

//                    _audioRecordThread->setStop(true);
                    _audioRecordThread->requestInterruption();

                    ui->pushButton->setText("开始录音");
                });

        // 注意所有跟UI相关的控件，我们都可以通过ui这个指针来获取到，直接指向控件的名字就可以了
        // 一旦开始录音，我们就改变文本
        ui->pushButton->setText("停止录音");
    } else { // 点击了“停止录音”
        // 录音注意点二：点击停止录音时，不要粗暴地销毁录音线程，避免丢掉最后一帧的部分数据或者无法正常释放资源，而是应该设置一个条件变量去打破录音线程的循环录制条件，让录音线程录完最后一帧正常结束
        // 那我们就停止录音
        // 那停止录音这件事需要做什么呢？其实我们最好不要直接销毁掉正在录音的那条音频线程，因为直接销毁掉的话，很有可能
        // 出现的问题就是某一帧还在录、还没录完呢，你突然就把线程搞死了，这就导致那一帧是不完整的，而且run函数也会终止
        // 地不明不白，很有可能无法执行到最后正常释放资源；所以实际开发中我们停止录音的正确做法其实不是销毁音频录制线程，
        // 而是设置一个条件变量，去打破音频录制线程里那个循环录制的条件，告诉音频录制线程录完这一帧后就不要在录了，可以结束了（这里可以感觉到点击“停止录音”其实
        // 不是立马停止录音吧？而是等一小小会，等录制线程录完正在录的最后一帧，并且正常释放完资源再真正停止录音），这样
        // 就可以保证就算我们点击停止录音时那一帧录到一半也能正常录完最后一个完整的帧，同时run函数也能正常结束，该释放的资源都释放掉
        _isAudioRecording = false;

        // 但其实让音频录制线程结束的那个条件变量“_stop”，Qt已经帮我们提供了一个，我们其实不需要自己定义，
        // 它就是requestInterruption，isInterruptionRequested()就相当于我们的_stop变量，requestInterruption()就相当于
        // 我们的setStop(true)函数，当调用过线程的requestInterruption()函数时，isInterruptionRequested返回值就为true，
        // 否则为false，两种实现办法的使用原理是一样的，所以我们可以直接使用系统提供的
//        _audioRecordThread->setStop(true);
        _audioRecordThread->requestInterruption();

        ui->pushButton->setText("开始录音");
    }
}

// 录音注意点三：如果我们在录音过程中做了类似关闭窗口这种强制杀死App的异常操作，这依然是相当于录音过程中粗暴地销毁了录音线程，也会带来丢掉最后一帧的部分数据或者无法正常释放资源的问题，
// 所以当我们监听到这些异常操作出现时，也要设置下条件变量，并且让录音线程wait一会再销毁内存
// 如果我们正在录音的过程中点击了关闭窗口按钮怎么办呢？这个操作其实就是在
// 销毁窗口的内存，并强制销毁掉正在录音的线程的内存，所以这个操作也会带来我们点击“停止录音”强制毁掉正在录音的那条音频线程
// 的问题，所以我们需要在窗口销毁的时候，也调用一下音频线程的setStop(true)函数，并且让音频录制线程等下死、等音频线程最后一帧正常
// 录完、资源也正常释放了再死，详见audiorecordthread.cpp里的析构函数
