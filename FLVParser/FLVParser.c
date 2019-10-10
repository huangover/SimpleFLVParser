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
#include <stdbool.h>
#include "FLVScriptDataParser.h"
#include "FLVParser.h"
#include "FLVParserUtil.h"
#include "FLVVideoParser.h"
#include "FLVAudioParser.h"

#define VIDEO_TAG 9
#define AUDIO_TAG 8
#define META_DATA_TAG 18


FILE *file = NULL;
long fileLen = 0;

void parseHeader(void);
void parseBody(void);
void parsePreviousTagLength(FILE *file);
void parseGeneralTagType(FILE *file, uint8_t *tagType);
void parseDataSize(FILE *file, uint32_t *dataSize);
void parseTimeStamp(FILE *file);
void parseStreamID(FILE *file);
void parseVideoData(FILE *file, uint32_t dataSize);

int initWithFile(const char* fileName) {
    if ((file = fopen(fileName, "r")) == NULL) {
        printf("Failed to open file %s\n", fileName);
        return FAILURE;
    }
    
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    printf("文件长度%lu\n", fileLen);
    fseek(file, 0, SEEK_SET);
    
    return SUCCESS;
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
            parseScriptData(file, dataSize);
        }
        printSeperator();
    }
}

void parsePreviousTagLength(FILE *file) {
    uint32_t tagSize;
    readOrExit(&tagSize, sizeof(char), 4, file, "读取前一个Tag长度错误");
    if(CPU_ENDIAN_SMALL) flip32(&tagSize);
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
    if(CPU_ENDIAN_SMALL) flip24(dataSize);
    printf("data size是%u\n", *dataSize);
}

void parseTimeStamp(FILE *file) {
    uint32_t timeStampLow = 0;
    readOrExit(&timeStampLow, sizeof(char), 3, file, "读取time stamp失败");
    if(CPU_ENDIAN_SMALL) flip24(&timeStampLow);
    printf("time stampe是%u\n", timeStampLow);
}

void parseStreamID(FILE *file) {
    uint32_t streamID = 0;
    readOrExit(&streamID, sizeof(char), 3, file, "读取sream id失败");
    if(CPU_ENDIAN_SMALL) flip24(&streamID);
    printf("streamID是%u\n", streamID);
}
