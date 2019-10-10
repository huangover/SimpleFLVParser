//
//  FLVAudioParser.c
//  FLVParser
//
//  Created by SihangHuang on 2019/10/10.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVAudioParser.h"
#include "FLVParserUtil.h"
#include <stdlib.h>

const char * const Audio_Codecs[] = {"Linear PCM", "ADPCM", "MP3", "Linear PCM, little endian",
    "Nellymoser 16KHz mono", "nellymoser 8khz mono", "nellymoser",
    "G711 A-law log PCM", "G711 mu-law log PCM", "reversed", "AAC",
    "Speedx", "MP3 8kHz", "Device-specific sound"};
const char * const Audio_Sampling_Rate[] = {"5.5 kHz", "11 kHz", "22 kHz", "44 kHz"};
const char * const Audio_Sampleing_Accuracy[] = {"8 bits", "16 bits"};
const char * const Audio_Channel_Type[] = {"mono", "stereo"};

int parseAACAudioData(FILE *file, char **logMessage, uint32_t dataSize);

void parseAudioData(FILE *file, uint32_t dataSize) {
    
    char *logMessage = createLogString();
    
    char param;
    readOrExit(&param, sizeof(char), 1, file, "读取音频参数失败");
    
    int codec = (int)((param & 0xf0) >> 4);
    fp_strcatMultiple(&logMessage, "编码器: ", Audio_Codecs[codec], "\n", "");
    
    int sample = (int)((param & 0x0c) >> 2);
    fp_strcatMultiple(&logMessage, "采样率: ", Audio_Sampling_Rate[sample], "\n", "");
    
    int accu = (int)((param & 0x02) >> 1);
    fp_strcatMultiple(&logMessage, "精度: ", Audio_Sampleing_Accuracy[accu], "\n", "");
    
    int channel = (int)(param & 0x01);
    fp_strcatMultiple(&logMessage, "声道: ", Audio_Channel_Type[channel], "\n", "");
    
    if (codec == 10) {
        long pos = ftell(file);
        parseAACAudioData(file, &logMessage, dataSize);
        fseek(file, pos, SEEK_SET); // 只读取aac audio data的前几个字段
    }
    
    fseek(file, dataSize - 1, SEEK_CUR);
    
    puts(logMessage);
    free(logMessage);
}

int parseAACAudioData(FILE *file, char **logMessage, uint32_t dataSize) {
    
    uint8_t aacPacketType = 0;
    
    if (!readOrReturn(&aacPacketType, sizeof(uint8_t), 1, file, "读取AAC Packet Type出错！！！！！")) return FAILURE;
    char type[100];
    sprintf(type, "AAC Packet Type: %d\n", aacPacketType);
    fp_strcat(logMessage, type);
    
    if (aacPacketType == 1) {
        fp_strcat(logMessage, "data: Raw AAC frame data\n");
    } else {
        fp_strcat(logMessage, "data: AAC sequence header(Audio Specific Config)\n");
        
        /*
         数据没有字节对齐，为了读取audio object type(5 bits), sampling frequency index(4 bits), sampling frequency(24 bits) 和 channelConfig(4 bits)，我们一次性读取5个字节共40个bits
         */
        
        char *info = calloc(5, sizeof(char));
        if (!readOrReturn(info, sizeof(char), 5, file, "读取Audio Specific Config相关信息失败！！！！！！")) return FAILURE;
        
        // Audio object type
        /*
         - 2: MPEG-4 AAC Low Complexity.
         - 5: MPEG-4 AAC Low Complexity with Spectral Band Replication(HE-AAC).
         - 29: MPEG-4 AAC Low Complexity with Spectral Band Replication and Parametric Stereo (HE-AAC v2). This
         configuration can be used only with stereo input audio data.
         - 23: MPEG-4 AAC Low-Delay.
         - 39: MPEG-4 AAC Enhanced Low-Delay. Since there is no
         ::AUDIO_OBJECT_TYPE for ELD in combination with SBR defined,
         enable SBR explicitely by ::AACENC_SBR_MODE parameter. The ELD
         v2 212 configuration can be configured by ::AACENC_CHANNELMODE
         parameter.
         - 129: MPEG-2 AAC Low Complexity.
         - 132: MPEG-2 AAC Low Complexity with Spectral Band
         Replication (HE-AAC).
         */
        uint8_t AOT = info[0] >> 3;
        char aotM[50] = {'\0'};
        sprintf(aotM, "Audio object type: %d\n", AOT);
        fp_strcat(logMessage, aotM);
        
        // sampling frequency index
        char sfIndex = (info[0] & 0x07) << 1; // 取info[0]的第三位，并左移一位
        sfIndex |= (info[1] & 0x80) >> 7;// 取info[1]的最高位，放到sfIndex的最低位;
        
        if (sfIndex == 0x0f) {
            uint32_t sampleFrequency = (info[1] & 0x7f) << 8;
            sampleFrequency = (sampleFrequency | info[2]) << 8;
            sampleFrequency = (sampleFrequency | info[3]) << 8;
            sampleFrequency = (sampleFrequency << 1) | ((info[4] & 0x80) >> 7);
            if (CPU_ENDIAN_SMALL) flip24(&sampleFrequency);
            
            char sfM[50];
            sprintf(sfM, "sampling frequency: %d\n", sampleFrequency);
            fp_strcat(logMessage, sfM);
            
            uint8_t channel = (info[4] & 0x78) >> 3;
            char cM[50] = {'\0'};
            sprintf(cM, "channel config: %d\n", channel);
            fp_strcat(logMessage, cM);
        } else {
            uint8_t channel = (info[1] & 0x78) >> 3;
            char cM[50] = {'\0'};
            sprintf(cM, "channel config: %d\n", channel);
            fp_strcat(logMessage, cM);
        }
    }
    
    return SUCCESS;
}
