//
//  FLVVideoParser.c
//  FLVParser
//
//  Created by SihangHuang on 2019/9/26.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVVideoParser.h"
#include "FLVParserUtil.h"
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#define BASELINE_PROFILE 0x66
#define MAIN_PROFILE 0x77

const char * const Video_Frame_Type[] = {"!!视频帧类型出错!!", "keyframe", "inter frame", "disposable inter frame",
    "generated keyframe", "video info/command frame"};
const char * const Video_Codecs[] = {"!!视频编码器出错！！", "JPEG", "H263", "Sreen video", "On2 Vp6",
    "On2 Vp6 with alpha", "Screen video v2", "AVC"};

/*
 解析Advanced video coding的视频数据
 */
int parseAvcVideoPacket(FILE *file, char **logMessage, uint32_t dataSize);
/*
 解析NALU。数据可能有一个或者多个NALU
 */
int parseNALUs(FILE *file, char **logMessage, uint32_t dataSize);
/*
 解析AVC Decoder Configuration Record, 包含了SPS PPS信息
 
aligned(8) class AVCDecoderConfigurationRecord {
    unsigned int(8) configurationVersion = 1;
    unsigned int(8) AVCProfileIndication;
    unsigned int(8) profile_compatibility;
    unsigned int(8) AVCLevelIndication;
    bit(6) reserved = ‘111111’b;
    unsigned int(2) lengthSizeMinusOne;
    bit(3) reserved = ‘111’b;
    unsigned int(5) numOfSequenceParameterSets;
    for (i=0; i< numOfSequenceParameterSets; i++) {
        unsigned int(16) sequenceParameterSetLength ;
        bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
    }
    unsigned int(8) numOfPictureParameterSets;
    for (i=0; i< numOfPictureParameterSets; i++) {
        unsigned int(16) pictureParameterSetLength;
        bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
    }
}
  */
int parseAvcDecoderConfigRecord(FILE *file, char **logMessage);

    
void parseVideoData(FILE *file, uint32_t dataSize) {
    
    char *logMessage = createLogString();
    
    char param;
    readOrExit(&param, sizeof(char), 1, file, "读取视频参数失败");
    
    int frame = (int)((param & 0xf0) >> 4);
    fp_strcatMultiple(&logMessage, "视频帧类型: ", Video_Frame_Type[frame], " ", "");
    
    int codec = (int)(param & 0x0f);
    fp_strcatMultiple(&logMessage, "视频编码器: ", Video_Codecs[codec], " ", "");
    
//    // 跳过实际数据
//    fseek(file, dataSize-1, SEEK_CUR);
    
    int video_info_or_command_frame = 5;
    
    if (frame == video_info_or_command_frame) {
        uint8_t param;
        readOrExit(&param, sizeof(char), 1, file, "读取数据失败");
        if (param == 0) {
            fp_strcatMultiple(&logMessage, "内容：", "start of client-side seeking video frame sequence\n", "");
        } else {
            fp_strcatMultiple(&logMessage, "内容：", "end of client-side seeking video frame sequence\n", "");
        }
    } else {
        const int h263_video_packet = 2;
        const int screen_video_packet = 3;
        const int vp6_flv_video_packet = 4;
        const int vp6_flv_alpha_video_packet = 5;
        const int screen_v2_video_packet = 6;
        const int avc_video_packet = 7;

        switch (codec) {
            case h263_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示h263_video数据内容\n", "");
                fseek(file, dataSize-1, SEEK_CUR); // 跳过实际数据
                break;
            case screen_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示screen_video数据内容\n", "");
                fseek(file, dataSize-1, SEEK_CUR); // 跳过实际数据
                break;
            case vp6_flv_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示vp6_flv_video数据内容\n", "");
                fseek(file, dataSize-1, SEEK_CUR); // 跳过实际数据
                break;
            case vp6_flv_alpha_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示vp6_flv_alpha_video数据内容\n", "");
                fseek(file, dataSize-1, SEEK_CUR); // 跳过实际数据
                break;
            case screen_v2_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示screen_v2_video数据内容\n", "");
                fseek(file, dataSize-1, SEEK_CUR); // 跳过实际数据
                break;
            case avc_video_packet: {
                    long pos = ftell(file);
                
                    if (!parseAvcVideoPacket(file, &logMessage, dataSize - 1)) {
                        fseek(file, pos, SEEK_SET);
                        fseek(file, dataSize-1, SEEK_CUR); // 跳过实际数据
                    }
                }
                break;
            default:
                fp_strcat(&logMessage, "video packet类型暂不支持，出错");
                fseek(file, dataSize-1, SEEK_CUR); // 跳过实际数据
                break;
        }
    }
    
    puts(logMessage);
    free(logMessage);
}

int parseAvcVideoPacket(FILE *file, char **logMessage, uint32_t dataSize) {
    // AVCPacketType
    uint8_t type;
    if (!readOrReturn(&type, sizeof(char), 1, file, "读取AVCPacketType出错")) {
        return FAILURE;
    }
    if (type == 0) {
        fp_strcat(logMessage, "视频数据类型: header ");
    } else if (type == 1) {
        fp_strcat(logMessage, "视频数据类型: NALU ");
    } else if (type == 2) {
        fp_strcat(logMessage, "视频数据类型: end of sequence ");
    } else {
        fp_strcat(logMessage, "视频数据类型出错 ");
        return FAILURE;
    }
    dataSize -= 1;
    
    // Composition time
    int32_t compositionTime;
    if (!readOrReturn(&compositionTime, sizeof(char), 3, file, "读取Composition time出错")) {
        return FAILURE;
    }
    if (CPU_ENDIAN_SMALL) flip24(&compositionTime);
    
    char m[50];
    sprintf(m, "composition time: %d ", compositionTime);
    if (type != 1 && compositionTime != 0) {
        fp_strcat(logMessage, "当AVCPacketType为1时，composition time必须为0，出错");
        return FAILURE;
    } else {
        fp_strcat(logMessage, m);
    }
    dataSize -= 3;
    
    // data
    if (type == 0) {
        if (!parseAvcDecoderConfigRecord(file, logMessage)) {
            return FAILURE;
        }
    } else if (type == 1) {
        if (!parseNALUs(file, logMessage, dataSize)) {
            return FAILURE;
        }
    }
    
    return SUCCESS;
}

int parseAvcDecoderConfigRecord(FILE *file, char **logMessage) {
    // configurationVersion
    uint8_t version = 0;
    if (!readOrReturn(&version, sizeof(uint8_t), 1, file, "读取configurationVersion错误")) {
        return FAILURE;
    }
    if (version != 1) {
        fp_strcat(logMessage, "configurationVersion必须是1，出错 ");
        return FAILURE;
    }
    fp_strcat(logMessage, "configurationVersion: 1 ");
    
    //AVCProfileIndication
    uint8_t profile = 0;
    if (!readOrReturn(&profile, sizeof(uint8_t), 1, file, "读取AVCProfileIndication错误")) {
        return FAILURE;
    }
    char p[100];
    sprintf(p, "AVCProfileIndication: %d ", profile);
    fp_strcat(logMessage, p);
    
    //profile_compatibility
    uint8_t compat = 0;
    if (!readOrReturn(&compat, sizeof(uint8_t), 1, file, "读取profile_compatibility错误")) {
        return FAILURE;
    }
    char com[100];
    sprintf(com, "profile_compatibility: %d ", compat);
    fp_strcat(logMessage, com);
    
    // AVCLevelIndication
    uint8_t iden = 0;
    if (!readOrReturn(&iden, sizeof(uint8_t), 1, file, "读取AVCLevelIndication错误")) {
        return FAILURE;
    }
    char id[100];
    sprintf(id, "AVCLevelIndication: %d ", iden);
    fp_strcat(logMessage, id);
    
    // reserved + lenthSizeMinusOne
    // reserved
    uint8_t reserveAndLen = 0;
    if (!readOrReturn(&reserveAndLen, sizeof(uint8_t), 1, file, "读取reserved位和lenthSizeMinusOne错误")) {
        return FAILURE;
    }
    if ((reserveAndLen & 0xfc) != 0xfc) {
        fp_strcat(logMessage, "此处保留位必须是111111，出错");
        return FAILURE;
    }
    // lenthSizeMinusOne
    char lenM[100];
    sprintf(lenM, "lenthSizeMinusOne: %d ", reserveAndLen & 0x03);
    fp_strcat(logMessage, lenM);
    
    // reserved + numOfSequenceParameterSets
    // reserved
    uint8_t reserveAndSPS = 0;
    if (!readOrReturn(&reserveAndSPS, sizeof(uint8_t), 1, file, "读取reserved位和lenthSizeMinusOne错误")) {
        return FAILURE;
    }
    if ((reserveAndSPS & 0xe0) != 0xe0) {
        fp_strcat(logMessage, "此处保留位必须是111，出错");
        return FAILURE;
    }
    // numOfSequenceParameterSets
    uint8_t numSPS = reserveAndSPS & 0x1f;
    char spsM[100];
    sprintf(spsM, "num # SPS: %d ", numSPS);
    fp_strcat(logMessage, spsM);
    
    // 遍历SPS数据
    while (numSPS > 0) {
        uint16_t spsDataLen;
        if (!readOrReturn(&spsDataLen, sizeof(uint16_t), 1, file, "读取SPS数据长度错误！！！！！！")) {
            return FAILURE;
        }
        if (CPU_ENDIAN_SMALL) flip16(&spsDataLen);
        //跳过实际SPS数据
        fseek(file, spsDataLen, SEEK_CUR);
        numSPS--;
    }
    
    // numOfPictureParameterSets
    uint8_t numPPS = 0;
    if (!readOrReturn(&numPPS, sizeof(uint8_t), 1, file, "读取num # PPS错误！！！！！！")) {
        return FAILURE;
    }
    char ppsM[100];
    sprintf(ppsM, "numOfSequenceParameterSets: %d ", numPPS);
    fp_strcat(logMessage, ppsM);
    
    // 遍历PPS数据
    while (numPPS > 0) {
        uint16_t ppsDataLen = 0;
        if (!readOrReturn(&ppsDataLen, sizeof(uint16_t), 1, file, "读取PPS数据长度错误！！！！！！")) {
            return FAILURE;
        }
        if (CPU_ENDIAN_SMALL) flip16(&ppsDataLen);
        //跳过实际PPS数据
        fseek(file, ppsDataLen, SEEK_CUR);
        numPPS--;
    }
    
    return SUCCESS;
}

int parseNALUs(FILE *file, char **logMessage, uint32_t dataSize) {
    // FLV把NALU的起始码去掉了，没有起始码(start prefix code)
    
    // 头信息
    uint8_t header = 0;
    if (!readOrReturn(&header, sizeof(char), 1, file, "读取NALU的头信息失败！！！！！！")) {
        return FAILURE;
    }
    // forbidden zero bit
    if ((header & 0x80) != 0) {
        fp_strcat(logMessage, "forbidden zero bit = 1， NALU出现比特错误!!!! ");
    } else {
        fp_strcat(logMessage, "forbidden zero bit: 0 ");
    }
    // NRI重要性指示位
    char m[50];
    sprintf(m, "NRI：%d ", (header & 0x60) >> 5);
    fp_strcat(logMessage, m);
    
    // NALU类型
    char t[100];
    uint8_t type = header & 0x1f;
    if (type == 1) {
        sprintf(t, "NALU类型：%s ", "不分区非IDR图像的片");
    } else if (type == 2) {
        sprintf(t, "NALU类型：%s ", "片分区A");
    } else if (type == 3) {
        sprintf(t, "NALU类型：%s ", "片分区B");
    } else if (type == 4) {
        sprintf(t, "NALU类型：%s ", "片分区C");
    } else if (type == 5) {
        sprintf(t, "NALU类型：%s ", "IDR图像的片");
    } else if (type == 6) {
        sprintf(t, "NALU类型：%s ", "SEI");
    } else if (type == 7) {
        sprintf(t, "NALU类型：%s ", "SPS");
    } else if (type == 8) {
        sprintf(t, "NALU类型：%s ", "PPS");
    } else if (type == 9) {
        sprintf(t, "NALU类型：%s ", "AU分界符");
    } else if (type == 10) {
        sprintf(t, "NALU类型：%s ", "序列结束");
    } else if (type == 11) {
        sprintf(t, "NALU类型：%s ", "码流结束");
    } else if (type == 12) {
        sprintf(t, "NALU类型：%s ", "填充数据");
    } else if (type >= 13 && type <= 23) {
        sprintf(t, "NALU类型：%s ", "保留");
    } else if (type == 0 || (type >= 24 && type <= 31)) {
        sprintf(t, "NALU类型：%s ", "未使用");
    } else {
        fp_strcat(logMessage, "NALU类型错误");
        return FAILURE;
    }
    fp_strcat(logMessage, t);
    dataSize -= 1;
    
    // 跳过实际数据
    fseek(file, dataSize, SEEK_CUR);
    
    return SUCCESS;
}
