extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class CaptureVideo{
private:
    char* m_inFileName=nullptr;
    char* m_outFileName=nullptr;
    AVFormatContext* m_inFmtCtx=nullptr;
    AVCodecContext* m_decoderCtx=nullptr;
    FILE* m_dest_fp=nullptr;
    AVPacket* m_pkt=nullptr;
public:
    CaptureVideo(char* argv1){
        m_outFileName=argv1;
    }
    ~CaptureVideo(){
        if(m_inFmtCtx){
            avformat_close_input(&m_inFmtCtx);
        }
        if(m_decoderCtx){
            avcodec_free_context(&m_decoderCtx);
        }
        if(m_dest_fp){
            fclose(m_dest_fp);
        }
    }
    //查询设备支持的录制视频格式
    void captureVideo_ListDevices(){
        const AVInputFormat* inFmt= av_find_input_format("video4linux2");
        if(inFmt==NULL){
            av_log(NULL,AV_LOG_ERROR,"find captureVideo_ListDevices failed!\n");
        }
        
        const char* deviceName="/dev/video0";


        AVDictionary *options=NULL;
        av_dict_set(&options,"list_devices","true",0);

        m_inFmtCtx=avformat_alloc_context();
        int ret=avformat_open_input(&m_inFmtCtx,deviceName,inFmt,&options);
        if(ret!=0){
            av_log(NULL,AV_LOG_ERROR,"open input format failed!\n");
            return;
        }

    }
    //查询设备支持的录制音频格式
    void captureAudio_ListDevices(){
        const AVInputFormat* inFmt= av_find_input_format("alsa");
        if(inFmt==NULL){
            av_log(NULL,AV_LOG_ERROR,"find captureVideo_ListDevices failed!\n");
        }
        
        const char* deviceName="hw:0,0";


        AVDictionary *options=NULL;

        m_inFmtCtx=avformat_alloc_context();
        int ret=avformat_open_input(&m_inFmtCtx,deviceName,inFmt,&options);
        if(ret!=0){
            av_log(NULL,AV_LOG_ERROR,"open input format failed!\n");
            return;
        }

    }
 
 
    //录制视频—以本机支持的设备格式保存 YVYU422
    void recVideo(){
        m_inFmtCtx=avformat_alloc_context();
        const AVInputFormat *inFmt=av_find_input_format("video4linux2");
        if(inFmt==NULL){
            av_log(NULL,AV_LOG_ERROR,"find captureVideo_ListDevices failed!\n");
        }

        const char* deviceName="/dev/video0";

        AVDictionary *options=NULL;
        //av_dict_set(&options,"framerate","30",0);
        av_dict_set(&options,"video_size","640x480",0);

        m_inFmtCtx=avformat_alloc_context();
        int ret=avformat_open_input(&m_inFmtCtx,deviceName,inFmt,&options);
        if(ret!=0){
            char errors[1024]={0};
            av_strerror(ret,errors,1024);
            printf("%d %s\n",ret,errors);
            //av_log(NULL,AV_LOG_ERROR,"open input format failed!\n");
            return;
        }

        ret=avformat_find_stream_info(m_inFmtCtx,NULL);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"avformat_find_stream_info failed!\n");
            return;
        }

        ret=av_find_best_stream(m_inFmtCtx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"av_find_best_stream failed!\n");
            return;
        }
        int videoIndex=ret;

        //创建解码器句柄
        m_decoderCtx=avcodec_alloc_context3(NULL);
        //拷贝解码器参数
        ret=avcodec_parameters_to_context(m_decoderCtx,m_inFmtCtx->streams[videoIndex]->codecpar);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"avcodec_parameters_to_context failed!\n");
            return;
        }

        //查找解码器
        const AVCodec* decoder=avcodec_find_decoder(m_decoderCtx->codec_id);
        if(decoder==NULL){
            av_log(NULL,AV_LOG_ERROR,"avcodec_find_decoder failed!\n");
            return;
        }

        //打开解码器
        ret=avcodec_open2(m_decoderCtx,decoder,NULL);
        if(ret!=0){
            av_log(NULL,AV_LOG_ERROR,"avcodec_open2 failed!\n");
            return;
        }

        //打开输出文件
        m_dest_fp=fopen(m_outFileName,"wb+");
        if(m_dest_fp==NULL){
            av_log(NULL,AV_LOG_ERROR,"open dest_fp failed!\n");
            return;
        }

        //
        m_pkt=av_packet_alloc();
        while(1){
            if(av_read_frame(m_inFmtCtx,m_pkt)==0){
                if(m_pkt->stream_index==videoIndex){
                    decodeVideo(m_pkt);
                }
            }
            av_packet_unref(m_pkt);
        }
    }
    //视频解码
    void decodeVideo(AVPacket* pkt){
        if(avcodec_send_packet(m_decoderCtx,pkt)==0){
            AVFrame* frame=av_frame_alloc();
            while(avcodec_receive_frame(m_decoderCtx,frame)>=0){
                //yuyv422==yuv422 packed连续存储，所以只有一个data
                fwrite(frame->data[0],1,m_decoderCtx->width*m_decoderCtx->height*2,m_dest_fp);
            }
            av_frame_free(&frame);
        }
    }

    //录制视频—以本机支持的设备格式 YVYU422转换成YUV420P格式
    void recVideo_YUV420p(){
        m_inFmtCtx=avformat_alloc_context();
        const AVInputFormat *inFmt=av_find_input_format("video4linux2");
        if(inFmt==NULL){
            av_log(NULL,AV_LOG_ERROR,"find captureVideo_ListDevices failed!\n");
        }

        const char* deviceName="/dev/video0";

        AVDictionary *options=NULL;
        //av_dict_set(&options,"framerate","30",0);
        av_dict_set(&options,"video_size","640x480",0);

        m_inFmtCtx=avformat_alloc_context();
        int ret=avformat_open_input(&m_inFmtCtx,deviceName,inFmt,&options);
        if(ret!=0){
            char errors[1024]={0};
            av_strerror(ret,errors,1024);
            printf("%d %s\n",ret,errors);
            //av_log(NULL,AV_LOG_ERROR,"open input format failed!\n");
            return;
        }

        ret=avformat_find_stream_info(m_inFmtCtx,NULL);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"avformat_find_stream_info failed!\n");
            return;
        }

        ret=av_find_best_stream(m_inFmtCtx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"av_find_best_stream failed!\n");
            return;
        }
        int videoIndex=ret;

        //创建解码器句柄
        m_decoderCtx=avcodec_alloc_context3(NULL);
        //拷贝解码器参数
        ret=avcodec_parameters_to_context(m_decoderCtx,m_inFmtCtx->streams[videoIndex]->codecpar);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"avcodec_parameters_to_context failed!\n");
            return;
        }

        //查找解码器
        const AVCodec* decoder=avcodec_find_decoder(m_decoderCtx->codec_id);
        if(decoder==NULL){
            av_log(NULL,AV_LOG_ERROR,"avcodec_find_decoder failed!\n");
            return;
        }

        //打开解码器
        ret=avcodec_open2(m_decoderCtx,decoder,NULL);
        if(ret!=0){
            av_log(NULL,AV_LOG_ERROR,"avcodec_open2 failed!\n");
            return;
        }

        //打开输出文件
        m_dest_fp=fopen(m_outFileName,"wb+");
        if(m_dest_fp==NULL){
            av_log(NULL,AV_LOG_ERROR,"open dest_fp failed!\n");
            return;
        }

        //从YUV422转换成YUV420p格式
        AVFrame* destFrame=av_frame_alloc();
        enum AVPixelFormat destPixFmt=AV_PIX_FMT_YUV420P;
        uint8_t* outBuffer=(static_cast<uint8_t*>(av_malloc(av_image_get_buffer_size(destPixFmt,m_decoderCtx->width,m_decoderCtx->height,1))));
        av_image_fill_arrays(destFrame->data,destFrame->linesize,outBuffer,destPixFmt,m_decoderCtx->width,m_decoderCtx->height,1);

        struct SwsContext* swsCtx=sws_getContext(m_decoderCtx->width,m_decoderCtx->height,m_decoderCtx->pix_fmt,m_decoderCtx->width,m_decoderCtx->height,destPixFmt,0,NULL,NULL,0);
        if(swsCtx==NULL){
            av_log(NULL,AV_LOG_ERROR,"sws_getContext failed!\n");
            return;
        }

        //
        m_pkt=av_packet_alloc();
        while(1){
            if(av_read_frame(m_inFmtCtx,m_pkt)==0){
                if(m_pkt->stream_index==videoIndex){
                    decodeVideo(m_pkt,swsCtx,destFrame);
                }
            }
            av_packet_unref(m_pkt);
        }
        decodeVideo(NULL,swsCtx,destFrame);
        if(outBuffer){
            av_freep(&outBuffer);
        }
    }
    //视频解码-转换成YUV420p
    void decodeVideo(AVPacket* pkt,struct SwsContext* swsCtx,AVFrame* destFrame){
        if(avcodec_send_packet(m_decoderCtx,pkt)==0){
            AVFrame* frame=av_frame_alloc();
            while(avcodec_receive_frame(m_decoderCtx,frame)>=0){
                //yuv420p
                sws_scale(swsCtx,frame->data,frame->linesize,0,m_decoderCtx->height,destFrame->data,destFrame->linesize);
                fwrite(destFrame->data[0],1,m_decoderCtx->width*m_decoderCtx->height,m_dest_fp);
                fwrite(destFrame->data[1],1,m_decoderCtx->width*m_decoderCtx->height/4,m_dest_fp);
                fwrite(destFrame->data[2],1,m_decoderCtx->width*m_decoderCtx->height/4,m_dest_fp);

                //yuyv422==yuv422 packed连续存储，所以只有一个data
                //fwrite(frame->data[0],1,m_decoderCtx->width*m_decoderCtx->height*2,m_dest_fp);
            }
            av_frame_free(&frame);
        }
    }

    //录制音频
    void recAudio(){
        m_inFmtCtx=avformat_alloc_context();
        const AVInputFormat *inFmt=av_find_input_format("alsa");
        if(inFmt==NULL){
            av_log(NULL,AV_LOG_ERROR,"find captureVideo_ListDevices failed!\n");
        }

        const char* deviceName="hw:0,0";

        AVDictionary *options=NULL;

        m_inFmtCtx=avformat_alloc_context();
        int ret=avformat_open_input(&m_inFmtCtx,deviceName,inFmt,&options);
        if(ret!=0){
            char errors[1024]={0};
            av_strerror(ret,errors,1024);
            printf("%d %s\n",ret,errors);
            //av_log(NULL,AV_LOG_ERROR,"open input format failed!\n");
            return;
        }

        ret=avformat_find_stream_info(m_inFmtCtx,NULL);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"avformat_find_stream_info failed!\n");
            return;
        }

        ret=av_find_best_stream(m_inFmtCtx,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"av_find_best_stream failed!\n");
            return;
        }
        int audioIndex=ret;

        //创建解码器句柄
        m_decoderCtx=avcodec_alloc_context3(NULL);
        //拷贝解码器参数
        ret=avcodec_parameters_to_context(m_decoderCtx,m_inFmtCtx->streams[audioIndex]->codecpar);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"avcodec_parameters_to_context failed!\n");
            return;
        }

        //查找解码器
        const AVCodec* decoder=avcodec_find_decoder(m_decoderCtx->codec_id);
        if(decoder==NULL){
            av_log(NULL,AV_LOG_ERROR,"avcodec_find_decoder failed!\n");
            return;
        }

        //打开解码器
        ret=avcodec_open2(m_decoderCtx,decoder,NULL);
        if(ret!=0){
            av_log(NULL,AV_LOG_ERROR,"avcodec_open2 failed!\n");
            return;
        }

        //打开输出文件
        m_dest_fp=fopen(m_outFileName,"wb+");
        if(m_dest_fp==NULL){
            av_log(NULL,AV_LOG_ERROR,"open dest_fp failed!\n");
            return;
        }

        //
        m_pkt=av_packet_alloc();
        while(1){
            if(av_read_frame(m_inFmtCtx,m_pkt)==0){
                if(m_pkt->stream_index==audioIndex){
                    decodeAudio(m_pkt);
                }
            }
            av_packet_unref(m_pkt);
        }
        decodeAudio(NULL);
    }

    //音频解码
    void decodeAudio(AVPacket* pkt){
        if(avcodec_send_packet(m_decoderCtx,pkt)==0){
            AVFrame* frame=av_frame_alloc();
            while(avcodec_receive_frame(m_decoderCtx,frame)>=0){
                //f32le packed
                fwrite(frame->data[0],1,frame->linesize[0],m_dest_fp);

            }
            av_frame_free(&frame);
        }
    }
};


int main(int argc,char** argv){
    // 设置日志级别
    av_log_set_level(AV_LOG_DEBUG);
    avdevice_register_all();
    CaptureVideo cv(argv[1]);
    //cv.captureVideo_ListDevices();
    //cv.recVideo();
    //cv.captureAudio_ListDevices();
    cv.recAudio();

    return 0;
}