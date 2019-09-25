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

#define CPU_ENDIAN_SMALL computeEndian()

int computeEndian(void);
void flip16(uint16_t *i);
void flip24(uint32_t *i);
void flip32(uint32_t *i);
void flip64(uint64_t *i);
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
