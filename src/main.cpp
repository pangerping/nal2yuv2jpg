#include <sys/resource.h>
#include <gflags/gflags.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

DEFINE_string(src_type, "nal", "input  format. eg. yuv, nal, rtsp.");
DEFINE_string(dst_type, "jpg", "output format. eg. yuv, nal, jpg");
DEFINE_int32(width, 0, "image width.");
DEFINE_int32(height, 0, "image height.");
DEFINE_string(src_uri, "", "input src file.");

void debugCore()
{
	struct rlimit r;
	if(getrlimit(RLIMIT_CORE, &r) < 0)
	{
		printf("getrlimit error\n");
		return ;
	}
	r.rlim_cur = -1;
	r.rlim_max = -1;
	if(setrlimit(RLIMIT_CORE, &r) < 0)
	{
		printf("setrlimit error\n");
		return ;
	}
}

static bool ValidateSrcType(const char* flagname, const std::string& value) 
{
    if(strcmp("yuv", FLAGS_src_type.c_str()) == 0 || strcmp("nal", FLAGS_src_type.c_str()) == 0 
       || strcmp("jpg", FLAGS_src_type.c_str()) == 0 || strcmp("rtsp", FLAGS_src_type.c_str()) == 0)
        return true;
    printf("Invalid value for --%s: %s\n", flagname, (char*)value.c_str());
    return false;
}

static bool ValidateDstType(const char* flagname, const std::string& value) 
{
    if(strcmp("yuv", FLAGS_dst_type.c_str()) == 0 || strcmp("nal", FLAGS_dst_type.c_str()) == 0 || strcmp("jpg", FLAGS_dst_type.c_str()) == 0)
        return true;
    printf("Invalid value for --%s: %s\n", flagname, (char*)value.c_str());
    return false;
}

static bool ValidateSrcUri(const char* flagname, const std::string& value) 
{
    if(strcmp(value.c_str(), "") != 0)
        return true;
    printf("Invalid value for --%s: %s\n", flagname, (char*)value.c_str());
    return false;
}

static bool ValidateWidth(const char* flagname, int32_t value) 
{
    if(value != 0)
        return true;
    printf("Invalid value for --%s: %d\n", flagname, value);
    return false;
}

static bool ValidateHeight(const char* flagname, int32_t value) 
{
    if(value != 0)
        return true;
    printf("Invalid value for --%s: %d\n", flagname, value);
    return false;
}

static int usage_message(char *argv[])
{
    int ret = 0;
    return ret;
}

int main(int argc, char* argv[])
{
    debugCore();
    if(argc == 1){
        return -1;
    }

    bool ret;
    std::string usage("\n \n \n This is trans tools. Sample usage:\n");
    usage += std::string(argv[0]) + " -src_type yuv -dst_type jpg -width 1280 -height 720 -src_uri 1.yuv\n";

    google::SetUsageMessage(usage);

    ret  = google::RegisterFlagValidator(&FLAGS_src_type, &ValidateSrcType);
    ret &= google::RegisterFlagValidator(&FLAGS_dst_type, &ValidateDstType);
    ret &= google::RegisterFlagValidator(&FLAGS_src_uri,  &ValidateSrcUri);
    //ret &= google::RegisterFlagValidator(&FLAGS_width,    &ValidateWidth);
    //ret &= google::RegisterFlagValidator(&FLAGS_height,   &ValidateHeight);
    if(ret == false)
        return -1;

    google::ParseCommandLineFlags(&argc, &argv, true);

    Params params;
    memset(&params, 0, sizeof(Params));
    
    params.src_type = FORMAT_ERROR;
    params.dst_type = FORMAT_ERROR;
    
    if(strcmp("yuv", FLAGS_src_type.c_str()) == 0)
        params.src_type = FORMAT_YUV;
    if(strcmp("nal", FLAGS_src_type.c_str()) == 0)
        params.src_type = FORMAT_NAL;
    if(strcmp("jpg", FLAGS_src_type.c_str()) == 0)
        params.src_type = FORMAT_JPG;
   if(strcmp("rtsp", FLAGS_src_type.c_str()) == 0)
        params.src_type = FORMAT_RTSP;

    if(strcmp("yuv", FLAGS_dst_type.c_str()) == 0)    
        params.dst_type = FORMAT_YUV;
    if(strcmp("nal", FLAGS_dst_type.c_str()) == 0)    
        params.dst_type = FORMAT_NAL;
    if(strcmp("jpg", FLAGS_dst_type.c_str()) == 0)    
        params.dst_type = FORMAT_JPG;
     
    if(params.src_type == FORMAT_ERROR || params.dst_type == FORMAT_ERROR){
        return -1;
    }
    
    params.width  = FLAGS_width;   
    params.height = FLAGS_height;
    
    sprintf(params.src_uri, "%s", FLAGS_src_uri.c_str());
    char suffix[5];
    switch(params.dst_type){
    case FORMAT_YUV:
        sprintf(suffix, "%s", "yuv");
        break;
    case FORMAT_JPG:
        sprintf(suffix, "%s", "jpg");
        break;
    case FORMAT_NAL:
        sprintf(suffix, "%s", "h264");
        break;
    default:
        break;
    }
    sprintf(params.dst_uri, "%s_%s.%s", FLAGS_src_uri.c_str(), FLAGS_dst_type.c_str(), suffix);
    
    if(params.src_type == FORMAT_YUV){

        if(params.width == 0 || params.height == 0){
            printf("Invalid value for --%s: %d - %d\n", "width or height", params.width, params.height);
            return -1;
        }
        FILE* fp = fopen(params.src_uri, "rb");
        if(fp == NULL){
            printf("open src uri failed\n");
            return -1;
        }
        fseek(fp, 0, SEEK_END);
        int file_len;
        file_len = ftell(fp); 
        fclose(fp);
        if(params.width * params.height * 3 / 2 != file_len){
            printf("src data size is not equl to width * height * 3 / 2: %d != %d (%d*%d*3/2)\n", 
                      file_len, params.width * params.height * 3 / 2, params.width, params.height);
            return -1;
        }
    }
    
    dispatch(&params); 
    
    return 0;
}
