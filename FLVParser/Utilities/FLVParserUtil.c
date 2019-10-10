//
//  FLVParserUtil.c
//  FLVParser
//
//  Created by SihangHuang on 2019/9/25.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVParserUtil.h"
#include <stdlib.h>
#include <strings.h>

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

typedef struct {
    int len;
    char *logString;
} LogMessage;

const int LOG_STRING_INITIAL_LEN = 300;
static int LOG_STRING_LEN = LOG_STRING_INITIAL_LEN;

//LogMessage *createLogString(void) {
//    LogMessage *log = malloc(sizeof(LogMessage));
//    log->len = 300;
//    log->dst = calloc(sizeof(char), log->len);
//
//    return log;
//}
//
//void fp_strcat(LogMessage *logMessage, const char *source) {
//    if (strlen(logMessage->dst) + strlen(source) < logMessage->len) {
//        strcat(logMessage->dst, source);
//    } else {
//        logMessage->len *= 2;
//        char *new = calloc(sizeof(char), logMessage->len);
//        strcpy(new, logMessage->dst);
//        free(logMessage->dst);
//        logMessage->dst = new;
//        fp_strcat(logMessage, source);
//    }
//}

char *createLogString(void) {
    char *logString = calloc(sizeof(char), LOG_STRING_INITIAL_LEN);
    return logString;
}

void fp_strcat(char **dst, const char *source) {
    if (strlen(*dst) + strlen(source) < LOG_STRING_LEN) {
        strcat(*dst, source);
    } else {
        LOG_STRING_LEN *= 2;
        char *newA = calloc(sizeof(char), LOG_STRING_LEN);
        strcpy(newA, *dst);
        free(*dst);
        *dst = newA;
        fp_strcat(dst, source);
    }
}

void fp_strcatMultiple(char **dst, ...) {
    va_list ap;
    va_start(ap, dst);
    const char *c;
    
    while (1) {
        c = va_arg(ap, const char*);
        if (strcmp(c, "") == 0) break;
        fp_strcat(dst, c);
    }
    va_end(ap);
}

void flip16(void *i) {
    if(CPU_ENDIAN_SMALL) flip(i, 16);
}

void flip32(void *i) {
    if(CPU_ENDIAN_SMALL) flip(i, 32);
}

void flip24(void *i) {
    // 32位ABCD
    // fread读3个字节，把数值填充到abc上
    if(CPU_ENDIAN_SMALL) flip(i, 24);
}

void flip64(void *i) {
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
        return FAILURE;
    }
    
    return SUCCESS;
}

void printAndExit(char *errMsg) {
    puts(errMsg);
    exit(EXIT_FAILURE);
}

void printSeperator(void) {
    puts("\n============================================\n");
}
