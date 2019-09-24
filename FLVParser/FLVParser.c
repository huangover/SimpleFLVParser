//
//  FLVParser.c
//  FLVParser
//
//  Created by SihangHuang on 2019/9/23.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVParser.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#define VIDEO_TAG 9
#define AUDIO_TAG 8
#define META_DATA_TAG 18

FILE *file = NULL;
long fileLen = 0;
const char * const Audio_Codecs[] = {"Linear PCM", "ADPCM", "MP3", "Linear PCM, little endian",
                        "Nellymoser 16KHz mono", "nellymoser 8khz mono", "nellymoser",
                        "G711 A-law log PCM", "G711 mu-law log PCM", "reversed", "AAC",
                        "Speedx", "MP3 8kHz", "Device-specific sound"};
const char * const Audio_Sampling_Rate[] = {"5.5 kHz", "11 kHz", "22 kHz", "44 kHz"};
const char * const Audio_Sampleing_Accuracy[] = {"8 bits", "16 bits"};
const char * const Audio_Channel_Type[] = {"mono", "stereo"};
const char * const Video_Frame_Type[] = {"!!视频帧类型出错!!", "keyframe", "inter frame", "disposable inter frame",
                            "generated keyframe", "video info/command frame"};
const char * const Video_Codecs[] = {"!!视频编码器出错！！", "JPEG", "H263", "Sreen video", "On2 Vp6",
                        "On2 Vp6 with alpha", "Screen video v2", "AVC"};

void parseHeader(void);
void parseBody(void);
void parsePreviousTagLength(FILE *file);
void parseGeneralTagType(FILE *file, uint8_t *tagType);
void parseDataSize(FILE *file, uint32_t *dataSize);
void parseTimeStamp(FILE *file);
void parseStreamID(FILE *file);
void parseAudioData(FILE *file, uint32_t dataSize);
void parseVideoData(FILE *file, uint32_t dataSize);
void parseMetaData(FILE *file, uint32_t dataSize);

void flip32(uint32_t *i);
void flip24(uint32_t *i);
void readOrExit(void * __restrict __ptr, size_t __size, size_t __nitems, FILE * __restrict __stream, char *errMsg);
void printAndExit(char *errMsg);
void printSeperator(void);

int initWithFile(const char* fileName) {
    if ((file = fopen(fileName, "r")) == NULL) {
        printf("Failed to open file %s\n", fileName);
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    printf("文件长度%lu\n", fileLen);
    fseek(file, 0, SEEK_SET);
    
    return 1;
}

void parse() {
    if (!file) printAndExit("Cannot open file with NULL");
    parseHeader();
    parseBody();
}

void parseHeader() {
    char *header = calloc(9, sizeof(char));
    readOrExit(header, sizeof(char), 9, file, "Failed to read FLV header");
    
    // signature
    if (header[0] == 'F' && header[1] == 'L' && header[2] == 'V') {
        puts("Signature是：FLV");
    } else {
        printAndExit("Signature不是FLV，出错");
    }
    // version
    if (header[3] == 1) {
        puts("Version是：1");
    } else {
        printAndExit("Version不是1，出错");
    }
    //Flgas
    if ((header[4] & 0xF8) != 0) {
        printAndExit("Flags前5位必须为0，出错");
    }
    if (header[4] & 0x01) {
        puts("FLV文件有视频TAG");
    }
    if (header[4] & 0x02) {
        printAndExit("Flags第7位必须为0，出错");
    }
    if (header[4] & 0x04) {
        puts("FLV文件有音频TAG");
    }
    free(header);
    printSeperator();
}

void parseBody() {
    
    if(fileLen == ftell(file)) printAndExit("文件body为空，出错");
    
    uint8_t tagType = 0;
    uint32_t dataSize = 0;
    
    while (1) {
        //=========================== 前一个tag的长度 ===========================
        parsePreviousTagLength(file);
        printSeperator();
        
        // 根据FLV文件的格式，文件到此为止
        if(fileLen == ftell(file)) break;
        
        //===========================tag type===========================
        parseGeneralTagType(file, &tagType);
        //===========================data size===========================
        parseDataSize(file, &dataSize);
        //===========================time stamp low===========================
        parseTimeStamp(file);
        //===========================time stamp high暂时忽略===========================
        fseek(file, 1, SEEK_CUR);
        //===========================stream ID===========================
        parseStreamID(file);
        //=========================== audio tag ===========================
        if (tagType == AUDIO_TAG) {
            parseAudioData(file, dataSize);
        }
        //=========================== video tag ===========================
        if (tagType == VIDEO_TAG) {
            parseVideoData(file, dataSize);
        }
        //=========================== metadata tag ===========================
        if (tagType == META_DATA_TAG) {
            parseMetaData(file, dataSize);
        }
        printSeperator();
    }
}

void parsePreviousTagLength(FILE *file) {
    uint32_t tagSize;
    readOrExit(&tagSize, sizeof(char), 4, file, "读取前一个Tag长度错误");
    flip32(&tagSize);
    printf("前一个tag长度是%d\n\n", tagSize);
}

void parseGeneralTagType(FILE *file, uint8_t *tagType) {
    readOrExit(tagType, sizeof(char), 1, file, "读取tagType失败");
    if (*tagType == AUDIO_TAG) {
        puts("Tag的类型是音频");
    } else if (*tagType == VIDEO_TAG) {
        puts("Tag的类型是视频");
    } else if (*tagType == META_DATA_TAG) {
        puts("Tag的类型是meta data");
    } else {
        puts("Tag类型读取错误，退出");
        exit(EXIT_FAILURE);
    }
}

void parseDataSize(FILE *file, uint32_t *dataSize) {
    *dataSize = 0;
    readOrExit(dataSize, sizeof(char), 3, file, "读取data size失败");
    flip24(dataSize);
    printf("data size是%u\n", *dataSize);
}

void parseTimeStamp(FILE *file) {
    uint32_t timeStampLow = 0;
    readOrExit(&timeStampLow, sizeof(char), 3, file, "读取time stamp失败");
    flip24(&timeStampLow);
    printf("time stampe是%u\n", timeStampLow);
}

void parseStreamID(FILE *file) {
    uint32_t streamID = 0;
    readOrExit(&streamID, sizeof(char), 3, file, "读取sream id失败");
    flip24(&streamID);
    printf("streamID是%u\n", streamID);
}

void parseAudioData(FILE *file, uint32_t dataSize) {
    char param;
    readOrExit(&param, sizeof(char), 1, file, "读取音频参数失败");
    int codec = (int)((param & 0xf0) >> 4);
    printf("编码器是: %s\n", Audio_Codecs[codec]);
    int sample = (int)((param & 0x0c) >> 2);
    printf("采样率: %s\n", Audio_Sampling_Rate[sample]);
    int accu = (int)((param & 0x02) >> 1);
    printf("精度: %s\n", Audio_Sampleing_Accuracy[accu]);
    int channel = (int)(param & 0x01);
    printf("声道: %s\n", Audio_Channel_Type[channel]);
    
    // 跳过实际数据
    fseek(file, dataSize-1, SEEK_CUR);
}

void parseVideoData(FILE *file, uint32_t dataSize) {
    char param;
    readOrExit(&param, sizeof(char), 1, file, "读取视频参数失败");
    int frame = (int)((param & 0xf0) >> 4);
    printf("视频帧类型: %s\n", Video_Frame_Type[frame]);
    int codec = (int)(param & 0x0f);
    printf("视频编码器: %s\n", Video_Codecs[codec]);
    
    // 跳过实际数据
    fseek(file, dataSize-1, SEEK_CUR);
    
    //                int video_info_or_command_frame = 5;
    //                if (frame == video_info_or_command_frame) {
    //                    fseek(file, 1, SEEK_CUR);
    //                } else {
    //                    const int h264_video_packet = 2;
    //                    const int screen_video_packet = 3;
    //                    const int vp6_flv_video_packet = 4;
    //                    const int vp6_flv_alpha_video_packet = 5;
    //                    const int screen_v2_video_packet = 6;
    //                    const int avc_video_packet = 7;
    //
    //                    switch (codec) {
    ////                        case h264_video_packet:
    ////                            break;
    ////                        case screen_video_packet:
    ////                            break;
    ////                        case vp6_flv_video_packet:
    ////                            break;
    ////                        case vp6_flv_alpha_video_packet:
    ////                            break;
    ////                        case screen_v2_video_packet:
    ////                            break;
    //                        case avc_video_packet:
    //                            fseek(file, *dataSize-1, SEEK_CUR);
    //                            break;
    //                        default:
    //                            puts("video packet类型暂不支持，出错");
    //                            exit(EXIT_FAILURE);
    //                            break;
    //                    }
    //                }
}

void parseMetaData(FILE *file, uint32_t dataSize) {
    puts("Tag的类型是meta data");
    // 跳过实际数据
    fseek(file, dataSize, SEEK_CUR);
}

#pragma mark - Utils

void flip32(uint32_t *i) {
    *i = (((uint32_t)(*i) & 0xff000000) >> 24) |  (((uint32_t)(*i) & 0x00ff0000) >> 8) | \
    (((uint32_t)(*i) & 0x0000ff00) << 8) | (((uint32_t)(*i) & 0x000000ff) << 24);
}

void flip24(uint32_t *i) {
    *i &= 0x00ffffff;
    *i = ((*i & 0x00ff0000) >> 16) | (*i & 0x0000ff00) | ((*i & 0x000000ff) << 16);
}

void readOrExit(void * __restrict __ptr, size_t __size, size_t __nitems, FILE * __restrict __stream, char *errMsg) {
    if (fread(__ptr, __size, __nitems, __stream) != __nitems) {
        puts(errMsg);
        exit(EXIT_FAILURE);
    }
}

void printAndExit(char *errMsg) {
    puts(errMsg);
    exit(EXIT_FAILURE);
}

void printSeperator(void) {
    puts("\n============================================\n");
}
