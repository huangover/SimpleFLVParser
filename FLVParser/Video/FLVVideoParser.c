//
//  FLVVideoParser.c
//  FLVParser
//
//  Created by SihangHuang on 2019/9/26.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#include "FLVVideoParser.h"
#include "FLVAvcVideoPacketParser.h"
#include "FLVParserUtil.h"
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#define BASELINE_PROFILE 0x66
#define MAIN_PROFILE 0x77

const char * const Video_Frame_Type[] = {"!!视频帧类型出错!!", "keyframe", "inter frame", "disposable inter frame",
    "generated keyframe", "video info/command frame"};
const char * const Video_Codecs[] = {"!!视频编码器出错！！", "JPEG", "H263", "Sreen video", "On2 Vp6",
    "On2 Vp6 with alpha", "Screen video v2", "AVC"};
    
void parseVideoData(FILE *file, uint32_t dataSize) {
    
    char *logMessage = createLogString();
    
    char param;
    readOrExit(&param, sizeof(char), 1, file, "读取视频参数失败");
    
    int frame = (int)((param & 0xf0) >> 4);
    fp_strcatMultiple(&logMessage, "视频帧类型: ", Video_Frame_Type[frame], " ", "");
    
    int codec = (int)(param & 0x0f);
    fp_strcatMultiple(&logMessage, "视频编码器: ", Video_Codecs[codec], " ", "");
    
    const int video_info_or_command_frame = 5;
    
    if (frame == video_info_or_command_frame) {
        uint8_t param;
        readOrExit(&param, sizeof(char), 1, file, "读取数据失败");
        if (param == 0) {
            fp_strcatMultiple(&logMessage, "内容：", "start of client-side seeking video frame sequence\n", "");
        } else {
            fp_strcatMultiple(&logMessage, "内容：", "end of client-side seeking video frame sequence\n", "");
        }
    } else {
        const int h263_video_packet = 2;
        const int screen_video_packet = 3;
        const int vp6_flv_video_packet = 4;
        const int vp6_flv_alpha_video_packet = 5;
        const int screen_v2_video_packet = 6;
        const int avc_video_packet = 7;

        switch (codec) {
            case h263_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示h263_video数据内容\n", ""); break;
            case screen_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示screen_video数据内容\n", ""); break;
            case vp6_flv_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示vp6_flv_video数据内容\n", ""); break;
            case vp6_flv_alpha_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示vp6_flv_alpha_video数据内容\n", ""); break;
            case screen_v2_video_packet:
                fp_strcatMultiple(&logMessage, "暂不支持显示screen_v2_video数据内容\n", ""); break;
            case avc_video_packet: {
                    long pos = ftell(file);
                
                    if (!parseAvcVideoPacket(file, &logMessage, dataSize - 1)) {
                        fp_strcatMultiple(&logMessage, "跳过出错的avc video packet，继续解析下一段数据......\n");
                        fseek(file, pos, SEEK_SET);
                        fseek(file, dataSize-1, SEEK_CUR); // 跳过实际数据
                    }
                }
                break;
            default:
                fp_strcat(&logMessage, "错误的video packet类型！！！！！！"); break;
        }
        
        if (codec != avc_video_packet) {
            fseek(file, dataSize - 1, SEEK_CUR); // 跳过不支持的视频数据
        }
    }
    
    puts(logMessage);
    free(logMessage);
}
