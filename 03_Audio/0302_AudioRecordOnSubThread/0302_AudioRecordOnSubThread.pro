QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    audiorecordthread.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    audiorecordthread.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    mac/Info.plist

#======================跨平台开发配置=====================#
# macOS平台开发配置
mac: {
    QMAKE_INFO_PLIST = mac/Info.plist
    # 因为##/usr/local/Cellar/ffmpeg/6.0-with-options_1##有点重复，所以我们可以定义一个FFMPEG_HOME来存放它，然后用$${FFMPEG_HOME}来引用它就可以了
    FFMPEG_HOME = /usr/local/Cellar/ffmpeg/6.0-with-options_1
    INCLUDEPATH += $${FFMPEG_HOME}/include
    LIBS += -L$${FFMPEG_HOME}/lib \
            -lavdevice \
            -lavformat \
            -lavutil
}

# Windows平台开发配置
win32: {
    # 因为F:/xx/xx/ffmpeg/6.0-with-options_1有点重复，所以我们可以定义一个FFMPEG_HOME来存放它，然后用$${FFMPEG_HOME}来引用它就可以了
    FFMPEG_HOME = F:/xx/xx/ffmpeg/6.0
    INCLUDEPATH += $${FFMPEG_HOME}/include
    LIBS += -L$${FFMPEG_HOME}/lib \
            -lavdevice \
            -lavformat \
            -lavutil
}
#======================跨平台开发配置=====================#
