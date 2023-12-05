#include "ffmpegutil.h"

#include <Qfile>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

// 输入缓冲区的大小，bit为单位
// 20480b / 8 = 2560字节
// 这个值也是官方建议的，太小的话输入缓冲区1里的数据还不够解析器塞牙缝的，当然你也可以搞个1280字节啥的
#define IN_DATA_SIZE 20480
// 需要再次读取输入文件数据的阈值，bit为单位，官方建议值
// 4096b / 8 = 512字节
// 我们不是等解析器从输入缓冲区1里读完数据才从.acc文件里读下一波往输入缓冲区1里放，而是输入缓冲区1里数据长度<4096的时候就开始从.acc文件里读下一波往输入缓冲区1里补充
// 这样做可能也是为了提高解析器的工作效率，避免后面剩很少的数据了还让解析器携带少量数据工作一波，只有真正是.aac里最后一点数据时才让解析器携带少量数据工作一波
#define REFILL_THRESH 4096

// 处理错误码
#define ERROR_BUF(ret) \
    char errbuf[1024]; \
    av_strerror(ret, errbuf, sizeof (errbuf));

FFmpegUtil::FFmpegUtil()
{

}

// 思路跟上一篇的编码是差不多的
static int decode(AVCodecContext *ctx,
                  AVPacket *pkt,
                  AVFrame *frame,
                  QFile &outFile) {
    // 把输入文件缓冲区里所有的数据一次性发送到解码器
    int ret = avcodec_send_packet(ctx, pkt);
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "avcodec_send_packet error" << errbuf;
        return ret;
    }

    // 能来到这里说明解码器已经收到了数据，开始解码了...

    // 那我们就这里搞个循环，不断从解码器中取出解码后的数据往frame里放
    // 因为很有可能我们往解码器里一次性送了1024个样本进去，但是解码后我们去解码器里拿可不一定就一次性能拿到全部的1024个样本，很有可能是
    // 先拿到100个样本解码完往输出缓冲区里放，与此同时解码器还在解码没解完的样本呢
    while (true) {
        // 从解码器中获取解码后的数据并写到输出缓冲区里
        ret = avcodec_receive_frame(ctx, frame);
        // AVERROR(EAGAIN)：说明编码器里没有数据，所以我们需要结束掉这里的while循环、重新从输入缓冲区里拿数据发送给编码器（send frame）编码
        // AVERROR_EOF：说明编码器里没有数据——编好的数据被拿完了，所以我们需要结束掉这里的while循环、重新从输入缓冲区里拿数据发送给编码器（send frame）编码
        // 但是既然能走到这里，说明输入缓冲区已经把数据交给过编码器了，输入缓冲区里也没有数据了，因此我们直接return掉、结束掉本波编码、去文件里拿一波数据
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) { // 出现了错误
            ERROR_BUF(ret);
            qDebug() << "avcodec_receive_frame error" << errbuf;
            return ret;
        }

        // 能来到这里，说明成功从解码器里拿到了数据并且写到了输出缓冲区里，此时将输出缓冲区里的数据写入文件
        outFile.write((char *) frame->data[0], frame->linesize[0]);
    }
}

void FFmpegUtil::aacDecode(
        // 输入参数，.aac文件、.m4a文件等
        const char *inFilePath,
        // 输出参数，.pcm文件等
        const char *outFilePath,
        // 输出的音频的相关参数，不然外界是不知道.pcm该怎么用
        int64_t *outChLayout, int *outAr, AVSampleFormat *outFs
        ) {
    // 1、变量定义
    // 函数调用的返回结果
    int ret = 0;

    // 每次从输入文件中读取到的数据的真实长度，不一定够输入缓冲区的长度——比如最后一次
    int inLen = 0;
    // 是否已经读取到了输入文件的尾部
    int inEnd = 0;

    // 用来存放从.aac文件里读取出来的数据，解码并不像编码那样直接把数据读取出来就往pkt里放，而是会先放到这个数组里，这个数组其实才是真正的输入缓冲区1——或者叫它解析器输入缓冲区
    // 然后这个输入缓冲区里的数据要经过解析器处理后才能进入到pkt
    // 为什么不像编码的时候那样直接搞进pkt里呢？
    // 因为编码的时候我们用的是PCM数据，对于PCM数据我们能明确地知道它的样本是多大的
    // 而AAC编码的数据，我们根本不知道它的一个样本有多大，只能先大概读取xx字节的数据到输入缓冲区1里，交给解析器去解析后才能知道样本有多大，此时再交给pkt才行
    // IN_DATA_SIZE就是这个输入缓冲区的大小，加上AV_INPUT_BUFFER_PADDING_SIZE是为了防止某些优化过的reader一次性读取过多导致越界（加上就行了，暂时不要深究）
    char inDataArray[IN_DATA_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    // 再让这个指针指向inDataArray，将来也要偏移的，类似于我们音频重采样哪里的做法
    char *inData = inDataArray;

    // 文件
    QFile inFile(inFilePath);
    QFile outFile(outFilePath);

    // 解码器
    const AVCodec *codec = nullptr;
    // 解码上下文
    AVCodecContext *ctx = nullptr;
    // 解析器上下文
    AVCodecParserContext *parserCtx = nullptr;

    // 解码的整体流程其实就是编码的流程给逆过来：
    // .aac文件 -数据-> 输入缓冲区1即inDataArray -解析器处理-> 输入缓冲区2即pkt -解码器解码-> 输出缓冲区frame -数据-> .pcm文件
    // 在重采样那个地方我们是自定义输入缓冲区和输出缓冲区的，而编码和解码这里其实我们会用两个类来达到输入缓冲区和输出缓冲区的作用
    // 类似于输入缓冲区，存放从.aac文件读取出来的、解码前的数据
    AVPacket *pkt = nullptr;
    // 类似于输出缓冲区，存放解码后的、尚未写入到.acc文件里的数据
    AVFrame *frame = nullptr;

    // 2、获取解码器
    codec = avcodec_find_decoder_by_name("libfdk_aac");
    if (!codec) {
        qDebug() << "decoder libfdk_aac not found";
        return;
    }

    // 3、初始化解析器上下文
    parserCtx = av_parser_init(codec->id);
    if (!parserCtx) {
        qDebug() << "av_parser_init error";
        return;
    }

    // 4、创建解码上下文
    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        qDebug() << "avcodec_alloc_context3 error";
        goto end;
    }

    // 5、创建AVPacket
    // 解码的时候我们不需要设置输入和输出缓冲区的大小，可见解码器有它们自己的解码节奏
    pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "av_packet_alloc error";
        goto end;
    }

    // 6、创建AVFrame
    frame = av_frame_alloc();
    if (!frame) {
        qDebug() << "av_frame_alloc error";
        goto end;
    }

    // 7、打开解码器
    ret = avcodec_open2(ctx, codec, nullptr);
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "avcodec_open2 error" << errbuf;
        goto end;
    }

    // 8、打开文件
    if (!inFile.open(QFile::ReadOnly)) {
        qDebug() << "file open error:" << inFilePath;
        goto end;
    }
    if (!outFile.open(QFile::WriteOnly)) {
        qDebug() << "file open error:" << outFilePath;
        goto end;
    }

    // 9、解码
    // 从输入文件里读取一波【输入缓冲区大小】长度的数据（注意后面的64个字节其实就是防止越界用的，不会真得用来存数据），放到输入缓冲区1即inDataArray里面
    inLen = inFile.read(inDataArray, IN_DATA_SIZE);
    while (inLen > 0) { // inLen > 0就代表读到数据了
        // 先经过解析器上下文处理，把输入缓冲区1里的数据搞到输入缓冲区2即pkt里
        // 所以对于解析器来说：inDataArray/inData是输入，pkt是输出
        // 解析过后的数据才能解码
        // 但是需要注意的是我们inFile.read是读取了IN_DATA_SIZE=20480大小的数据到inDataArray里，但是这里解析器可能一下子吞不了这么大的数据量，它有自己的节奏，很可能是分批解析的，
        // 也就是说有可能我们读取了一次.aac，但是解析了三次、解码了三次、往.pcm文件里写了三次，这也是为什么我们要搞inData和inLen的原因，目的就是为了解析器读取数据做偏移用
        ret = av_parser_parse2(parserCtx, ctx,
                               &pkt->data, &pkt->size,
                               (uint8_t *) inData, inLen,
                               AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            ERROR_BUF(ret);
            qDebug() << "av_parser_parse2 error" << errbuf;
            goto end;
        }

        // 这两步就是协助解析器分批从输入缓冲区1里拿数据时，每次都能知道从哪个位置开始读，直到解析完输入缓冲区1里的数据，代表这一波解码完毕
        // 但是这只是这一波数据解码完毕了，也就是说从.aac文件里读取的一波数据解码完毕了，但是.aac文件里也可能有好几波数据啊，这就是要靠下面的处理了
        // 跳过已经解析过的数据
        inData += ret;
        // 减去已经解析过的数据大小
        inLen -= ret;

        // 解码
        if (pkt->size > 0 && decode(ctx, pkt, frame, outFile) < 0) {
            goto end;
        }

        // 如果数据不够了，再次从.aac文件里读取数据往输入缓冲区1里补充弹药，直到.aac文件里数据读完了，inLen减为<=0了就退出while循环
        if (inLen < REFILL_THRESH && !inEnd) {
            // 先把上一波不足REFILL_THRESH的剩余数据移动到缓冲区最前面
            memmove(inDataArray, inData, inLen);
            // 此时因为剩余数据已经移动到最前面了，所以inData可以再次指向缓冲区的最前面
            inData = inDataArray;

            // 跨过上一波的剩余数据，从.aac文件里读取【IN_DATA_SIZE - inLen】数据补充到后面——即从inData + inLen——即上一波剩余数据的尾部的位置
            int len = inFile.read(inData + inLen, IN_DATA_SIZE - inLen);
            if (len > 0) { // 读取到数据了
                // 此时输入缓冲区1里数据的大小就应该加上新读到的数据大小
                inLen += len;
            } else { // 没有读取到数据，代表文件中已经没有任何数据了，inEnd = 1以便下次循环不要再去文件里读数据了
                inEnd = 1;
            }
        }
    }

    // flush解码器，处理数据残留
    decode(ctx, nullptr, frame, outFile);

    // 10、设置输出参数，等全部解码结束后，设置一次就可以了
    *outAr = ctx->sample_rate;
    *outFs = ctx->sample_fmt;
    *outChLayout = ctx->channel_layout;

    // 11、释放资源
end:
    inFile.close();
    outFile.close();
    av_frame_free(&frame);
    av_packet_free(&pkt);
    av_parser_close(parserCtx);
    avcodec_free_context(&ctx);
}
