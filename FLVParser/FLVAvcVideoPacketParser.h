//
//  FLVAvcVideoPacketParser.h
//  FLVParser
//
//  Created by SihangHuang on 2019/10/9.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#ifndef FLVAvcVideoPacketParser_h
#define FLVAvcVideoPacketParser_h

#include <stdio.h>

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
 */
int parseAvcDecoderConfigRecord(FILE *file, char **logMessage);

#endif /* FLVAvcVideoPacketParser_h */
