#include "ffmpegutil.h"

#include <Qfile>
#include <QDebug>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
}

// 处理错误码
#define ERROR_BUF(ret) \
    char errbuf[1024]; \
    av_strerror(ret, errbuf, sizeof (errbuf));

FFmpegUtil::FFmpegUtil()
{

}

void FFmpegUtil::audioResample(
        // 输入参数
        const char *inFilePath, int64_t inChLayout, int inAr, AVSampleFormat inF,
        // 输出参数
        const char *outFilePath, int64_t outChLayout, int outAr, AVSampleFormat outF
        ) {
    // 1、变量定义：音频重采样这里涉及到的资源比较多，所以为了简化释放资源的代码，函数中用到了goto语句，所以需要把所有用到的变量都定义到前面，否则前面goto过去的时候后面的变量还有可能没定义呢
    // 很多函数执行的返回结果
    int ret = 0;

    QFile inFile(inFilePath);
    int readLength = 0;
    QFile outFile(outFilePath);

    // 输出文件的参数
    int64_t out_ch_layout = outChLayout;
    int out_ar = outAr;
    AVSampleFormat out_f = outF;

    // 输入文件的参数
    int64_t in_ch_layout = inChLayout;
    int in_ar = inAr;
    AVSampleFormat in_f = inF;

    // 输入文件缓冲区
    // 指向输入文件缓冲区的指针，参数是三颗星，所以我们这里定义两个星，然后把地址传进去，创建完输入文件缓冲区会通过这个值让我们能访问到输入文件缓冲区
    uint8_t **inData = nullptr;
    // 输入文件缓冲区的大小，参数是一颗星，所以我们这里定义一个int，然后把地址传进去，创建完输入文件缓冲区会通过这个值告诉我们输入文件缓冲区的大小
    int inLinesize = 0;
    // 声道数
    int inChs = av_get_channel_layout_nb_channels(in_ch_layout);
    // 你希望输入文件缓冲区能容纳多少个样本
    int inSample = 1024;
    // 一个输入样本有多少个字节
    int inBytePerSample = av_get_bytes_per_sample(in_f) * inChs;

    // 输出文件缓冲区
    // 指向输出文件缓冲区的指针，参数是三颗星，所以我们这里定义两个星，然后把地址传进去，创建完输出文件缓冲区会通过这个值让我们能访问到输出文件缓冲区
    uint8_t **outData = nullptr;
    // 输出文件缓冲区的大小，参数是一颗星，所以我们这里定义一个int，然后把地址传进去，创建完输出文件缓冲区会通过这个值告诉我们输出文件缓冲区的大小
    int outLinesize = 0;
    // 声道数
    int outChs = av_get_channel_layout_nb_channels(out_ch_layout);
    // 你希望输出文件缓冲区能容纳多少个样本（AV_ROUND_UP向上取整）
    int outSample = av_rescale_rnd(out_ar, inSample, in_ar, AV_ROUND_UP);;
    // 一个输出样本有多少个字节
    int outBytePerSample = av_get_bytes_per_sample(out_f) * outChs;

    // 2、创建重采样上下文
    SwrContext *ctx = swr_alloc_set_opts(nullptr,
                                         // 输出参数
                                         out_ch_layout, out_f, out_ar,
                                         // 输入参数
                                         in_ch_layout, in_f, in_ar,
                                         0, nullptr);
    if (ctx == nullptr) {
        qDebug() << "创建重采样上下文失败";

        goto end;
    }

    // 3、初始化重采样上下文
    ret = swr_init(ctx);
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "初始化重采样上下文失败" << errbuf;

        goto end;
    }

    // 注意：文件操作不能直接把一个文件里的数据搬到另一个文件里，中间必然得有文件缓冲区，而音频重采样这里会涉及到两个缓冲区，输入文件缓冲区和输出缓冲区
    // 为什么这里要搞两个缓冲区呢？主要是因为重采样前后所需的文件缓冲区大小可能是不一样的，比如重采样前我们从文件里读取了1024
    // 个样本出来放进了文件缓冲区准备重采样，此时我们假设文件缓冲区要1000个字节，但是重采样后因为音频参数变化了，很可能需要1200个字节才能存储的下重采样后的数据，
    // 所以没办法用一个文件缓冲区来搞
    // 重采样的流程大概是：in.pcm -数据-> 输入缓冲区 -重采样API-> 输出缓冲区 -数据-> out.pcm
    // 重采样的过程是读一段数据放到输入缓冲区就重采样一段，而不是全部读取到输入缓冲区才开始重采样
    // 至于输入输出缓冲区为什么要搞成二级指针**这样的，其实是为了兼容planner格式的PCM数据，这个有需要时再去回看一下视频（第12节）吧，此处就按代码那么写就行了

    // 4、创建输入缓冲区和输出缓冲区
    // inChs, inSample, in_f,这三个参数其实就是用来让av_samples_alloc_array_and_samples知道大约应该分配多大的输入缓冲区的
    ret = av_samples_alloc_array_and_samples(&inData,
                                       &inLinesize,
                                       inChs,
                                       inSample,
                                       in_f,
                                       1 // 缓冲区不用搞内存对齐
                                       );
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "创建输入缓冲区失败" << errbuf;

        goto end;
    }
    ret = av_samples_alloc_array_and_samples(&outData,
                                       &outLinesize,
                                       outChs,
                                       outSample,
                                       out_f,
                                       1 // 缓冲区不用搞内存对齐
                                       );
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "创建输出缓冲区失败" << errbuf;

        goto end;
    }

    // 5、in.pcm -数据-> 输入缓冲区 -重采样API-> 输出缓冲区 -数据-> out.pcm
    // 打开输入文件
    if (!inFile.open(QFile::ReadOnly)) {
        qDebug() << "打开输入文件失败" << inFilePath;
        goto end;
    }
    // 打开输出文件
    if (!outFile.open(QFile::WriteOnly)) {
        qDebug() << "打开输出文件失败" << outFilePath;
        goto end;
    }

    // 从in.pcm文件里读取输入缓冲区大小inLinesize个字节的数据存储到输入缓冲区里
    while ((readLength = inFile.read((char *)inData[0], inLinesize)) > 0) {
        // 调用重采样的API对输入缓冲区里的数据进行重采样，并且函数内部会自动把重采样之后的数据存储到输出缓冲区里
        // 可见重采样的过程是读一段数据放到输入缓冲区就重采样一段，而不是全部读取到输入缓冲区才开始重采样
        //
        // 第一个参数：重采样上下文
        // 第二个参数：输出缓冲区
        // 第三个参数：输出缓冲区能放得下多少个样本
        // 第四个参数：输入缓冲区
        // 第五个参数：输入缓冲区内当前有多少个样本，注意不是能放得下多少个样本（比如最后一次读取的时候数据就不一定能填满输入缓冲区了）
        //
        // 返回值：重采样后的输出缓冲区里有多少个样本
        ret = swr_convert(ctx,
                    outData, outSample,
                    (const uint8_t **)inData, readLength / inBytePerSample
                    );

        if (ret < 0) {
            ERROR_BUF(ret);
            qDebug() << "swr_convert失败" << errbuf;

            goto end;
        }

        // 输出缓冲区 -数据-> out.pcm
        //
        // 第二个参数的单位是字节
        outFile.write((char *)outData[0], ret * outBytePerSample);
    }

    // 6、输出缓冲区数据残留问题的处理，直接这么搞一下就行了
    while ((ret = swr_convert(ctx,
                              outData, outSample,
                              nullptr, 0)) > 0) {
        outFile.write((char *)outData[0], ret * outBytePerSample);
    }

    // 7、释放资源
end:
    // 关闭输出文件
    outFile.close();
    // 关闭输入文件
    inFile.close();

    // 释放输出缓冲区
    if (outData) {
        av_freep(&outData[0]);
    }
    av_freep(&outData);
    // 释放输入缓冲区
    if (inData) {
        av_freep(&inData[0]);
    }
    av_freep(&inData);

    // 释放重采样上下文，之所以要传ctx的地址进去，是因为swr_free函数内部要把ctx置位为nullptr
    swr_free(&ctx);
}
