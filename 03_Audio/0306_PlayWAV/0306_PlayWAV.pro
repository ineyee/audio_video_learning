QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    audioplaythread.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    audioplaythread.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#======================跨平台开发配置=====================#
# macOS平台开发配置
mac: {
    # 因为##/usr/local/Cellar/sdl2/2.26.5##有点重复，所以我们可以定义一个SDL_HOME来存放它，然后用$${SDL_HOME}来引用它就可以了
    SDL_HOME = /usr/local/Cellar/sdl2/2.26.5
    INCLUDEPATH += $${SDL_HOME}/include
    LIBS += -L$${SDL_HOME}/lib \
            -lSDL2
}

# Windows平台开发配置
win32: {
    # 因为##F:/xx/xx/sdl2/2.26.5##有点重复，所以我们可以定义一个SDL_HOME来存放它，然后用$${SDL_HOME}来引用它就可以了
    SDL_HOME = /usr/local/Cellar/sdl2/2.26.5
    INCLUDEPATH += $${SDL_HOME}/include
    LIBS += -L$${SDL_HOME}/lib \
            -lSDL
}
#======================跨平台开发配置=====================#
