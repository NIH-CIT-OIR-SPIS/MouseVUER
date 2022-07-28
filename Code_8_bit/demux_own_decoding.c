/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * Demuxing and decoding example.
 *
 * Show how to use the libavformat and libavcodec API to demux and
 * decode audio and video data.
 * @example demuxing_decoding.c
 */

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
// #include <libavfilter/avfilter.h>
// #include <libavfilter/buffersink.h>
// #include <libavfilter/buffersrc.h>
// #include <libavfilter/version.h>
// #include <libavutil/opt.h>
// #include <libavutil/pixdesc.h>
// #include <libavutil/pixfmt.h>
// ff_framesync

// static AVFormatContext *fmt_ctx = NULL;
// static AVCodecContext *video_dec_ctx = NULL;
// static int width, height;
// static enum AVPixelFormat pix_fmt;
// static AVStream *video_stream = NULL;//, *audio_stream = NULL;
// static const char *src_filename = NULL;
// static const char *video_dst_filename = NULL;
// static FILE *video_dst_file = NULL;

// static uint8_t *video_dst_data[4] = {NULL};
// static int      video_dst_linesize[4];
// static int video_dst_bufsize;

// static int video_stream_idx = -1;
// static AVFrame *frame = NULL;

// static AVPacket *pkt = NULL;
// static int video_frame_count = 0;

// static AVFormatContext *fmt_ctx_msb = NULL;
// static AVCodecContext *video_dec_ctx_msb = NULL;
// static int width_msb, height_msb;
// static enum AVPixelFormat pix_fmt_msb;
// static AVStream *video_stream_msb = NULL;//, *audio_stream = NULL;
// static const char *src_filename_msb = NULL;
// static const char *video_dst_filename_msb = NULL;
// static FILE *video_dst_file_msb = NULL;

// static uint8_t *video_dst_data_msb[4] = {NULL};
// static int      video_dst_linesize_msb[4];
// static int video_dst_bufsize_msb;

// static int video_stream_idx_msb = -1;
// static AVFrame *frame_msb = NULL;

// static AVPacket *pkt_msb = NULL;
// static int video_frame_count_msb = 0;

// static int output_video_frame(AVFrame *frame)
// {
//     if (frame->width != width || frame->height != height ||
//         frame->format != pix_fmt) {
//         fprintf(stderr, "Error: Width, height and pixel format have to be "
//                 "constant in a rawvideo file, but the width, height or "
//                 "pixel format of the input video changed:\n"
//                 "old: width = %d, height = %d, format = %s\n"
//                 "new: width = %d, height = %d, format = %s\n",
//                 width, height, av_get_pix_fmt_name(pix_fmt),
//                 frame->width, frame->height,
//                 av_get_pix_fmt_name(frame->format));
//         return -1;
//     }
//     if(video_frame_count == 0){
//         //gbrp is frame->format = 73
//         //yuv420p10le is frame->format = 64
//         printf("video_frame n:%d coded_n:%d, frame_format: %d\n",
//             video_frame_count++, frame->coded_picture_number, frame->format);

//         printf("Width, height and pixel format have to be "
//                 "constant in a rawvideo file, but the width, height or "
//                 "pixel format of the input video changed:\n"
//                 "old: width = %d, height = %d, format = %s\n"
//                 "new: width = %d, height = %d, format = %s\n",
//                 width, height, av_get_pix_fmt_name(pix_fmt),
//                 frame->width, frame->height,
//                 av_get_pix_fmt_name(frame->format));
//     }

//     av_image_copy(video_dst_data, video_dst_linesize,
//                   (const uint8_t **)(frame->data), frame->linesize,
//                   pix_fmt, width, height);

//
//     fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
//     return 0;
// }
#define STORE_FRM_NM 6
static int video_frame_count = 0;
static AVFrame *frame_lod = NULL;
static AVFrame *frame_arr_msb[STORE_FRM_NM] = {NULL};
static AVFrame *frame_arr_lsb[STORE_FRM_NM] = {NULL};

static void free_avframe_lst(AVFrame *frame_arr_lsb[STORE_FRM_NM], AVFrame *frame_arr_msb[STORE_FRM_NM])
{
    int i = 0;


    for (i = 0; i < STORE_FRM_NM; ++i)
    {
        if (frame_arr_lsb[i] != NULL)
        {
            av_frame_free(&frame_arr_lsb[i]);
            //frame_arr_lsb[i] = NULL;
        }
        if (frame_arr_msb[i] != NULL)
        {
            av_frame_free(&frame_arr_msb[i]);
            //frame_arr_msb[i] = NULL;
        }
    }
}

static void unref_avframe_lst(AVFrame *frame_arr_lsb[STORE_FRM_NM], AVFrame *frame_arr_msb[STORE_FRM_NM])
{
    int i;
    for (i = 0; i < STORE_FRM_NM; ++i)
    {
        if (frame_arr_lsb[i] != NULL)
        {
            av_frame_unref(frame_arr_lsb[i]);
        }
        if (frame_arr_msb[i] != NULL)
        {
            av_frame_unref(frame_arr_msb[i]);
        }
    }
}

static int video_combine_frames(AVFrame *frame, AVFrame *frame_msb, const char *dst_filename)
{
    uint8_t *p0, *p = 0;
    int x = 0, y = 0;
    if (frame->width != 1280 || frame->height != 720 ||
        frame->format != 64 /*yuv420p10le*/ || frame_msb->width != 1280 || frame_msb->height != 720 || frame_msb->format != 73 /*gbrp*/)
    {
        fprintf(stderr, "Error: Width, height and pixel format have to be "
                        "constant in a rawvideo file, but the width, height or "
                        "pixel format of the input video changed:\n"
                        "old: width = %d, height = %d, format = %s\n"
                        "new: width = %d, height = %d, format = %s\n",
                1280, 720, av_get_pix_fmt_name(64 /*yuv420p10le*/),
                frame->width, frame->height,
                av_get_pix_fmt_name(frame->format));
        return -1;
    }
    // if(frame->width != frame_msb->width || frame_msb->height != frame->height){
    //     printf("frame->width != frame_msb->width || frame_msb->height != frame->height\n");
    //     return -1;
    // }
    // if(frame->coded_picture_number != frame_msb->coded_picture_number && frame_msb->coded_picture_number < 5){
    // printf( "msb: width = %d, height = %d, format = %s coded_n:%d\n"
    //         "lsb: width = %d, height = %d, format = %s coded_n:%d\n"
    //         "video_frame_count = %d\n",
    //         frame_msb->width, frame_msb->height, av_get_pix_fmt_name(frame_msb->format), frame_msb->coded_picture_number,
    //         frame->width, frame->height, av_get_pix_fmt_name(frame->format),frame->coded_picture_number , video_frame_count++);
    // }

    x = (frame_msb->coded_picture_number - frame->coded_picture_number);
    if (video_frame_count >= 890)
    {
        printf("format = %s, coded_n: %d\n"
               "format = %s, coded_n: %d\n"
               "coded_n_diff:%d\n",
               av_get_pix_fmt_name(frame_msb->format), frame_msb->coded_picture_number,
               av_get_pix_fmt_name(frame->format), frame->coded_picture_number, x);

        // printf( "msb: width = %d, height = %d, format = %s coded_n:%d\n"
        //         "lsb: width = %d, height = %d, format = %s coded_n:%d\n"
        //         "video_frame_count = %d\n",
        //         frame_msb->width, frame_msb->height, av_get_pix_fmt_name(frame_msb->format), frame_msb->coded_picture_number,
        //         frame->width, frame->height, av_get_pix_fmt_name(frame->format),frame->coded_picture_number , video_frame_count++);
    }
    video_frame_count++;
    // p0 = frame->data[0];
    // //a0 = frame_msb->data[0];
    // for(y = 0; y < frame->height; ++y){
    //     for(x = 0; x < frame->width; ++x){

    //     }
    // }
    //++;

    // printf("video_frame n:%d coded_n:%d\n",
    //     video_frame_count++, frame->coded_picture_number);
    return 0;
}
static int check_frame_arr_test(AVFrame *frame_arr_lsb[STORE_FRM_NM], AVFrame *frame_arr_msb[STORE_FRM_NM]){

    int i = 0;
    int c = 0;
    for (i = 0; i < STORE_FRM_NM; ++i)
    {
        if (frame_arr_lsb[i] != NULL)
        {
            c = frame_arr_lsb[i]->coded_picture_number - frame_arr_msb[i]->coded_picture_number;
            printf("frame_arr_lsb, coded_n: %d, diff lsb - msb = %d\n", frame_arr_lsb[i]->coded_picture_number, c);
        }
        if (frame_arr_msb[i] != NULL)
        {
            c = frame_arr_msb[i]->coded_picture_number - frame_arr_lsb[i]->coded_picture_number;
            printf("frame_arr_msb, coded_n: %d, diff msb - lsb = %d\n", frame_arr_msb[i]->coded_picture_number, c);
        }
        c = 0;
    }
    return 0;

}
static int decode_packet(AVCodecContext *dec, AVCodecContext *dec_msb,
                         AVPacket *pkt, AVPacket *pkt_msb, AVFrame *frame, AVFrame *frame_msb, const char *filename)
{
    int ret = 0;
    int ret_msb = 0;
    int no_output_frm = 0;
    int no_output_frm_msb = 0;
    int i = 0, j = 0;
    int x = 0, y = 0;
    


    while(ret >= 0 || ret_msb >= 0){
        ret = avcodec_send_packet(dec, pkt);
        
        if (ret < 0)
        {
            fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
            return ret;
        }
        // get up to 10 max frames from the decoder
        for (i = 0; i < STORE_FRM_NM && ret >= 0; ++i)
        {
            ret = avcodec_receive_frame(dec, frame);
            if (ret < 0)
            {
                // those two return values are special and mean there is no output
                // frame available, but there were no errors during decoding
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                {
                    no_output_frm = 1;
                    break;
                }
                fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
                return ret;
            }

            // append frame and dec to the list of frames
            frame_arr_lsb[i] = frame;
        }
        ret_msb = avcodec_send_packet(dec_msb, pkt_msb);
        if (ret_msb < 0)
        {
            fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret_msb));
            return ret_msb;
        }
        
        for (i = 0; i < STORE_FRM_NM && ret_msb >= 0; ++i)
        {
            ret_msb = avcodec_receive_frame(dec_msb, frame_msb);
            if (ret_msb < 0)
            {
                if (ret_msb == AVERROR_EOF || ret_msb == AVERROR(EAGAIN))
                {
                    no_output_frm_msb = 1;
                    break;
                }
                fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret_msb));
                return ret_msb;
            }

            frame_arr_msb[i] = frame_msb;
        }

        if (no_output_frm && no_output_frm_msb)
        {
            return 0;
        }
        // video_combine_frames called here
        ret = check_frame_arr_test(frame_arr_lsb, frame_arr_msb);
        unref_avframe_lst(frame_arr_lsb, frame_arr_msb);
        if (ret < 0)
        {
            return ret;
        }
        i = 0;
        no_output_frm = 0;
        no_output_frm_msb = 0;
        ret = 0;
        ret_msb = 0;
    }

    return 0;
}
/*
static int decode_packet(AVCodecContext *dec, AVCodecContext *dec_msb,
                         AVPacket *pkt, AVPacket *pkt_msb, AVFrame *frame, AVFrame *frame_msb, const char *filename)
{
    int ret = 0;
    int ret_msb = 0;
    // submit the packet to the decoder
    //ret = avcodec_send
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0){
        fprintf(stderr, "Error submitting the packet to the decoder(%s)\n", av_err2str(ret));
        return ret;

    }

    ret_msb = avcodec_send_packet(dec_msb, pkt_msb);
    if (ret_msb < 0)
    {
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret_msb));
        return ret_msb;
    }

    // get all the available frames from the decoder
    int get_msb_frame_early = 0;
    int get_lsb_frame_early = 0;

    while (ret >= 0 || ret_msb >= 0)
    {

        ret = avcodec_receive_frame(dec, frame);
        // if(ret < 0 ){
        //     if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
        //     {

        //         return 0;
        //     }
        //     fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
        //     return ret;
        // }

        ret_msb = avcodec_receive_frame(dec_msb, frame_msb);
        // if(ret_msb < 0){
        //     if (ret_msb == AVERROR_EOF || ret_msb == AVERROR(EAGAIN))
        //     {
        //         return 0;
        //     }
        //     fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret_msb));
        //     return ret_msb;
        // }
        if (ret < 0 || ret_msb < 0)
        {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN) || ret_msb == AVERROR_EOF || ret_msb == AVERROR(EAGAIN))
            {

                return 0;
            }


            if( ret < 0){
                fprintf(stderr, "Error during decoding (%s) (%s)\n", av_err2str(ret),  av_err2str(ret_msb));
                return ret;

            }
            if(ret_msb < 0){
                fprintf(stderr, "Error during decoding (%s) (%s)\n", av_err2str(ret),  av_err2str(ret_msb));
                return ret_msb;
            }
            fprintf(stderr, "Error during decoding (%s) (%s)\n", av_err2str(ret),  av_err2str(ret_msb));
            return ret;
        }

        // write the frame data to output file
        // if (dec->codec->type == AVMEDIA_TYPE_VIDEO && dec_msb->codec->type  == AVMEDIA_TYPE_VIDEO)
        // {
        //if(frame->coded_picture_number != frame_msb->coded_picture_number){

        ret = video_combine_frames(frame, frame_msb, filename);
        //}
        // }
        // else
        // {
        //     return -1;
        // }

        // else
        //     ret = output_audio_frame(frame);

        //av_frame_unref(frame);
        //av_frame_unref(frame_msb);
        if (ret < 0 && ret_msb < 0)
            return ret;
    }

    return 0;
}*/

static int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type, const char *src_filename)
{
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0)
    {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), src_filename);
        return ret;
    }
    else
    {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec)
        {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx)
        {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0)
        {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        /* Init the decoders */
        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0)
        {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int ret_msb = 0;
    int width, height, width_msb, height_msb;
    int pix_fmt, pix_fmt_msb;
    uint8_t *video_dst_data[4] = {NULL};
    int video_dst_linesize[4];
    int video_dst_bufsize;
    int video_stream_idx = -1;
    uint8_t *video_dst_data_msb[4] = {NULL};
    int video_dst_linesize_msb[4];
    int video_dst_bufsize_msb;
    int video_stream_idx_msb = -1;

    const char *video_dst_filename, *src_filename, *src_filename_msb;
    AVFormatContext *fmt_ctx = NULL;
    AVFormatContext *fmt_ctx_msb = NULL;
    FILE *video_dst_file = NULL;
    AVStream *video_stream = NULL;
    AVStream *video_stream_msb = NULL;
    AVCodecContext *video_dec_ctx = NULL;
    AVCodecContext *video_dec_ctx_msb = NULL;
    AVFrame *frame = NULL;
    AVFrame *frame_msb = NULL;
    AVPacket *pkt = NULL;
    AVPacket *pkt_msb = NULL;
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s  input_file input_file_msb video_output_file\n"
                        "API example program to show how to read frames from an input file.\n"
                        "This program reads frames from a file, decodes them, and writes decoded\n"
                        "video frames to a rawvideo file named video_output_file, and decoded\n",
                argv[0]);
        exit(1);
    }
    src_filename = argv[1];
    src_filename_msb = argv[2];
    video_dst_filename = argv[3];
    // audio_dst_filename = argv[3];

    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0)
    {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }
    // For msb
    if (avformat_open_input(&fmt_ctx_msb, src_filename_msb, NULL, NULL) < 0)
    {
        fprintf(stderr, "Could not open source file %s\n", src_filename_msb);
        exit(1);
    }
    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }
    // For msb
    if (avformat_find_stream_info(fmt_ctx_msb, NULL) < 0)
    {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO, src_filename) >= 0)
    {
        video_stream = fmt_ctx->streams[video_stream_idx];

        video_dst_file = fopen(video_dst_filename, "wb");
        if (!video_dst_file)
        {
            fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
            ret = 1;
            goto end;
        }

        /* allocate image where the decoded image will be put */
        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
        ret = av_image_alloc(video_dst_data, video_dst_linesize,
                             width, height, pix_fmt, 1);
        if (ret < 0)
        {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            goto end;
        }
        video_dst_bufsize = ret;
    }

    if (open_codec_context(&video_stream_idx_msb, &video_dec_ctx_msb, fmt_ctx_msb, AVMEDIA_TYPE_VIDEO, src_filename_msb) >= 0)
    {
        video_stream_msb = fmt_ctx_msb->streams[video_stream_idx_msb];

        /* allocate image where the decoded image will be put */
        width_msb = video_dec_ctx_msb->width;
        height_msb = video_dec_ctx_msb->height;
        pix_fmt_msb = video_dec_ctx_msb->pix_fmt;
        ret_msb = av_image_alloc(video_dst_data_msb, video_dst_linesize_msb,
                                 width_msb, height_msb, pix_fmt_msb, 1);
        if (ret_msb < 0)
        {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            goto end;
        }
        video_dst_bufsize_msb = ret_msb;
    }
    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);
    av_dump_format(fmt_ctx_msb, 0, src_filename_msb, 0);
    if (!video_stream)
    {
        fprintf(stderr, "Could not find video stream in the input, aborting\n");
        ret = 1;

        goto end;
    }
    if (!video_stream_msb)
    {
        fprintf(stderr, "Could not find video stream in the input, aborting\n");
        ret_msb = 1;
        goto end;
    }

    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    frame_msb = av_frame_alloc();
    if (!frame_msb)
    {
        fprintf(stderr, "Could not allocate frame\n");
        ret_msb = AVERROR(ENOMEM);
        goto end;
    }

    pkt = av_packet_alloc();
    if (!pkt)
    {
        fprintf(stderr, "Could not allocate packet\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    pkt_msb = av_packet_alloc();
    if (!pkt_msb)
    {
        fprintf(stderr, "Could not allocate packet\n");
        ret_msb = AVERROR(ENOMEM);
        goto end;
    }

    if (video_stream)
    {
        printf("Demuxing video from file '%s' into '%s'\n", src_filename, video_dst_filename);
    }
    if (video_stream_msb)
    {
        printf("Demuxing video from file '%s' into '%s'\n", src_filename_msb, video_dst_filename);
    }
    /*
    if (audio_stream)
        printf("Demuxing audio from file '%s' into '%s'\n", src_filename, audio_dst_filename);
    */
    /* read frames from the file */
    
    while (av_read_frame(fmt_ctx, pkt) >= 0 && av_read_frame(fmt_ctx_msb, pkt_msb) >= 0)
    {
        // check if the packet belongs to a stream we are interested in, otherwise
        // skip it
        if (pkt->stream_index == video_stream_idx && pkt_msb->stream_index == video_stream_idx_msb)
        {
            ret = decode_packet(video_dec_ctx, video_dec_ctx_msb, pkt, pkt_msb, frame, frame_msb, video_dst_filename);
        }
        av_packet_unref(pkt);
        av_packet_unref(pkt_msb);
        if (ret < 0)
            break;
    }
    /* flush the decoders */
    if (video_dec_ctx && video_dec_ctx_msb)
    {
        decode_packet(video_dec_ctx, video_dec_ctx_msb, NULL, NULL, frame, frame_msb, video_dst_filename);
    }
    printf("Demuxing succeeded.\n");
    printf("HIer\n");
end:
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&video_dec_ctx_msb);
    // avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    avformat_close_input(&fmt_ctx_msb);
    if (video_dst_file)
    {
        fclose(video_dst_file);
    }
    av_packet_free(&pkt);
    av_packet_free(&pkt_msb);
    av_frame_free(&frame);
    av_frame_free(&frame_msb);
    free_avframe_lst(frame_arr_lsb, frame_arr_msb);
    av_free(video_dst_data[0]);
    av_free(video_dst_data_msb[0]);

    return ret < 0;
}
