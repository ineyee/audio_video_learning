#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    // 音频播放操作同样是个耗时操作，所以我们也是放到子线程去做
    // 跟音频录制线程类似，我们也创建一个音频播放线程

    if (_isAudioPlaying == false) { // 点击了“开始播放”
        _isAudioPlaying = true;

        _audioPlayThread = new AudioPlayThread(this);
        _audioPlayThread->start();
        // 这里我们也监听一下，修复一下播放出错退出了或者正常播放完了按钮文字还是保持“停止播放”的bug
        // 当然我们也可以放在AudioRecordThread构造函数那里的监听改，但毕竟改文字是和UI相关的，而那个音频线程类
        // 就是跟播放相关的一系列东西，所以还是单独放在这里再监听一次吧
        connect(_audioPlayThread, &AudioPlayThread::finished,
                _audioPlayThread, [this] () -> void {
                    _isAudioPlaying = false;

                    _audioPlayThread->requestInterruption();

                    ui->pushButton->setText("开始播放");
                });

        ui->pushButton->setText("停止播放");
    } else { // 点击了“停止播放”
        _isAudioPlaying = false;

        _audioPlayThread->requestInterruption();

        ui->pushButton->setText("开始播放");
    }
}

