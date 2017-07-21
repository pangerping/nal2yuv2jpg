#ifndef __TRANS_FORMAT_
#define __TRANS_FORMAT_

typedef enum FORMAT{
    FORMAT_ERROR = -1,
    FORMAT_YUV = 1,
    FORMAT_NAL,
    FORMAT_JPG,
    FORMAT_RTSP
}FORMAT;

typedef struct params{
    int width;
    int height;
    enum FORMAT src_type;
    enum FORMAT dst_type;
    char src_uri[1024];
    char dst_uri[1024];
}Params;

void dispatch(Params* params);

#endif// __TRANS_FORMAT_
