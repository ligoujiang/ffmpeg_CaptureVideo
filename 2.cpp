extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
}


AVFormatContext* open_dev()
{
    // 获取采集格式
    const AVInputFormat *inputFmt = av_find_input_format("video4linux2"); // 视频

    int ret = 0;
    AVFormatContext *fmt_ctx = NULL;
    const char *deviceName = "/dev/video0"; // 视频设备
    AVDictionary *options = NULL;

    av_dict_set(&options, "video_size", "640x480", 0);  // 为打开视频设备设置参数
//    av_dict_set(&options, "pixel_format", "yuyv422",0);
    // 打开设备
    ret = avformat_open_input(&fmt_ctx, deviceName, inputFmt, &options);

    char errors[1024] = {0};
    if (ret < 0)
    {
        av_strerror(ret, errors, 1024);
        printf("Failed to open audio device, [%d]%s\n", ret, errors);
        return NULL;
    }
    return fmt_ctx;
}

int main()
{
    AVFormatContext *fmt_ctx = NULL;
    AVPacket *packet=av_packet_alloc();
    int ret = 0;
    // 设置日志级别
    av_log_set_level(AV_LOG_DEBUG);
    // 注册设备
    avdevice_register_all();

    // 创建文件
    const char *outPath = "/home/ligoujiang/文档/VScode/ffmpeg_CaptureVideo/vedio.yuv";   // 保存视频文件
    FILE *outFile = fopen(outPath, "wb+");
    if (!outFile)
    {
        goto __ERROR;
    }

    // 打开设备
    fmt_ctx = open_dev();
    if (!fmt_ctx)
    {
        goto __ERROR;
    }

    while ((ret = av_read_frame(fmt_ctx, packet) == 0))
    {
        av_log(NULL, AV_LOG_INFO, "Packet size: %d(%p)\n",
               packet->size, packet->data);

        // 写入文件
        fwrite(packet->data, 1, packet->size, outFile);
        fflush(outFile);

        // 释放packet空间
        av_packet_unref(packet);
    }

__ERROR:

    // 关闭文件
    if (outFile)
    {
        fclose(outFile);
    }

    // 关闭设备，释放上下文空间
    if (fmt_ctx)
    {
        avformat_close_input(&fmt_ctx);
    }

    av_log(NULL, AV_LOG_DEBUG, "Finish!\n");
}
