#ifndef __TOOLS_H
#define __TOOLS_H

void trans_yuv2jpg(char* src_uri, char* dst_uri, int width, int height);
void trans_nal2yuv(char* src_uri, char* dst_uri);
void trans_nal2jpg(char* src_uri, char* dst_uri);
void trans_jpg2yuv(char* src_uri, char* dst_uri);

void trans_rtsp2yuv(char* src_uri);
void trans_rtsp2nal(char* src_uri);
void trans_rtsp2jpg(char* src_uri);

#endif// __TOOLS_H
