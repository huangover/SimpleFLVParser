//
//  FLVParserUtil.h
//  FLVParser
//
//  Created by SihangHuang on 2019/9/25.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#ifndef FLVParserUtil_h
#define FLVParserUtil_h

#include <stdio.h>
#include <stdarg.h>

#define CPU_ENDIAN_SMALL computeEndian()
#define SUCCESS 1
#define FAILURE 0
//typedef struct {
//    int len;
//    char *dst;
//} LogMessage;

int computeEndian(void);
//LogMessage *createLogString(void);
//void fp_strcat(LogMessage *logMessage, const char *source);
char *createLogString(void);
void fp_strcat(char **dst, const char *source);
void fp_strcatMultiple(char **dst, ...);

void flip16(void *i);
void flip24(void *i);
void flip32(void *i);
void flip64(void *i);
/*
 读取一定数量的字节，如果失败，打印errMsg，退出程序
 */
void readOrExit(void * __restrict __ptr, size_t __size, size_t __nitems, FILE * __restrict __stream, char *errMsg);
/*
 读取一定数量的字节，如果失败，打印errMsg，并返回数值
 返回值：1 成功；0 失败
 */
int readOrReturn(void * __restrict __ptr, size_t __size, size_t __nitems, FILE * __restrict __stream, char *errMsg);
/*
 打印errMsg，退出程序
 */
void printAndExit(char *errMsg);
void printSeperator(void);

#endif /* FLVParserUtil_h */
