#include "utils.h"
#include "tools.h"

void dispatch(Params* params)
{
    switch(params->src_type){
    case FORMAT_RTSP:
        switch(params->dst_type){
        case FORMAT_YUV:
            trans_rtsp2yuv(params->src_uri);
            break;
        case FORMAT_NAL:
            trans_rtsp2nal(params->src_uri);
            break;
        case FORMAT_JPG:
            trans_rtsp2jpg(params->src_uri);
            break;
        default:
            break;
        }
        break;
    case FORMAT_YUV:
        switch(params->dst_type){
        case FORMAT_YUV:
        case FORMAT_NAL:
            break;
        case FORMAT_JPG:
            trans_yuv2jpg(params->src_uri, params->dst_uri, params->width, params->height);
            break;
        default:
            break;
        }
        break;
    case FORMAT_NAL:
        switch(params->dst_type){
        case FORMAT_YUV:
            trans_nal2yuv(params->src_uri, params->dst_uri);
            break;
        case FORMAT_NAL:
            break;
        case FORMAT_JPG:
            trans_nal2jpg(params->src_uri, params->dst_uri);
            break;
        default:
            break;
        }
        break;
    case FORMAT_JPG:
        switch(params->dst_type){
        case FORMAT_NAL:
        case FORMAT_JPG:
            break;
        case FORMAT_YUV:
            trans_jpg2yuv(params->src_uri, params->dst_uri);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}
