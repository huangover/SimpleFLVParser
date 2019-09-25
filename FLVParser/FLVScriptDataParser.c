//
//  FLVScriptDataParser.c
//  FLVParser
//
//  Created by SihangHuang on 2019/9/25.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVScriptDataParser.h"
#include "FLVParser.h"
#include "FLVParserUtil.h"
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#define META_DATA_END 0x000009

/*
 解析SCRIPT DATA STRING对象
 返回值：1 成功；0 失败
 */
int parseScriptDataString(FILE *file, char **logMessage);
/*
 解析SCRIPT DATA VARIABLE数组
 返回值：1 成功；0 失败
 */
int parseScriptDataVariableArray(FILE *file, int arrayLen, char **logMessage);
/*
 解析SCRIPT DATA VALUE对象
 返回值：1 成功；0 失败
 */
int parseScriptDataValue(FILE *file, char **logMessage);
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
        if (!parseScriptDataValue(file, &logMessage)) {
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

int parseScriptDataString(FILE *file, char **logMessage) {
    uint16_t nameLen;
    if (!readOrReturn(&nameLen, sizeof(uint16_t), 1, file, "读取string的长度失败")) {
        return 0;
    }
#ifdef CPU_ENDIAN_SMALL
    flip16(&nameLen);
#endif
    char *name = calloc(nameLen + 2, sizeof(char)); // +1 为了在最后放:+一个空格
    if (!readOrReturn(name, sizeof(char), nameLen, file, "读取string的值失败")) {
        return 0;
    }
    
    fp_strcat(logMessage, strcat(name, ": "));
    free(name);
    
    return 1;
}

int parseScriptDataVariableArray(FILE *file, int arrayLen, char **logMessage) {
    while (arrayLen > 0) {
        if (!parseScriptDataString(file, logMessage)) {
            return 0;
        }
        if (!parseScriptDataValue(file, logMessage)) {
            return 0;
        }
        arrayLen--;
    }
    
    // ECMA数组以0x000009结束
    if (readMetaDataEndSymbol(file) != META_DATA_END) {
        return 0;
    }
    
    return 1;
}

int parseScriptDataValue(FILE *file, char **logMessage) {
    //================= 解析ScriptDataValue ===================
    //================= type =================
    uint8_t valueType;
    if (!readOrReturn(&valueType, sizeof(uint8_t), 1, file, "读取metadata属性值的类型失败")) {
        return 0;
    }
    switch (valueType) {
        case 0:
            fp_strcat(logMessage, "number类型 ");
            break;
        case 1:
            //            logMessage = fp_strcat(&logMessage, "boolean类型");
            break;
        case 2:
            fp_strcat(logMessage, "string类型 ");
            break;
        case 3:
            //            logMessage = fp_strcat(&logMessage, "object类型");
            break;
        case 4:
            //            logMessage = fp_strcat(&logMessage, "movie clip类型");
            break;
        case 5:
            //            logMessage = fp_strcat(&logMessage, "null类型");
            break;
        case 6:
            //            logMessage = fp_strcat(&logMessage, "undefined类型");
            break;
        case 7:
            //            logMessage = fp_strcat(&logMessage, "reference类型");
            break;
        case 8:
            fp_strcat(logMessage, "ECMA数组类型 ");
            break;
        case 10:
            //            logMessage = fp_strcat(&logMessage, "strict array类型");
            break;
        case 11:
            //            logMessage = fp_strcat(&logMessage, "date类型");
            break;
        case 12:
            //            logMessage = fp_strcat(&logMessage, "long string类型");
            break;
        default:
            //logMessage = fp_strcat(&logMessage, "非法metadata类型");
            return 0;
    }
    
    //================= ECMA Array Length (Optional) =================
    uint32_t ecmaArrayLen = 0;
    if (valueType == 8) {
        if (!readOrReturn(&ecmaArrayLen, sizeof(uint32_t), 1, file, "读取ECMA Array长度出错")) {
            return 0;
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
            return 0;
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
        parseScriptDataString(file, logMessage);
    } else if (valueType == 7) {
        uint16_t value;
        if (!readOrReturn(&value, sizeof(uint16_t), 1, file, "读取reference的值失败")) {
            return 0;
        }
#ifdef CPU_ENDIAN_SMALL
        flip16(&value);
#endif
        char m[50];
        sprintf(m, " %d\n", value);
        fp_strcat(logMessage, m);
        
    } else if (valueType == 8) {
        return parseScriptDataVariableArray(file, ecmaArrayLen, logMessage);
    } else if (valueType == 10) {
        
    } else if (valueType == 11) {
        
    } else if (valueType == 12) {
        
    }
    
    return 1;
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

