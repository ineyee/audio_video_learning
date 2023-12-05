#include "ffmpegutil.h"

#include <Qfile>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

// 处理错误码
#define ERROR_BUF(ret) \
    char errbuf[1024]; \
    av_strerror(ret, errbuf, sizeof (errbuf));

FFmpegUtil::FFmpegUtil()
{

}

// 检查编码器codec是否支持采样格式sample_fmt
// FFmpeg官方提供的一个示例函数，我们直接复制过来用就行了，原来很简单，就是遍历codec->sample_fmts支持的采样格式，看看包不包含我们输入的采样格式
// retrun 1支持，return 0不支持
static int check_sample_fmt(const AVCodec *codec,
                            enum AVSampleFormat sample_fmt) {
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        qDebug() << codec->name << "支持的采样格式：" << av_get_sample_fmt_name(*p);
        if (*p == sample_fmt) return 1;
        p++;
    }
    return 0;
}

// 音频编码
//
// ctx: 编码上下文，用来进行编码
// frame: 文件输入缓冲区，里面存放着待编码的一波数据
// pkt: 文件输出缓冲区
// outFile: 输出文件
//
// 返回负数：中途出现了错误
// 返回0：编码操作正常完成
static int encode(AVCodecContext *ctx,
                  AVFrame *frame,
                  AVPacket *pkt,
                  QFile &outFile) {
    // 把输入文件缓冲区里所有的数据一次性发送到编码器
    int ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "avcodec_send_frame error" << errbuf;
        return ret;
    }

    // 能来到这里说明编码器已经收到了数据，开始编码了...

    // 那我们就这里搞个循环，不断从编码器中取出编码后的数据往packet里放
    // 因为很有可能我们往编码器里一次性送了1024个样本进去，但是编码后我们去编码器里拿可不一定就一次性能拿到全部的1024个样本，很有可能是
    // 先拿到100个样本编码完往输出缓冲区里放，与此同时编码器还在编码没编完的样本呢
    while (true) {
        // 从编码器中获取编码后的数据并写到输出缓冲区里
        ret = avcodec_receive_packet(ctx, pkt);
        // AVERROR(EAGAIN)：说明编码器里没有数据，所以我们需要结束掉这里的while循环、重新从输入缓冲区里拿数据发送给编码器（send frame）编码
        // AVERROR_EOF：说明编码器里没有数据——编好的数据被拿完了，所以我们需要结束掉这里的while循环、重新从输入缓冲区里拿数据发送给编码器（send frame）编码
        // 但是既然能走到这里，说明输入缓冲区已经把数据交给过编码器了，输入缓冲区里也没有数据了，因此我们直接return掉、结束掉本波编码、去文件里拿一波数据
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) { // 出现了错误
            ERROR_BUF(ret);
            qDebug() << "avcodec_receive_packet error" << errbuf;
            return ret;
        }

        // 能来到这里，说明成功从编码器里拿到了数据并且写到了输出缓冲区里，此时将输出缓冲区里的数据写入文件
        outFile.write((char *) pkt->data, pkt->size);

        // 释放pkt内部的资源
        av_packet_unref(pkt);
    }

    return 0;
}

void FFmpegUtil::aacEncode(
        // 输入参数
        const char *inFilePath, int64_t inChLayout, int inAr, AVSampleFormat inF,
        // 输出参数
        const char *outFilePath
        ) {
    // 1、变量定义
    // 文件
    QFile inFile(inFilePath);
    QFile outFile(outFilePath);

    // 函数调用的返回结果
    int ret = 0;

    // 编码器
    const AVCodec *codec = nullptr;

    // 编码上下文
    AVCodecContext *ctx = nullptr;

    // 编码的整体流程类似于重采样：
    // .pcm文件 -数据-> 输入缓冲区 -编码器编码-> 输出缓冲区 -数据-> .aac文件
    // 在重采样那个地方我们是自定义输入缓冲区和输出缓冲区的，而编码这里其实我们会用两个类来达到输入缓冲区和输出缓冲区的作用
    // 类似于输入缓冲区，存放从.pcm文件读取出来的、编码前的数据
    AVFrame *frame = nullptr;
    // 类似于输出缓冲区，存放编码后的、尚未写入到.acc文件里的数据
    AVPacket *pkt = nullptr;

    // 2、获取编码器
    // 获取到的默认是FFmpeg自带的aac编码器，名字就是“aac”，但是这个自带的编码器支持的采样格式比较少，而且规格也只能是LC的
    // 所以我们一般是用“libfdk_aac”，它支持的采样格式比较多，而且规格也可以是LC或者HE的
//    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    // 当然我们也可以直接通过名字来找AAC编码器，这样我们就可以FFmpeg自带的“aac”编码器以外更牛的编码器了，如“libfdk_aac”
    codec = avcodec_find_encoder_by_name("libfdk_aac");
    if (codec == nullptr) {
        qDebug() << "获取编码器失败";
        // 因为到这里还没有初始化成功任何东西，所以也没必要goto到后面释放资源，直接return掉即可
        return;
    }

    // 3、检查采样格式：检查当前编码器支持不支持输入文件各种参数，因为编码器对输入文件的各种参数是有要求的
    if (!check_sample_fmt(codec, inF)) {
        qDebug() << "不支持的采样格式" << av_get_sample_fmt_name(inF);
        // 因为到这里也还没有见到分配什么内存相关的东西，所以也没必要goto到后面释放资源，直接return掉即可
        return;
    }

    // 4、创建编码上下文
    // 把编码器传进去
    ctx = avcodec_alloc_context3(codec);
    if (ctx == nullptr) {
        qDebug() << "创建编码上下文失败";
        // 因为到这里还没有初始化成功任何东西，所以也没必要goto到后面释放资源，直接return掉即可
        return;
    }
    // 设置输入文件PCM的参数
    ctx->sample_rate = inAr;
    ctx->sample_fmt = inF;
    ctx->channel_layout = inChLayout;

    // 设置编码规格
    // AAC编码里我们用的比较多的编码规格是：FF_PROFILE_AAC_LOW和FF_PROFILE_AAC_HE、FF_PROFILE_AAC_HE_V2，后两者的压缩效率和压缩比会更高
    ctx->profile = FF_PROFILE_AAC_HE_V2;

    // 设置比特率
    // FF_PROFILE_AAC_LOW，适合96kbps~192kbps的比特率
    // FF_PROFILE_AAC_HE，适合48kbps~64kbps的比特率
    // FF_PROFILE_AAC_HE_V2，适合32kbps的比特率，可在低至32kbps的比特率下提供接近CD品质的声音
    // 当然这个值就算设定了，最终编码后的比特率也不一定是这个值，其实就是给编码器一个参考，尤其是我们开启了VBR模式之后；不设置的话，编码器会有自己的默认值
    ctx->bit_rate = 32000;

    // 5、打开编码器
    // 一些libfdk_aac特有的参数（比如vbr），可以通过options参数传递，这里我们就不开启vbr模式了
//    AVDictionary *options = nullptr;
//    av_dict_set(&options, "vbr", "1", 0);
//    ret = avcodec_open2(ctx, codec, &options);

    ret = avcodec_open2(ctx, codec, nullptr);
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "打开编码器失败";

        goto end;
    }

    // 6、创建AVFrame
    frame = av_frame_alloc();
    if (!frame) {
        qDebug() << "av_frame_alloc error";
        goto end;
    }

    // 输入缓冲区能存放多少个样本帧（注意这里是样本帧，不是样本，比如如果是双声道的话，1个样本帧 = 2个样本、左右各1个）：ctx->frame_size这个值是编码器建议的，不同的的编码器有不同的建议值，我们用它建议的值就行，样本帧数量（由frame_size决定）
    // frame->format和frame->channel_layout这两个参数，纯粹就是辅编码器给我们建议值ctx->frame_size的，以保证给出来输入缓冲区的大小能存放样本数的整数倍，而不至于出现建议存储1023.5样本的情况
    frame->nb_samples = ctx->frame_size;
    // 采样格式
    frame->format = ctx->sample_fmt;
    // 声道布局
    frame->channel_layout = ctx->channel_layout;
    // 创建AVFrame内部的输入缓冲区
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "av_frame_get_buffer error" << errbuf;
        goto end;
    }

    // 7、创建AVPacket
    pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "av_packet_alloc error";
        goto end;
    }

    // 8、打开文件
    if (!inFile.open(QFile::ReadOnly)) {
        qDebug() << "file open error" << inFilePath;
        goto end;
    }
    if (!outFile.open(QFile::WriteOnly)) {
        qDebug() << "file open error" << outFilePath;
        goto end;
    }

    // 9、编码
    // 从in.pcm文件里读取输入缓冲区大小inLinesize个字节的数据存储到输入缓冲区里，以便读取的数据刚好能填满输入缓冲区
    while ((ret = inFile.read((char *) frame->data[0],
                              frame->linesize[0])) > 0) {
        // 最后一次读取文件数据时，有可能剩余的数据不足输入缓冲区的最大大小——frame->linesize[0]——了，那就得少读取一点了，否则编码后的文件大小会有偏差
        if (ret < frame->linesize[0]) {
            // 声道数
            int chs = av_get_channel_layout_nb_channels(frame->channel_layout);
            // 每个样本的大小
            int bytes = av_get_bytes_per_sample((AVSampleFormat) frame->format);
            // 输入缓冲区改为真正读取到了多少个样本帧数量，防止编码器又读取了输入缓冲区最大大小——frame->linesize[0]——个样本数
            frame->nb_samples = ret / (chs * bytes);
        }

        // 编码
        if (encode(ctx, frame, pkt, outFile) < 0) {
            goto end;
        }
    }

    // 跟重采样一样，需要处理最后一波残留数据，flush编码器
    encode(ctx, nullptr, pkt, outFile);

end:
    // 10、释放资源
    // 关闭文件
    inFile.close();
    outFile.close();

    // 释放资源
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
}
