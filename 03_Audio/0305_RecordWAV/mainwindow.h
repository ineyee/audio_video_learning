#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// 导入我们自定义的音频录制线程
#include "audiorecordthread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;

    bool _isAudioRecording = false;
    AudioRecordThread *_audioRecordThread = nullptr;

    void audioDurationChanged(unsigned long long newAudioDuration);
};
#endif // MAINWINDOW_H
