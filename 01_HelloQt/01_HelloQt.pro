# 包含了core、gui两个模块，QT开发这两个模块是必备的
QT       += core gui

# 版本高于Qt4，就包含一下widgets模块
# 版本低于Qt4，widgets模块是内置在gui模块里面的
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# 支持C++17的语法
CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 要参与编译的源文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mybutton.cpp \
    receiver.cpp \
    sender.cpp

# 要参与编译的头文件
HEADERS += \
    mainwindow.h \
    mybutton.h \
    receiver.h \
    sender.h

# 要参与编译的界面文件，如果界面是通过可视化搭建的话
#FORMS += \
#    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
