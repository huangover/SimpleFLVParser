//
//  FLVParserUtil.c
//  FLVParser
//
//  Created by SihangHuang on 2019/9/25.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVParserUtil.h"
#include <stdlib.h>

void flip(void *i, int size);
void flips(void *i, int size, int startIndex);

int computeEndian() {
    static int i = -1;
    int x = 1;
    if (i != -1) {
        return i;
    } else {
        i = (*(char *)&x == 1);
        return i;
    }
}

void flip16(uint16_t *i) {
    if(CPU_ENDIAN_SMALL) flip(i, 16);
}

void flip32(uint32_t *i) {
    if(CPU_ENDIAN_SMALL) flip(i, 32);
}

void flip24(uint32_t *i) {
    // 32位ABCD
    // fread读3个字节，把数值填充到abc上
    if(CPU_ENDIAN_SMALL) flip(i, 24);
}

void flip64(uint64_t *i) {
    if(CPU_ENDIAN_SMALL) flip(i, 64);
}

void flip(void *i, int size) {
    if(CPU_ENDIAN_SMALL) flips(i, size, 0);
}

void flips(void *i, int size, int startIndex) {
    if(!CPU_ENDIAN_SMALL) return;

    if (size == 8 || size % 8 != 0) return;
    
    char *data = i;
    int count = size / 8;
    for (int i = startIndex; i < count / 2; i++) {
        char temp = data[i];
        data[i] = data[count - 1 - i];
        data[count - 1 - i] = temp;
    }
}

void readOrExit(void * __restrict __ptr, size_t __size, size_t __nitems, FILE * __restrict __stream, char *errMsg) {
    if (fread(__ptr, __size, __nitems, __stream) != __nitems) {
        puts(errMsg);
        exit(EXIT_FAILURE);
    }
}

int readOrReturn(void * __restrict __ptr, size_t __size, size_t __nitems, FILE * __restrict __stream, char *errMsg) {
    if (fread(__ptr, __size, __nitems, __stream) != __nitems) {
        puts(errMsg);
        return 0;
    }
    
    return 1;
}

void printAndExit(char *errMsg) {
    puts(errMsg);
    exit(EXIT_FAILURE);
}

void printSeperator(void) {
    puts("\n============================================\n");
}
