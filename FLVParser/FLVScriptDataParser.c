//
//  FLVScriptDataParser.c
//  FLVParser
//
//  Created by SihangHuang on 2019/9/25.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVScriptDataParser.h"
#include "FLVParserUtil.h"
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#define META_DATA_END 0x000009

/*
 解析SCRIPT DATA STRING对象
 返回值：1 成功；0 失败
 */
int parseString(FILE *file, char **logMessage);
/*
 解析SCRIPT DATA LONG STRING对象
 返回值：1 成功；0 失败
 */
int parseLongString(FILE *file, char **logMessage);
/*
 解析SCRIPT DATA DATE对象
 返回值：1 成功；0 失败
 */
int parseDate(FILE *file, char **logMessage);
/*
 解析SCRIPT DATA VARIABLE数组
 返回值：1 成功；0 失败
 */
int parseVariableArray(FILE *file, int arrayLen, char **logMessage);
/*
 解析SCRIPT DATA VALUE对象
 返回值：1 成功；0 失败
 */
int parseValue(FILE *file, char **logMessage);
/*
 预读0x000009结束符(文件偏移量会被重置)。SCRIPT DATA OBJECT END和SCRIPT DATA VARIABLE END均为此值
 */
uint32_t preReadMetaDataEndSymbol(FILE *file);
/*
 读取0x000009结束符。SCRIPT DATA OBJECT END和SCRIPT DATA VARIABLE END均为此值
 */
uint32_t readMetaDataEndSymbol(FILE *file);

#pragma mark - Public

void parseScriptData(FILE *file, uint32_t dataSize) {
    bool parseError = false;
    long pos = ftell(file);
    uint32_t endSymbol = preReadMetaDataEndSymbol(file);
    char *logMessage = createLogString();
    
    while (endSymbol != META_DATA_END) {
        if (!parseValue(file, &logMessage)) {
            parseError = true;
            puts(logMessage);
            break;
        }
        puts(logMessage);
        endSymbol = preReadMetaDataEndSymbol(file);
    }
    
    free(logMessage);
    
    if (parseError) {
        fseek(file, pos, SEEK_SET); // 重置文件偏移量
        fseek(file, dataSize, SEEK_CUR); // 跳过script data段，进入其他数据解析
        puts("❗❗❗metadata解析失败❗❗❗");
    } else {
        fseek(file, 3, SEEK_CUR); // 跳过SCRIPT DATA OBJECT END
        puts("metadata解析完毕");
        printSeperator();
    }
}

#pragma mark - Private

int parseString(FILE *file, char **logMessage) {
    uint16_t nameLen;
    if (!readOrReturn(&nameLen, sizeof(uint16_t), 1, file, "读取string的长度失败")) {
        return FAILURE;
    }
    if (CPU_ENDIAN_SMALL) flip16(&nameLen);
    char *name = calloc(nameLen + 2, sizeof(char)); // +1 为了在最后放:+一个空格
    if (!readOrReturn(name, sizeof(char), nameLen, file, "读取string的值失败")) {
        return FAILURE;
    }
    
    fp_strcat(logMessage, strcat(name, ": "));
    free(name);
    
    return SUCCESS;
}

int parseLongString(FILE *file, char **logMessage) {
    uint32_t nameLen;
    if (!readOrReturn(&nameLen, sizeof(uint16_t), 1, file, "读取string的长度失败")) {
        return FAILURE;
    }
    if (CPU_ENDIAN_SMALL) flip32(&nameLen);
    
    char *name = calloc(nameLen + 2, sizeof(char)); // +1 为了在最后放:+一个空格
    if (!readOrReturn(name, sizeof(char), nameLen, file, "读取string的值失败")) {
        return FAILURE;
    }
    
    fp_strcat(logMessage, strcat(name, ": "));
    free(name);
    
    return SUCCESS;
}

int parseDate(FILE *file, char **logMessage) {
    
    char *m = calloc(30, sizeof(char)); // +1 为了在最后放:+一个空格
    
    uint64_t seconds = 0;
    if (!readOrReturn(&seconds, sizeof(uint64_t), 1, file, "读取毫秒失败")) {
        return FAILURE;
    }
    
    sprintf(m, "时间(毫秒):%llu:\n", seconds);
    
    int16_t timeOffset = 0;
    if (!readOrReturn(&timeOffset, sizeof(int16_t), 1, file, "读取时间偏移量失败")) {
        return FAILURE;
    }
    
    memset(m, 0, 30);
    sprintf(m, "时间偏移(毫秒):%d:\n", timeOffset);
    
    return SUCCESS;
}

int parseVariableArray(FILE *file, int arrayLen, char **logMessage) {
    while (arrayLen > 0) {
        if (!parseString(file, logMessage)) {
            return FAILURE;
        }
        if (!parseValue(file, logMessage)) {
            return FAILURE;
        }
        arrayLen--;
    }
    
    return SUCCESS;
}

int parseStrictArray(FILE *file, char **logMessage) {
    
    uint32_t len = 0;
    if (!readOrReturn(&len, sizeof(uint32_t), 1, file, "读取strict array类型的数组长度失败")) {
        fp_strcat(logMessage, "读取strict array类型的数组长度失败");
        return FAILURE;
    }
    
    return parseVariableArray(file, len, logMessage);
}

int parseValue(FILE *file, char **logMessage) {
    //================= type =================
    uint8_t valueType;
    if (!readOrReturn(&valueType, sizeof(uint8_t), 1, file, "读取metadata属性值的类型失败")) {
        return FAILURE;
    }
    
    //================= ECMA Array Length (Optional) =================
    uint32_t ecmaArrayLen = 0;
    if (valueType == 8) {
        if (!readOrReturn(&ecmaArrayLen, sizeof(uint32_t), 1, file, "读取ECMA Array长度出错")) {
            return FAILURE;
        }

        if(CPU_ENDIAN_SMALL) flip32(&ecmaArrayLen);
        char m[20];
        sprintf(m, "(大概长度%d):\n", ecmaArrayLen);
        fp_strcat(logMessage, m);
    }
    
    //================= script data value =================
    if (valueType == 0) {
        uint64_t value;
        if (!readOrReturn(&value, sizeof(uint64_t), 1, file, "读取number的值失败")) {
            return FAILURE;
        }
        if (CPU_ENDIAN_SMALL) flip64(&value);
        char m[50];
        sprintf(m, " %llu\n", value);
        fp_strcat(logMessage, m);
        
    } else if (valueType == 1) {
        uint8_t value;
        readOrExit(&value, sizeof(uint8_t), 1, file, "读取boolean的值失败");
        
        char m[50];
        sprintf(m, " %d\n", value);
        fp_strcat(logMessage, m);
        
    } else if (valueType == 2 || valueType == 4) {
        parseString(file, logMessage);
    } else if (valueType == 3) {
        // TO-DO: script_data_object[n]，n未知
        fp_strcat(logMessage, "类型为object的metadata字段解析失败");
        return FAILURE;
    } else if (valueType == 7) {
        uint16_t value;
        if (!readOrReturn(&value, sizeof(uint16_t), 1, file, "读取reference的值失败")) {
            return FAILURE;
        }
        if (CPU_ENDIAN_SMALL) flip16(&value);
        char m[50];
        sprintf(m, " %d\n", value);
        fp_strcat(logMessage, m);
        
    } else if (valueType == 8) {
        return parseVariableArray(file, ecmaArrayLen, logMessage);
    } else if (valueType == 10) {
        return parseStrictArray(file, logMessage);
    } else if (valueType == 11) {
        return parseDate(file, logMessage);
    } else if (valueType == 12) {
        return parseLongString(file, logMessage);
    }
    
    if (valueType == 3 || valueType == 8) {
        if (readMetaDataEndSymbol(file) != META_DATA_END) {
            fp_strcat(logMessage, "类型为ECMA和ECMA数组必须以0x000009结尾");
            return FAILURE;
        }
    }
    
    return SUCCESS;
}

uint32_t readMetaDataEndSymbol(FILE *file) {
    uint32_t endSymbol = 0;
    readOrExit(&endSymbol, sizeof(char), 3, file, "预读取meta终止符失败");
    
    if(CPU_ENDIAN_SMALL) flip24(&endSymbol);
    
    return endSymbol;
}

uint32_t preReadMetaDataEndSymbol(FILE *file) {
    uint32_t endSymbol = readMetaDataEndSymbol(file);
    fseek(file, -3, SEEK_CUR); // 偏移量退回去
    return endSymbol;
}

