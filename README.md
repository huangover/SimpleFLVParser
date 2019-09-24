# SimpleFLVParser

## 使用方式1：
把FLVParser.h和FLVParser.c文件拷贝下来，调用int initWithFile(const char* fileName)方法初始化，然后调用void parse(void)进行解析

## 使用方式2：
克隆整个Xcode项目并运行

## 终端输出信息
可以在终端看到FLV文件的信息如下:  

文件长度669036  
Signature是：FLV  
Version是：1  
FLV文件有视频TAG  
FLV文件有音频TAG  

============================================

前一个tag长度是0  


============================================

Tag的类型是meta data  
data size是273  
time stampe是0  
streamID是0  
Tag的类型是meta data  

============================================

前一个tag长度是284  


============================================

Tag的类型是音频  
data size是131  
time stampe是0  
streamID是0  
编码器是: MP3  
采样率: 22 khz  
精度: 16 bits  
声道: stereo  

============================================

前一个tag长度是142  


============================================

Tag的类型是视频  
data size是660  
time stampe是0  
streamID是0  
视频帧类型: keyframe  
视频编码器: on2 vp6  

......
