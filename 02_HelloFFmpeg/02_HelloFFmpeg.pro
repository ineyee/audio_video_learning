QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#======================单平台开发配置=====================#
# 配置1：设置FFmpeg头文件的位置，以便这个QT项目能找到它们来导入，固定格式：INCLUDEPATH += ##FFmpeg头文件的位置##
INCLUDEPATH += /usr/local/Cellar/ffmpeg/6.0-with-options_1/include

# 配置2：设置FFmpeg链接库的位置，以便这个QT项目能找到它们、进而找到具体的实现，固定格式：LIBS += -L ##链接库的位置## -l##我们具体要把链接哪些库##
# \：代表后面的东西其实是一行，最后一个东西就不需要加\了
LIBS += -L /usr/local/Cellar/ffmpeg/6.0-with-options_1/lib \
    -lavcodec \
    -lavdevice \
    -lavfilter \
    -lavformat \
    -lavutil \
    -lpostproc \
    -lswscale \
    -lswresample
# 这个地方需要注意macOS和Windows有区别，Windows麻烦多了，macOS很简单：
# macOS的lib目录下既有一份动态库也有一份动态库，所以这里只要指明链接库的位置就可以了，默认就会去链接动态库，除非我们指明链接静态库

# 而Windows的lib目录下则是静态库（.a）和静态导入库（.dll.a），动态库（.dll）则是放在bin目录下
# 我们知道静态库是直接链接进可执行文件的，所以静态库会使得可执行文件变大，也正是因为静态库是直接在可执行文件里的，所以我们可以直接使用
# 静态库里的函数。但是动态库就不一样了，动态库不会链接进可执行文件里，而是程序运行时才去找的，所以动态库不会使得可执行文件变大，但是程
# 序运行时是怎么找到它调用的某个函数在哪个动态库里呢？此时就要用到静态导入库了，静态导入库也是一种静态库，也会链接进可执行文件里，但它
# 跟普通静态库的区别就是它里面打包的不是函数的具体实现，而是一些信息，比如某个函数在哪个动态库里等等，它是专门为动态库服务的，这样我们
# 只需要把静态导入库链接进可执行文件里，就可以在项目里使用动态库了，编译器就能根据静态导入库顺利找到相应的动态库。
# 所以在Windwos上这里配置的其实是静态导入库的位置，这里配置完后我们还得把bin目录下所有的动态库都复制一份到我们项目的exe同级目录下，
# 以便可执行文件运行时能找到动态库，macOS上之所以不需要静态导入库，是因为编译器在编译时一旦发现要用到一些动态库会直接把类似于静态导入
# 库这样的信息直接编译进可执行文件里，本质来说两平台还是一样的，只不过macOS少生成了一些文件
#======================单平台开发配置=====================#
