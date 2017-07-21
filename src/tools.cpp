#include <sys/types.h>
#include <unistd.h>

extern "C"{
#include <libavformat/avformat.h>
}

#include "tools.h"

typedef struct Context
{
    AVCodecContext* decoder;
    AVCodecContext* jpg_encoder;
    AVFrame* frame;
    AVPacket pkt;
    int width;
    int height;
    char src_uri[1024];
    char dst_uri[1024];
    char token[1024];
    
    //rtsp
    AVFormatContext* fmt_ctx;
    int video_stream_index;
    int audio_stream_index;
    AVStream* video_stream;
    AVStream* audio_stream;
}Context;

static int CreateJPGEncoderContext(Context* ctx)
{
    AVCodec *codec = NULL;
    AVCodecContext *c = NULL;

    av_register_all();

    if(ctx->width == 0 || ctx->height == 0) {
        av_log(NULL, AV_LOG_ERROR, "video encoder width or height error! token = %s\n", ctx->token);
    }   
    codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(!codec) {
        av_log(NULL, AV_LOG_ERROR, "video encoder Codec not found, token = %s\n", ctx->token);
        goto err;
    }   
    c = avcodec_alloc_context3(codec);
    if(!c) {
        av_log(NULL, AV_LOG_ERROR, "video encoder AVCodecCOntext alloc not sucess, token = %s\n", ctx->token);
        goto err;
    }   

    c->width = ctx->width;
    c->height = ctx->height;

    c->pix_fmt = AV_PIX_FMT_YUVJ420P;
    c->time_base = (AVRational ) { 1, 25 };

    if(avcodec_open2(c, codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "encoder Could not open codec, token = %s\n", ctx->token);
        goto err;
    }   

    ctx->jpg_encoder = c;
    return 0;
err:
    if(c) 
        avcodec_free_context(&c);
    av_log(NULL, AV_LOG_ERROR, "init mjpeg encoder failed! token = %s\n", ctx->token);
    return -1;
}


static int save_jpg(char* dst_uri, uint8_t* data, int size)
{
    FILE* out_file = NULL;
    out_file = fopen(dst_uri, "w+");
    if(out_file == NULL){
        av_log(NULL, AV_LOG_ERROR, "Open jpeg file failed\n");
        return -1;
    }
    int ret = fwrite(data, 1, size, out_file);
    if(ret != size){
        av_log(NULL, AV_LOG_ERROR, "write jpeg picture failed\n");
        fclose(out_file);
        return -1;
    }
    fclose(out_file);
	return 0;
}

static int YUV2JPG(Context* ctx)
{
    int ret = 0;
    int got_packet = 0;
    ret = avcodec_encode_video2(ctx->jpg_encoder, &ctx->pkt, ctx->frame, &got_packet);

    if (ret < 0 || got_packet == 0){
        av_log(NULL, AV_LOG_WARNING, "ADT Encoder video failed! token = %s\n", ctx->token);
        return -1;
    }
    else if (got_packet){
        save_jpg(ctx->dst_uri, ctx->pkt.data, ctx->pkt.size);
    }
    return 0;
}

static int DestroyContext(Context* ctx)
{
    av_free_packet(&ctx->pkt);
    if(ctx->frame)
        av_frame_free(&(ctx->frame));

    if(ctx->jpg_encoder){
        avcodec_flush_buffers(ctx->jpg_encoder);
        avcodec_free_context(&ctx->jpg_encoder);
        av_free(ctx->jpg_encoder);
    }
    if(ctx->decoder){
        avcodec_flush_buffers(ctx->decoder);
        avcodec_free_context(&ctx->decoder);
        av_free(ctx->decoder);
    }

    av_free(ctx);
    ctx = NULL;
    return 0;
}

void trans_yuv2jpg(char* src_uri, char* dst_uri, int width, int height)
{
    int ret;
    Context* ctx = (Context* )malloc(sizeof(Context));
    memset(ctx, 0, sizeof(Context));
    ctx->width = width;
    ctx->height = height;
    sprintf(ctx->src_uri, "%s", src_uri);
    sprintf(ctx->dst_uri, "%s", dst_uri);
    sprintf(ctx->token, "%s", "biubiubiu");
    
    ret = CreateJPGEncoderContext(ctx);

    if(ret < 0)
        return;
    char* data_buffer = (char* )malloc(width * height * 3 / 2);
    memset(data_buffer, 0, width * height * 3 / 2);

    FILE* fp = fopen(src_uri, "rb");
    fread(data_buffer, 1, width*height*3/2, fp);
    fclose(fp);

    ctx->frame = av_frame_alloc();
    av_init_packet(&ctx->pkt);    
    ctx->frame->data[0] = (uint8_t* )malloc(width * height);
    memcpy(ctx->frame->data[0], data_buffer, width * height);
    ctx->frame->data[1] = (uint8_t* )malloc(width * height / 4); 
    memcpy(ctx->frame->data[1], data_buffer + width * height, width * height / 4); 
    ctx->frame->data[2] = (uint8_t* )malloc(width * height / 4);
    memcpy(ctx->frame->data[2], data_buffer + width * height + width * height / 4, width * height / 4);

    ctx->frame->linesize[0] = width;
    ctx->frame->linesize[1] = width / 2;
    ctx->frame->linesize[2] = width / 2;
    ctx->frame->width = width;
    ctx->frame->height = height;

    ctx->frame->format = AV_PIX_FMT_YUV420P;
    
    YUV2JPG(ctx);
    //end
    free(data_buffer);
    DestroyContext(ctx);
}

static int CreateH264DecoderContext(Context* ctx)
{
	int ret = 0;
	AVDictionary *opts = NULL;
	AVCodecContext *context = NULL, *avctx = NULL;
	AVCodec *codec = NULL;
    av_register_all();

	codec = avcodec_find_decoder_by_name("h264");
	if (!codec){
		av_log(NULL, AV_LOG_ERROR, "video decoder AVCodec is not be fould, token = %s\n", ctx->token);
		goto err;
	}
	avctx = context = avcodec_alloc_context3(codec);
	if (!context){
		av_log(NULL, AV_LOG_ERROR, "video decoder AVCodecContext alloc no succss, token = %s\n", ctx->token);
		goto err;
	}
	avctx->workaround_bugs = 1;
	avctx->lowres = 0;
	if (avctx->lowres > codec->max_lowres) {
		av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d, token = %s\n", codec->max_lowres, ctx->token);
		avctx->lowres = codec->max_lowres;
	}
	avctx->idct_algo = 0;
	avctx->skip_frame = AVDISCARD_DEFAULT;
	avctx->skip_idct = AVDISCARD_DEFAULT;
	avctx->skip_loop_filter = AVDISCARD_DEFAULT;
	avctx->error_concealment = 3;
	if (avctx->lowres)
		avctx->flags |= CODEC_FLAG_EMU_EDGE;
	if (codec->capabilities & CODEC_CAP_DR1)
		avctx->flags |= CODEC_FLAG_EMU_EDGE;
	if (!av_dict_get(opts, "threads", NULL, 0))
		av_dict_set(&opts, "threads", "1", 0);
	if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
		av_dict_set(&opts, "refcounted_frames", "1", 0);

	context->codec_type = codec->type;
	context->codec_id = codec->id;

	context->flags |= CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(context, codec, &opts);
    if(ret < 0){
    	av_log(NULL, AV_LOG_ERROR, "video decoder avcodec open failed, token = %s\n", ctx->token);
    	goto err;
    }
    ctx->decoder = context;
	return 0;
err:
	if(context)
		avcodec_free_context(&context);
	av_log(NULL, AV_LOG_ERROR, "init video decoder failed! token = %s\n", ctx->token);
	return -1;
}

static int save_yuv(char* dst_uri, uint8_t *frame[], int linesize[], int width, int height)
{
    int ret = 0;
    FILE *fp;
    int y;
    fp = fopen(dst_uri, "wb");

    if (fp == NULL) {
        return -1;
    }

    for (y = 0; y < height; y++) {
        fwrite(frame[0] + linesize[0] * y, sizeof(uint8_t), width, fp);
    }
    for (y = 0; y < height / 2; y++) {
        fwrite(frame[1] + linesize[1] * y, sizeof(uint8_t), width / 2, fp);
    }
    for (y = 0; y < height / 2; y++) {
        fwrite(frame[2] + linesize[2] * y, sizeof(uint8_t), width / 2, fp);
    }

    fclose(fp);
    return ret;
}

static int NAL2YUV(Context* ctx)
{ 
	int ret;
	int got_frame;
	int got_packet;
    ret = avcodec_decode_video2(ctx->decoder, ctx->frame, &got_frame, (const AVPacket*)&ctx->pkt);
	if (ret < 0 || !got_frame){
		av_log(NULL, AV_LOG_WARNING, "ADT Decode video failed! token = %s\n", ctx->token);
		return -1;
	}else if (got_frame){
        
        ctx->width = ctx->decoder->width;
		ctx->height = ctx->decoder->height;
		save_yuv(ctx->dst_uri, ctx->frame->data, ctx->frame->linesize, ctx->width, ctx->height);
		
        return 0;
	}
	return -1;
}

void trans_nal2yuv(char* src_uri, char* dst_uri)
{
    int ret;
    Context* ctx = (Context* )malloc(sizeof(Context));
    memset(ctx, 0, sizeof(Context));
    sprintf(ctx->src_uri, "%s", src_uri);
    sprintf(ctx->dst_uri, "%s", dst_uri);
    sprintf(ctx->token, "%s", "biubiubiu");
    ctx->frame = av_frame_alloc();
    av_init_packet(&ctx->pkt);    
   
    FILE* fp = fopen(src_uri, "rb");
    fseek(fp, 0, SEEK_END);
    int file_len;
    file_len = ftell(fp); 
    fseek(fp, 0, SEEK_SET); 

    uint8_t* data_buffer = (uint8_t*)malloc(file_len);
    memset(data_buffer, 0, file_len);
    fread(data_buffer, 1, file_len, fp);
    fclose(fp);

    ctx->pkt.data = data_buffer;
    ctx->pkt.size = file_len;
    
    CreateH264DecoderContext(ctx);
    NAL2YUV(ctx);
    free(data_buffer);
    DestroyContext(ctx);
}

int NAL2JPG(Context* ctx)
{
	int ret;
	int got_frame;
	int got_packet;
    ret = avcodec_decode_video2(ctx->decoder, ctx->frame, &got_frame, (const AVPacket*)&ctx->pkt);
	if (ret < 0 || !got_frame){
		av_log(NULL, AV_LOG_WARNING, "ADT Decode video failed! token = %s\n", ctx->token);
		return -1;
	}else if (got_frame){

		ctx->width = ctx->decoder->width;
        ctx->height = ctx->decoder->height;

        CreateJPGEncoderContext(ctx);
        av_free_packet(&ctx->pkt);

        ret = avcodec_encode_video2(ctx->jpg_encoder, &ctx->pkt, ctx->frame, &got_packet);

        if (ret < 0 || got_packet == 0){
            av_log(NULL, AV_LOG_WARNING, "ADT Encoder video failed! token = %s\n", ctx->token);
            return -1;
        }
        else if (got_packet){

        	save_jpg(ctx->dst_uri, ctx->pkt.data, ctx->pkt.size);
        }
    }
    return 0;
}

void trans_nal2jpg(char* src_uri, char* dst_uri)
{
    int ret;
    Context* ctx = (Context* )malloc(sizeof(Context));
    memset(ctx, 0, sizeof(Context));
    sprintf(ctx->src_uri, "%s", src_uri);
    sprintf(ctx->dst_uri, "%s", dst_uri);
    sprintf(ctx->token, "%s", "biubiubiu");
    ctx->frame = av_frame_alloc();
    av_init_packet(&ctx->pkt);    
   
    FILE* fp = fopen(src_uri, "rb");
    fseek(fp, 0, SEEK_END);
    int file_len;
    file_len = ftell(fp); 
    fseek(fp, 0, SEEK_SET); 

    uint8_t* data_buffer = (uint8_t*)malloc(file_len);
    memset(data_buffer, 0, file_len);
    fread(data_buffer, 1, file_len, fp);
    fclose(fp);

    ctx->pkt.data = data_buffer;
    ctx->pkt.size = file_len;
    
    CreateH264DecoderContext(ctx);

    NAL2JPG(ctx);

    free(data_buffer);
    DestroyContext(ctx);
}

static int CreateJPGDecoderContext(Context* ctx)
{
	int ret = 0;
	AVDictionary *opts = NULL;
	AVCodecContext *context = NULL, *avctx = NULL;
	AVCodec *codec = NULL;
    av_register_all();

    codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);

	if (!codec){
		av_log(NULL, AV_LOG_ERROR, "video decoder AVCodec is not be fould, token = %s\n", ctx->token);
		goto err;
	}
	avctx = context = avcodec_alloc_context3(codec);
	if (!context){
		av_log(NULL, AV_LOG_ERROR, "video decoder AVCodecContext alloc no succss, token = %s\n", ctx->token);
		goto err;
	}
	avctx->workaround_bugs = 1;
	avctx->lowres = 0;
	if (avctx->lowres > codec->max_lowres) {
		av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d, token = %s\n", codec->max_lowres, ctx->token);
		avctx->lowres = codec->max_lowres;
	}
	avctx->idct_algo = 0;
	avctx->skip_frame = AVDISCARD_DEFAULT;
	avctx->skip_idct = AVDISCARD_DEFAULT;
	avctx->skip_loop_filter = AVDISCARD_DEFAULT;
	avctx->error_concealment = 3;
	if (avctx->lowres)
		avctx->flags |= CODEC_FLAG_EMU_EDGE;
	if (codec->capabilities & CODEC_CAP_DR1)
		avctx->flags |= CODEC_FLAG_EMU_EDGE;
	if (!av_dict_get(opts, "threads", NULL, 0))
		av_dict_set(&opts, "threads", "1", 0);
	if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
		av_dict_set(&opts, "refcounted_frames", "1", 0);

	context->codec_type = codec->type;
	context->codec_id = codec->id;

	context->flags |= CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(context, codec, &opts);
    if(ret < 0){
    	av_log(NULL, AV_LOG_ERROR, "video decoder avcodec open failed, token = %s\n", ctx->token);
    	goto err;
    }
    ctx->decoder = context;

	return 0;
err:
	if(context)
		avcodec_free_context(&context);
	av_log(NULL, AV_LOG_ERROR, "init video decoder failed! token = %s\n", ctx->token);
	return -1;
}

static int JPG2YUV(Context* ctx)
{
	int ret;
	int got_frame;
	int got_packet;
    ret = avcodec_decode_video2(ctx->decoder, ctx->frame, &got_frame, (const AVPacket*)&ctx->pkt);
	if (ret < 0 || !got_frame){
		av_log(NULL, AV_LOG_WARNING, "ADT Decode video failed! token = %s\n", ctx->token);
		return -1;
	}else if (got_frame){
        
        ctx->width = ctx->decoder->width;
		ctx->height = ctx->decoder->height;
		
		save_yuv(ctx->dst_uri, ctx->frame->data, ctx->frame->linesize, ctx->width, ctx->height);

        return 0;
	}
	return -1;
}

void trans_jpg2yuv(char* src_uri, char* dst_uri)
{
    int ret;
    Context* ctx = (Context* )malloc(sizeof(Context));
    memset(ctx, 0, sizeof(Context));
    sprintf(ctx->src_uri, "%s", src_uri);
    sprintf(ctx->dst_uri, "%s", dst_uri);
    sprintf(ctx->token, "%s", "biubiubiu");
    ctx->frame = av_frame_alloc();
    av_init_packet(&ctx->pkt);    
   
    FILE* fp = fopen(src_uri, "rb");
    fseek(fp, 0, SEEK_END);
    int file_len;
    file_len = ftell(fp); 
    fseek(fp, 0, SEEK_SET); 

    uint8_t* data_buffer = (uint8_t*)malloc(file_len);
    memset(data_buffer, 0, file_len);
    fread(data_buffer, 1, file_len, fp);
    fclose(fp);
    
    ctx->pkt.data = data_buffer;
    ctx->pkt.size = file_len;
    
    CreateJPGDecoderContext(ctx);
    JPG2YUV(ctx);
    
    free(data_buffer);
    DestroyContext(ctx);
}

static int CreateRtspContext(Context* ctx)
{
    int ret, ret1;
	AVCodec* dec;
	AVDictionary* options = NULL;
    av_register_all();
    avformat_network_init();

    ctx->video_stream_index = -1;
	ctx->fmt_ctx = avformat_alloc_context();

	if(ctx->fmt_ctx == NULL){
		av_log(NULL, AV_LOG_ERROR, "Cannot alloc failed for format context!\n");
		goto err;
	}

	av_dict_set(&options, "rtsp_transport", "tcp", 0);
	//av_dict_set(&options, "use_wallclock_as_timestamps", "1", 0);
	av_dict_set(&options, "stimeout", "5*1000*1000", 0);
	ctx->fmt_ctx->max_analyze_duration2 = 10 * 1000 * 1000;
	//av_log(ctx->token, AV_LOG_INFO, "input file  before ok\n");
	if((ret = avformat_open_input(&(ctx->fmt_ctx), ctx->src_uri, NULL, &options)) < 0){
		//ctx->callback(ctx->context, NULL, MPS_MESSAGE_CONNECT_FAILURE);
	    av_dict_free(&options);
		av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
		goto err;
	}
	else
		av_log(NULL, AV_LOG_INFO, "open input file ok\n");
	av_dict_free(&options);
	av_log(NULL, AV_LOG_INFO, "input file after ok\n");
	//ctx->callback(ctx->context, NULL, MPS_MESSAGE_CONNECT_SUCCESS);
	if((ret = avformat_find_stream_info(ctx->fmt_ctx, NULL)) < 0){
		avformat_close_input(&ctx->fmt_ctx);
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		goto err;
	} else
		av_log(NULL, AV_LOG_INFO, "find stream information ok\n");

	/* select the video stream */
	ret = av_find_best_stream(ctx->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	if(ret < 0){
		av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
	}
	else{
		av_log(NULL, AV_LOG_INFO, "find a video stream in the input file OK\n");
		ctx->video_stream_index = ret;
		ctx->video_stream = ctx->fmt_ctx->streams[ret];
        AVStream *stream;
        AVCodecContext *codec_ctx;
        stream = ctx->fmt_ctx->streams[ret];
        codec_ctx = stream->codec;
        if (!av_dict_get(options, "threads", NULL, 0))
            av_dict_set(&options, "threads", "1", 0);
        /* Open decoder */
        ret = avcodec_open2(codec_ctx, avcodec_find_decoder(codec_ctx->codec_id), &options);
        av_dict_free(&options);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for video stream\n");
        }
        ctx->decoder = codec_ctx;
	}

	/* select the audio stream */
	ret1 = av_find_best_stream(ctx->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
	if(ret1 < 0){
		av_log(NULL, AV_LOG_ERROR, "Cannot find a audio stream in the input file\n");
	}
	else{
		av_log(NULL, AV_LOG_INFO, "find a audio stream in the input file ok\n");
		ctx->audio_stream_index = ret1;
		ctx->audio_stream = ctx->fmt_ctx->streams[ret1];
	}

	av_log(NULL, AV_LOG_INFO, "init input rtsp ok\n");
	av_dump_format(ctx->fmt_ctx, 0, ctx->src_uri, 0);
	return 0;
err:
	if(ctx->fmt_ctx)
		avformat_free_context(ctx->fmt_ctx);
	av_log(NULL, AV_LOG_ERROR, "init stream failed\n");
	return -1;
}

static int save_rtspnal(uint8_t *nal, int size)
{
    FILE *fp;
    char filename[80];
    static int frame_num = 0;

    sprintf(filename, "nal_%d_%d.h264", (int)(getpid()), frame_num++);
    fp = fopen(filename, "wb");

    if (fp == NULL) {
        return -1;
    }

    fwrite(nal, sizeof(uint8_t), size, fp);

    fclose(fp);
    return 0;
}

static int RTSP2NAL(Context* ctx)
{
	AVPacket packet, *pkt = &packet;
	av_init_packet(&packet);
	int ret, got_frame;
	AVFrame *frame = NULL;
	int audio_pts;
	int video_pts;
    int num = 0;
    AVPacket pkt_tmp;
    do{
		ret = av_read_frame(ctx->fmt_ctx, pkt);
		if(ret == AVERROR(EAGAIN)){
			usleep(1000);
			continue;
		}
		if(ret < 0){
            av_free_packet(pkt);
			av_log(NULL, AV_LOG_ERROR, "Read frame time out!\n");
			break;
		}
		if((pkt->flags & AV_PKT_FLAG_KEY) == 1 && pkt->stream_index == ctx->video_stream_index){
            av_log(NULL, AV_LOG_INFO, "get a nal\n");
            save_rtspnal(pkt->data, pkt->size);
		}

        av_free_packet(pkt);
    }while(1);
}

void trans_rtsp2nal(char* src_uri)
{
    int ret;
    Context* ctx = (Context* )malloc(sizeof(Context));
    memset(ctx, 0, sizeof(Context));
    sprintf(ctx->src_uri, "%s", src_uri);
    sprintf(ctx->token, "%s", "biubiubiu");
   
    CreateRtspContext(ctx);
    RTSP2NAL(ctx);     
    DestroyContext(ctx);
}

static int save_rtspyuv(uint8_t *frame[], int linesize[], int width, int height)
{
    int ret = 0;
    FILE *fp;
    int y;
    
    char filename[80];
    static int frame_num = 0;

    sprintf(filename, "%d_%d.yuv", (int)(getpid()), frame_num++);
    fp = fopen(filename, "wb");

    if (fp == NULL) {
        return -1;
    }

    for (y = 0; y < height; y++) {
        fwrite(frame[0] + linesize[0] * y, sizeof(uint8_t), width, fp);
    }
    for (y = 0; y < height / 2; y++) {
        fwrite(frame[1] + linesize[1] * y, sizeof(uint8_t), width / 2, fp);
    }
    for (y = 0; y < height / 2; y++) {
        fwrite(frame[2] + linesize[2] * y, sizeof(uint8_t), width / 2, fp);
    }

    fclose(fp);
    return ret;
}

static int RTSP2YUV(Context* ctx)
{
	AVPacket packet, *pkt = &packet;
	av_init_packet(&packet);
	int ret, got_frame;
	AVFrame *frame = NULL;
	int audio_pts;
	int video_pts;
    int num = 0;
    AVPacket pkt_tmp;
    do{
		ret = av_read_frame(ctx->fmt_ctx, pkt);
		if(ret == AVERROR(EAGAIN)){
			usleep(1000);
			continue;
		}
		if(ret < 0){
            av_free_packet(pkt);
			av_log(NULL, AV_LOG_ERROR, "Read frame time out!\n");
			break;
		}
		if((pkt->flags & AV_PKT_FLAG_KEY) == 1 && pkt->stream_index == ctx->video_stream_index){
	        int got_packet;
            ret = avcodec_decode_video2(ctx->decoder, ctx->frame, &got_frame, (const AVPacket*)pkt);
	        if (ret < 0 || !got_frame){
	        	av_log(NULL, AV_LOG_WARNING, "ADT Decode video failed! token = %s\n", ctx->token);
	        	return -1;
	        }else if (got_frame){
                
                ctx->width = ctx->decoder->width;
	        	ctx->height = ctx->decoder->height;
	        	
	        	save_rtspyuv(ctx->frame->data, ctx->frame->linesize, ctx->width, ctx->height);
                av_log(NULL, AV_LOG_INFO, "get a yuv\n");

	        }
		}

        av_free_packet(pkt);
    }while(1);
}

void trans_rtsp2yuv(char* src_uri)
{
    int ret;
    Context* ctx = (Context* )malloc(sizeof(Context));
    memset(ctx, 0, sizeof(Context));
    sprintf(ctx->src_uri, "%s", src_uri);
    sprintf(ctx->token, "%s", "biubiubiu");
    ctx->frame = av_frame_alloc();
    av_init_packet(&ctx->pkt);    
   
    CreateRtspContext(ctx);
    RTSP2YUV(ctx);     
    DestroyContext(ctx);
}

static int save_rtspjpg(uint8_t *data, int size)
{
    FILE *fp;
    char filename[80];
    static int frame_num = 0;

    sprintf(filename, "%d_%d.jpg", (int)(getpid()), frame_num++);
    fp = fopen(filename, "wb");

    if (fp == NULL) {
        return -1;
    }

    fwrite(data, sizeof(uint8_t), size, fp);

    fclose(fp);
    return 0;
}

static int RTSP2JPG(Context* ctx)
{
	AVPacket packet, *pkt = &packet;
	av_init_packet(&packet);
	int ret, got_frame;
	AVFrame *frame = NULL;
	int audio_pts;
	int video_pts;
    int num = 0;
    AVPacket pkt_tmp;
    do{
		ret = av_read_frame(ctx->fmt_ctx, pkt);
		if(ret == AVERROR(EAGAIN)){
			usleep(1000);
			continue;
		}
		if(ret < 0){
            av_free_packet(pkt);
			av_log(NULL, AV_LOG_ERROR, "Read frame time out!\n");
			break;
		}
		if((pkt->flags & AV_PKT_FLAG_KEY) == 1 && pkt->stream_index == ctx->video_stream_index){
	        int got_packet;
            ret = avcodec_decode_video2(ctx->decoder, ctx->frame, &got_frame, (const AVPacket*)pkt);
	        if (ret < 0 || !got_frame){
	        	av_log(NULL, AV_LOG_WARNING, "ADT Decode video failed! token = %s\n", ctx->token);
	        	return -1;
	        }else if (got_frame){
                
                ctx->width = ctx->decoder->width;
	        	ctx->height = ctx->decoder->height;
                CreateJPGEncoderContext(ctx);
                int ret = 0;
                int got_packet = 0;
                av_free_packet(&ctx->pkt);
                ret = avcodec_encode_video2(ctx->jpg_encoder, &ctx->pkt, ctx->frame, &got_packet);

                if (ret < 0 || got_packet == 0){
                    av_log(NULL, AV_LOG_WARNING, "ADT Encoder video failed! token = %s\n", ctx->token);
                    return -1;
                }
                else if (got_packet){
                    save_rtspjpg(ctx->pkt.data, ctx->pkt.size);
                    av_log(NULL, AV_LOG_INFO, "get a jpg\n");
                }
	        }
		}

        av_free_packet(pkt);
    }while(1);
}

void trans_rtsp2jpg(char* src_uri)
{
    int ret;
    Context* ctx = (Context* )malloc(sizeof(Context));
    memset(ctx, 0, sizeof(Context));
    sprintf(ctx->src_uri, "%s", src_uri);
    sprintf(ctx->token, "%s", "biubiubiu");
    ctx->frame = av_frame_alloc();
    av_init_packet(&ctx->pkt);    
   
    CreateRtspContext(ctx);
    RTSP2JPG(ctx);     
    DestroyContext(ctx);
}
