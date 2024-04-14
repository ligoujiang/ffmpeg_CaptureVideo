1.cpp 有bug，录制的视频会抽搐并且伴随有绿屏。  （问题已解决）
2.cpp 暂时没发现问题。

1.cpp是使用本机支持的YVYU422格式采集视频  
3.cpp通过转换成YUV420p格式保存视频  
4.cpp是录制音频  

编译  
g++ 1.cpp -I/usr/local/ffmpeg/include -L/usr/local/ffmpeg/lib -lavformat -lavutil -lavcodec -lswscale  -lavdevice -o 1.out  
使用方法  
./1.out 1.yuv  
(其他文件都是类似的使用方法)
