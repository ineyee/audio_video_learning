#include "ffmpegutil.h"

#include <QFile>
#include <QDebug>

FFmpegUtil::FFmpegUtil()
{

}

// pcm2wav
//
// [header]: WAV文件的头部信息
// [pcmPath]: 原.pcm文件的路径
// [wavPath]: 要转换出的.wav文件的路径
//
// 注意点二：文件格式转换的核心逻辑其实：
// （1）打开输入文件
// （2）打开输出文件
// （3）先从输入文件的文件头里挨个读取出音频数据的参数，然后挨个写入输出文件的文件头里
// （4）然后从输入文件里读取一段数据、写入到文件缓冲区、然后再写入到输出文件里（我们无法直接把一个文件的数据读取到另一个文件里，中间都是要经过文件缓冲区的，至于文件缓冲区
// 定为多少个字节，可以看情况，比如我们的项目里定义为了1024个字节），就这样读一波、写一波、直到读完写完
// （5）关闭文件

// 注意点三：
// 我们都知道.wav文件无非就是给.pcm文件添加一个文件头，那怎么来添加这个文件头呢？
//
// 方法1：创建一个.wav文件，先把文件头写进去，然后再从.pcm文件里一段一段读取数据往.wav文件里追加
// 但是需要注意追加完之后，还需要通过文件的seek方法跳转到文件相应的下标处把文件头里的第2和第13部分改掉（但是我们现在这个demo是直接把已有的.pcm文件转成.wav文件，
// 所以不需要这么做，可以直接读取到.pcm文件的大小就能得到这两个值了，这个做法其实针对的是音频录制的时候就直接存进.wav文件里的场景），
// 因为这两部分的数据只有在把PCM数据写完之后我们才能确定它们的值，而文件头里其它部分的值我们在构建文件头的时候就能确定下来
//
// 方法2：创建一个.wav文件，先从.pcm文件里一段一段读取数据往.wav文件里写，等写完PCM数据后，我们不就能得到文件全部信息了嘛，文件头里的东西不就都能确定下来了吗嘛，
// 此时再在.wav文件的前面插入44个字节的文件头
//
// 方法2看起来更好，但其实效率更低，因为文件都是一个字节一个字节存储东西的，我们如果想往文件的头部插入44个字节的数据，那就意味着PCM数据的每一个字节都得往后移动44个字节，
// 这就跟数据结构与算法里数组的inser操作一样，因此我们此处选择方法1
void FFmpegUtil::pcm2wav(WAVHeader &header, const char *pcmPath, const char *wavPath) {
    header.byteRate = header.sampleRate * header.bitsPerSample * header.numChannels / 8;
    header.blockAlign = header.bitsPerSample * header.numChannels / 8;

    // （1）打开输入文件：.pcm文件
    QFile pcmFile(pcmPath);
    if (!pcmFile.open(QFile::ReadOnly)) {
        qDebug() << "打开.pcm文件失败，请检查文件路径：" << pcmPath;
        return;
    }

    // （2）打开输出文件：.wav文件
    QFile wavFile(wavPath);
    if (!wavFile.open(QFile::WriteOnly)) {
        qDebug() << "打开.wav文件失败，请检查文件路径：" << pcmPath;

        pcmFile.close();

        return;
    }

    // （3）先从输入文件的文件头里挨个读取出音频数据的参数，然后挨个写入输出文件的文件头里
    // 把header的地址传进去，再把告诉header有多大，这个write函数就能把整个header的内容都写进文件里去了
    header.pcmDataSize = pcmFile.size();
    header.riffDataSize = header.riffDataSize + sizeof(WAVHeader) - 8;
    wavFile.write((const char *)&header, sizeof(header));

    // （4）然后从输入文件里读取一段数据、写入到文件缓冲区、然后再写入到输出文件里，就这样读一波、写一波、直到读完写完
    char fileBuffer[1024];
    // 从.pcm文件里真正读取到的数据大小
    int readLength = 0;
    // 每次从.pcm文件里读取1024个字节到缓冲区里
    // readLength > 0代表的确从.pcm文件里读取到数据并且放进缓冲区里了
    while ((readLength = pcmFile.read(fileBuffer, sizeof(fileBuffer))) > 0) {
        // 那就把缓冲区里的数据写入到.wav文件里
        wavFile.write(fileBuffer, readLength);
    }

    // （5）关闭文件
    wavFile.close();
    pcmFile.close();
}
