//
//  FLVAvcVideoPacketParser.c
//  FLVParser
//
//  Created by SihangHuang on 2019/10/9.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVAvcVideoPacketParser.h"
#include "FLVParserUtil.h"

int parseAvcVideoPacket(FILE *file, char **logMessage, uint32_t dataSize) {
    // AVCPacketType
    uint8_t type;
    if (!readOrReturn(&type, sizeof(char), 1, file, "读取AVCPacketType出错！！！！！！")) {
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
    if (!readOrReturn(&compositionTime, sizeof(char), 3, file, "读取Composition time出错！！！！！！")) {
        return FAILURE;
    }
    if (CPU_ENDIAN_SMALL) flip24(&compositionTime);
    
    char m[50];
    sprintf(m, "composition time: %d ", compositionTime);
    if (type != 1 && compositionTime != 0) {
        fp_strcat(logMessage, "当AVCPacketType为1时，composition time必须为0，出错！！！！！！");
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
