extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
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
    //列举本机的设备
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
    //录制视频
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
        decodeVideo(NULL);
    }

    void decodeVideo(AVPacket* pkt){
        if(avcodec_send_packet(m_decoderCtx,pkt)==0){
            AVFrame* frame=av_frame_alloc();
            while(avcodec_receive_frame(m_decoderCtx,frame)>=0){
                //yuyu422==yuv422 packed
                fwrite(frame->data[0],1,m_decoderCtx->width*m_decoderCtx->height*4,m_dest_fp);
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
    cv.recVideo();

    return 0;
}