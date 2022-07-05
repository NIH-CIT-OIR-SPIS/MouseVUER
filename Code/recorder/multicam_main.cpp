/**
 * @file multicam_main.cpp
 * @author Noah Cubert (asci_anything@protonmail.com)
 * @brief A cross OS command line tool that uses FFmpeg and Intel(R) RealSense(TM) cross platform API.
 * This program compresses in real time 16 bit grayscale depth images.
 * Average compression ratio seen is 33x with an average PSNR of 74 dB
 * Complete compressed files range from 50 MB - 200 MB per minute
 *
 * Tested on FFmpeg 5.0 "Lorentz" Released 2022
 * Tested on Intel(R) Realsense(TM) software version 2.50.0.
 * Tested on Surface Pro 4, 16 GB DDR4 RAM, with an Intel(R) Core(TM) i7-6650U CPU @ 2.20GHz
 * CPU Load around 50% when recording just depth video, higher if other settings are on like raw recording, previewing, etc.
 *
 * @version 0.1
 * @date 2022-06-30
 * @copyright Copyright (c) 2022
 */

// TODO: Add networking code for operating on a network drive or on storage
// TODO: Download Intel(R) helper API for a GUI like system that the user can use with OpenGL. File is example.hpp
// TODO: Add support for multiple cameras if you have time.

#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#endif

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/rs_advanced_mode.hpp>

#if __has_include(<example.hpp>)
#include <example.hpp>
#endif

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/detail/cmdline.hpp>

// #include "libswscale/swscale.h"

// #include <libavcodec/avcodec.h>
// #include <libavformat/avformat.h>
// #include <libswscale/swscale.h>
// #include <libavutil/opt.h>
// #include <libavutil/pixdesc.h>
// #include <libavutil/pixfmt.h>
// #include <libavutil/imgutils.h>
// #include <libavutil/error.h>
// #include <libavutil/frame.h>

#include <map>
#include <vector>
#include <thread>
#include <queue>
#include <atomic>
#include <memory>
#include <mutex>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <chrono>
#include <iterator>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <iostream>
extern "C"
{
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
//#define STREAM_DURATION 10.0
#define STREAM_FRAME_RATE 30                  /* 25 images/s */
#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P10LE /* default pix_fmt */
static int shift_num = 63;
#define SCALE_FLAGS SWS_BICUBIC
    typedef struct OutputStream
    {
        AVStream *st;
        AVCodecContext *enc;

        /* pts of the next frame that will be generated */
        int64_t next_pts;
        int samples_count;

        AVFrame *frame;
        AVFrame *tmp_frame;

        AVPacket *tmp_pkt;

        float t, tincr, tincr2;

        struct SwsContext *sws_ctx;
        struct SwrContext *swr_ctx;
    } OutputStream;
    // static AVFrame* RGBtoYUV(AVFrame *frame, AVCodecContext *c){
    //     int ret;
    //     AVFrame *frameYUV=av_frame_alloc();
    //     assert(frameYUV);
    //     frameYUV->format = c->pix_fmt;
    //     frameYUV->width  = c->width;
    //     frameYUV->height = c->height;

    //     int numBytes=avpicture_get_size(AV_PIX_FMT_YUV420P, c->width, c->height);
    //     assert(numBytes);
    //     uint8_t *dataBuffer = (uint8_t*) av_malloc (numBytes*sizeof(uint8_t));

    //     frameYUV->data[0]=dataBuffer;

    //     avpicture_fill((AVPicture *)frameYUV, dataBuffer, AV_PIX_FMT_YUV420P, c->width, c->height);

    //     struct SwsContext * YUVScaleCtx = sws_getContext
    //     (
    //         c->width,
    //         c->height,
    //         AV_PIX_FMT_RGB24,
    //         c->width,
    //         c->height,
    //         AV_PIX_FMT_YUV420P,
    //         SWS_BILINEAR,
    //         0,
    //         0,
    //         0
    //     );
    //     ret = sws_scale(
    //         YUVScaleCtx,
    //         (const uint8_t *const *)frame->data,
    //         frame->linesize, //stride is the size of a line + potential padding for performance issue
    //         0,
    //         c->height,
    //         frameYUV->data,
    //         frameYUV->linesize
    //     );
    //     assert(ret);
    //     return frameYUV;
    // }
    static int encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                      FILE *outfile)
    {
        int ret = 0;

        /* send the frame to the encoder */
        if (frame)
            printf("Send frame %3" PRId64 " \n", frame->pts);

        ret = avcodec_send_frame(enc_ctx, frame);
        

        if (ret < 0)
        {
            fprintf(stderr, "Error sending a frame for encoding\n");
            exit(1);
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0)
            {
                fprintf(stderr, "Error during encoding\n");
                exit(1);
            }

            // printf("Write packet %3" PRId64 " (size=%5d)\n", pkt->pts, pkt->size);
            fwrite(pkt->data, 1, pkt->size, outfile);
            av_packet_unref(pkt);
        }
        return ret == AVERROR_EOF ? 0 : 1; // return 1 on success
    }
    static void fill_lsb_image(AVFrame *pict, int width, int height, uint8_t *src_img){
        int i = 0, j = 0, k = 0;
        // uint8_t *dst_img = pict->data[0];
        // for(i = 0; i < height; i++){
        //     for(j = 0; j < width; j++){
        //         for(k = 0; k < 3; k++){
        //             dst_img[i*pict->linesize[0] + j*3 + k] = src_img[i*width*3 + j*3 + k];
        //         }
        //     }
        // }
        //pict->data[0] = src_img;
        int numBytes = height*width*2;
        for(i = 0; i < numBytes; i+=2){
            src_img[i + 1] &= 0x3F;
        }
        pict->data[0] = src_img;
        
        
        //memccpy(pict->data[0], src_img, sizeof(uint8_t), numBytes);

    }
    static AVFrame *get_frame(AVCodecContext *enc_ctx, AVFrame *frame, uint8_t *src_img)
    {
        int ret = 0;

        if (av_frame_make_writable(frame) < 0)
        {
            fprintf(stderr, "Could not make frame writable\n");
            exit(1);
        }
        fill_lsb_image(frame, enc_ctx->width, enc_ctx->height, src_img);

        return frame;
    }

    static int write_frame_own(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                               FILE *outfile, uint8_t *src_img,  int flush)
    {
        fflush(stdout);
        int ret = 0;

        if (flush == 1)
        {
            ret = encode(enc_ctx, NULL, pkt, outfile);
        }
        else
        {
            frame->pts = enc_ctx->frame_number;
            frame->display_picture_number = enc_ctx->frame_number;
            ret = encode(enc_ctx, get_frame(enc_ctx, frame, src_img), pkt, outfile);
        }

        return ret;
    }

    // static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
    // {
    //     AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    //     printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
    //            av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
    //            av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
    //            av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
    //            pkt->stream_index);
    // }

    static int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                           AVStream *st, AVFrame *frame, AVPacket *pkt)
    {
        int ret;
    
        // send the frame to the encoder
        ret = avcodec_send_frame(c, frame);
        if (ret < 0)
        {
            fprintf(stderr, "Error sending a frame to the encoder: \n");
            exit(1);
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_packet(c, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0)
            {
                fprintf(stderr, "Error encoding a frame: \n");
                exit(1);
            }

            /* rescale output packet timestamp values from codec to stream timebase */
            av_packet_rescale_ts(pkt, c->time_base, st->time_base);
            pkt->stream_index = st->index;

            /* Write the compressed frame to the media file. */
            // log_packet(fmt_ctx, pkt);
            ret = av_interleaved_write_frame(fmt_ctx, pkt);
            /* pkt is now blank (av_interleaved_write_frame() takes ownership of
             * its contents and resets pkt), so that no unreferencing is necessary.
             * This would be different if one used av_write_frame(). */
            if (ret < 0)
            {
                fprintf(stderr, "Error while writing output packet:\n");
                exit(1);
            }
        }

        return ret == AVERROR_EOF ? 1 : 0;
    }
    /* Add an output stream. */
    static void add_stream(OutputStream *ost, AVFormatContext *oc,
                           const AVCodec **codec,
                           enum AVCodecID codec_id)
    {
        AVCodecContext *c = NULL;
        int i;

        /* find the encoder */
        *codec = avcodec_find_encoder(codec_id);
        if (!(*codec))
        {
            fprintf(stderr, "Could not find encoder for '%s'\n",
                    avcodec_get_name(codec_id));
            exit(1);
        }

        ost->tmp_pkt = av_packet_alloc();
        if (!ost->tmp_pkt)
        {
            fprintf(stderr, "Could not allocate AVPacket\n");
            exit(1);
        }

        ost->st = avformat_new_stream(oc, NULL);
        if (!ost->st)
        {
            fprintf(stderr, "Could not allocate stream\n");
            exit(1);
        }
        ost->st->id = oc->nb_streams - 1;
        c = avcodec_alloc_context3(*codec);
        if (!c)
        {
            fprintf(stderr, "Could not alloc an encoding context\n");
            exit(1);
        }
        ost->enc = c;
        printf("hih\n");
        c->codec_id = codec_id;

        c->bit_rate = 8 * 2000000;
        /* Resolution must be a multiple of two. */
        c->width = 1280;
        c->height = 720;
        c->thread_count = 4;

        // av_opt_set(c->priv_data, "crf", "24", 0);
        //  av_opt_set(c->priv_data, "
        //  c->active_thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        // ost->st->r_frame_rate =
        ost->st->time_base = (AVRational){1, STREAM_FRAME_RATE};
        // ost->st->nb_frames = 450;
        c->time_base = ost->st->time_base;
        c->framerate = (AVRational){STREAM_FRAME_RATE, 1};
        // c->rc_min_rate
        // c->gop_size = 5; /* emit one intra frame every twelve frames at most */
        // c->max_b_frames = 3;
        c->pix_fmt = AV_PIX_FMT_YUV420P10LE;
        av_opt_set(c->priv_data, "preset", "veryfast", 0);
        av_opt_set(c->priv_data, "tune", "psnr", 0);
        av_opt_set(c->priv_data, "crf", "20", 0);
        // printf("Bits per raw sample %d\n", c->bits_per_raw_sample);
        //  switch ((*codec)->type)
        //  {
        //  case AVMEDIA_TYPE_AUDIO:
        //      // c->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        //      // c->bit_rate = 64000;
        //      // c->sample_rate = 44100;
        //      // if ((*codec)->supported_samplerates)
        //      // {
        //      //     c->sample_rate = (*codec)->supported_samplerates[0];
        //      //     for (i = 0; (*codec)->supported_samplerates[i]; i++)
        //      //     {
        //      //         if ((*codec)->supported_samplerates[i] == 44100)
        //      //             c->sample_rate = 44100;
        //      //     }
        //      // }
        //      // av_channel_layout_copy(&c->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);
        //      // ost->st->time_base = (AVRational){1, c->sample_rate};
        //      // break;

        // case AVMEDIA_TYPE_VIDEO:

        //     if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
        //     {
        //         printf("Warning using AV_CODEC_ID_MPEG2VIDEO!\n");
        //         /* just for testing, we also add B-frames */
        //         c->max_b_frames = 2;
        //     }
        //     if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
        //     {
        //         printf("Warning using AV_CODEC_ID_MPEG2VIDEO!\n");
        //         /* Needed to avoid using macroblocks in which some coeffs overflow.
        //          * This does not happen with normal video, it just happens here as
        //          * the motion of the chroma plane does not match the luma plane. */
        //         c->mb_decision = 2;
        //     }

        //     break;

        // default:
        //     break;
        // }

        /* Some formats want stream headers to be separate. */
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
    {
        AVFrame *picture;
        int ret;

        picture = av_frame_alloc();
        if (!picture)
        {
            fprintf(stderr, "Could not allocate picture\n");
            return NULL;
        }
        picture->format = pix_fmt;
        picture->width = width;
        picture->height = height;

        /* allocate the buffers for the frame data */
        ret = av_frame_get_buffer(picture, 0);
        if (ret < 0)
        {
            fprintf(stderr, "Could not allocate frame data.\n");
            exit(1);
        }

        return picture;
    }

    static void open_video(AVFormatContext *oc, const AVCodec *codec,
                           OutputStream *ost, AVDictionary *opt_arg)
    {
        int ret;
        AVCodecContext *c = ost->enc;
        AVDictionary *opt = NULL;

        av_dict_copy(&opt, opt_arg, 0);

        /* open the codec */
        ret = avcodec_open2(c, codec, &opt);
        av_dict_free(&opt);
        if (ret < 0)
        {
            fprintf(stderr, "Could not open video codec:\n");
            exit(1);
        }

        /* allocate and init a re-usable frame */
        ost->frame = alloc_picture(AV_PIX_FMT_YUV420P10LE, c->width, c->height);

        if (!ost->frame)
        {
            fprintf(stderr, "Could not allocate video frame\n");
            exit(1);
        }

        /* If the output format is not YUV420P, then a temporary YUV420P
         * picture is needed too. It is then converted to the required
         * output format. */
        ost->tmp_frame = NULL;
        // if (c->pix_fmt != AV_PIX_FMT_YUV420P)
        // {
        //     ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        //     if (!ost->tmp_frame)
        //     {
        //         fprintf(stderr, "Could not allocate temporary picture\n");
        //         exit(1);
        //     }
        // }

        /* copy the stream parameters to the muxer */
        ret = avcodec_parameters_from_context(ost->st->codecpar, c);
        if (ret < 0)
        {
            fprintf(stderr, "Could not copy the stream parameters\n");
            exit(1);
        }
    }
    // avpicture_fill(pic_raw, (uint8_t*)pixelBuffer, PIX_FMT_RGB24, width, height);
    // AVFrame * frame;

    // AVPicture destinyPictureYUV;

    // avpicture_alloc(&destinyPictureYUV, codecContext->pix_fmt, newCodecContext->width, newCodecContext->height);

    // THIS is what you want probably
    //*reinterpret_cast<AVPicture *>(pict) = destinyPictureYUV;
    // sws_cosws_getContext(width, height, AV_PIX_FMT_GRAY16LE, width, height, AV_PIX_FMT_YUV420P10LE, SWS_BICUBIC, NULL, NULL, NULL);
    // sws_scale(sws_ctx, (const uint8_t * const *)data, width, 0, height, pict->data, width);
    // sws_getCachedContext(sws_ctx, width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    // pict->data[0] = data;
    // pict->data[1] = data;
    // pict->data[2] = data;
    // pict->format
    /* Y */
    // sws_getContext(width, height, AV_PIX_FMT_YUV420P10LE, width, height, AV_PIX_FMT_YUV420P10LE, SWS_BICUBIC, NULL, NULL, NULL);

    /* Prepare a dummy image. */
    static void fill_yuv_image(AVFrame *pict, int frame_index,
                               int width, int height, uint8_t *data, uint8_t *color_data, int stride)
    {
        int x = 0, y = 0, i = 0;
        int cd = 0;
        i = frame_index;
        // struct SwsContext* convertCtx = sws_getContext(width, height, AV_PIX_FMT_YUV420P10LE, width, height, AV_PIX_FMT_YUV420P10LE, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        // avpicture_fill((AVPicture *)pict, (uint8_t*)data, AV_PIX_FMT_YUV420P10LE, width, height);
        pict->linesize[0] = pict->linesize[1] = stride;
        pict->data[0] = data;
        pict->data[1] = color_data;
        // pict->data[0] = data;
        // pict->data[1] = data;
        // pict->data[2] = data;

        // for (y = 0; y < height; y++)
        // {
        //     for (x = 0; x < width; x++)
        //     {
        //         pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

        //     }
        // }

        // /* Cb and Cr */
        // for (y = 0; y < height / 2; y++)
        // {
        //     for (x = 0; x < width / 2; x++)
        //     {
        //         ++cd;
        //         pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
        //         pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        //     }
        // }
    }

    static AVFrame *get_video_frame(OutputStream *ost, long time_run, uint8_t *data, uint8_t *color_data, int stride)
    {
        AVCodecContext *c = ost->enc; // 64 + x + i * 5

        /* check if we want to generate more frames */
        if (av_compare_ts(ost->next_pts, c->time_base, (int64_t)time_run, (AVRational){1, 1}) > 0)
        {
            return NULL;
        }
        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally; make sure we do not overwrite it here */
        if (av_frame_make_writable(ost->frame) < 0)
        {
            exit(1);
        }
        // printf("HIeeeee\n");
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height, data, color_data, stride);

        ost->frame->pts = ost->next_pts++;

        return ost->frame;
    }

    /*
     * encode one video frame and send it to the muxer
     * return 1 when encoding is finished, 0 otherwise
     */
    static int write_video_frame(AVFormatContext *oc, OutputStream *ost, uint8_t *data, long time_run, uint8_t *color_data, int stride)
    {
        return write_frame(oc, ost->enc, ost->st, get_video_frame(ost, time_run, data, color_data, stride), ost->tmp_pkt);
    }

    static void close_stream(AVFormatContext *oc, OutputStream *ost)
    {
        avcodec_free_context(&ost->enc);
        av_frame_free(&ost->frame);
        av_frame_free(&ost->tmp_frame);
        av_packet_free(&ost->tmp_pkt);
        // sws_freeContext(ost->sws_ctx);
        swr_free(&ost->swr_ctx);
    }

    // int record(const char *filename)
    // {
    //     OutputStream video_st = {0}, audio_st = {0};
    //     const AVOutputFormat *fmt;
    //     AVFormatContext *oc;
    //     const AVCodec *video_codec;
    //     int ret = 0;
    //     int have_video = 0, have_audio = 0;
    //     int encode_video = 0, encode_audio = 0;
    //     AVDictionary *opt = NULL;
    //     int i = 0;

    //     // if (argc < 2)
    //     // {
    //     //     printf("usage: %s output_file\n"
    //     //            "API example program to output a media file with libavformat.\n"
    //     //            "This program generates a synthetic audio and video stream, encodes and\n"
    //     //            "muxes them into a file named output_file.\n"
    //     //            "The output format is automatically guessed according to the file extension.\n"
    //     //            "Raw images can also be output by using '%%d' in the filename.\n"
    //     //            "\n",
    //     //            argv[0]);
    //     //     return 1;
    //     // }

    // filename = argv[1];
    //  for (i = 2; i + 1 < argc; i += 2)
    //  {
    //      if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
    //          av_dict_set(&opt, argv[i] + 1, argv[i + 1], 0);
    //  }

    //     /* allocate the output media context */
    //     avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    //     if (!oc)
    //     {
    //         printf("Could not deduce output format from file extension: using MPEG.\n");
    //         avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    //     }
    //     if (!oc)
    //         return 1;

    //     fmt = oc->oformat;
    //     //AV_CODEC_ID_H264;

    //     /* Add the audio and video streams using the default format codecs
    //      * and initialize the codecs. */
    //     if (fmt->video_codec != AV_CODEC_ID_NONE)
    //     {
    //         add_stream(&video_st, oc, &video_codec, AV_CODEC_ID_H264);
    //         have_video = 1;
    //         encode_video = 1;
    //     }
    //     // if (fmt->audio_codec != AV_CODEC_ID_NONE)
    //     // {
    //     //     add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
    //     //     have_audio = 1;
    //     //     encode_audio = 1;
    //     // }

    //     /* Now that all the parameters are set, we can open the audio and
    //      * video codecs and allocate the necessary encode buffers. */
    //     if (have_video)
    //         open_video(oc, video_codec, &video_st, opt);

    //     // if (have_audio)
    //     //     open_audio(oc, audio_codec, &audio_st, opt);

    //     av_dump_format(oc, 0, filename, 1);

    //     /* open the output file, if needed */
    //     if (!(fmt->flags & AVFMT_NOFILE))
    //     {
    //         ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
    //         if (ret < 0)
    //         {
    //             fprintf(stderr, "Could not open '%s'", filename);
    //             return 1;
    //         }
    //     }

    //     /* Write the stream header, if any. */
    //     ret = avformat_write_header(oc, &opt);
    //     if (ret < 0)
    //     {
    //         fprintf(stderr, "Error occurred when opening output file:\n");
    //         return 1;
    //     }

    //     // while (encode_video || encode_audio)
    //     // {
    //     //     /* select the stream to encode */
    //     //     if (encode_video &&
    //     //         (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
    //     //                                         audio_st.next_pts, audio_st.enc->time_base) <= 0))
    //     //     {
    //     //         encode_video = !write_video_frame(oc, &video_st);
    //     //     }
    //     //     // else
    //     //     // {
    //     //     //     encode_audio = !write_audio_frame(oc, &audio_st);
    //     //     // }
    //     // }

    //     av_write_trailer(oc);

    //     /* Close each codec. */
    //     if (have_video)
    //         close_stream(oc, &video_st);
    //     // if (have_audio)
    //     //     close_stream(oc, &audio_st);

    //     if (!(fmt->flags & AVFMT_NOFILE))
    //         /* Close the output file. */
    //         avio_closep(&oc->pb);

    //     /* free the stream */
    //     avformat_free_context(oc);

    //     return 0;
    // }
}

// Timer structure
struct timer
{
    void reset()
    {
        start = std::chrono::steady_clock::now();
    }

    unsigned long long milliseconds_elapsed() const
    {
        const auto now = clock::now();
        using namespace std::chrono;
        return duration_cast<milliseconds>(now - start).count();
    }

    using clock = std::chrono::steady_clock;
    clock::time_point start = clock::now();
};

// TODO: Replace InputParser with Boost API
class InputParser
{
public:
    InputParser(int &argc, char **argv)
    {
        for (int i = 1; i < argc; ++i)
            this->tokens.push_back(std::string(argv[i]));
    }

    const std::string &getCmdOption(const std::string &option) const
    {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != this->tokens.end() && ++itr != this->tokens.end())
        {
            return *itr;
        }
        static const std::string empty_string("");
        return empty_string;
    }

    bool cmdOptionExists(const std::string &option) const
    {
        return std::find(this->tokens.begin(), this->tokens.end(), option) != this->tokens.end();
    }

private:
    std::vector<std::string> tokens;
};
/**
 * @brief print_usage statement
 * @param   choice: prints a specific usage statement given a value
 */
void print_usage(std::string choice = "")
{
    if (choice == "-depth_lossless")
    {
        std::cout << "-depth_lossless [int: 0 or 1]\t If you wish to store the depth frames losslessly compressed, causes high CPU usage." << std::endl;
    }
    else if (choice == "-crf_color")
    {
        std::cout << "-crf_color [int: 0-50]\t Control rate factor for RGB video" << std::endl;
    }
    else if (choice == "-color")
    {
        std::cout << "-color [int: 0 or 1]\tWhether to save color RGB stream as well" << std::endl;
    }
    else if (choice == "-dir")
    {
        std::cout << "-dir [string: DIRECTORY PATH TO SAVE VIDEO FILES]\t Please note that the directory cannot have any spaces" << std::endl;
        // break;
    }
    else if (choice == "-sec")
    {
        std::cout << "-sec [long: SECONDS TO RECORD FOR]" << std::endl;
        // break;
    }
    else if (choice == "-wd")
    {
        std::cout << "-wd [uint: WIDTH]" << std::endl;
        // break;
    }
    else if (choice == "-ht")
    {
        std::cout << "-ht [uint: HEIGHT]" << std::endl;
    }
    else if (choice == "-fps")
    {
        std::cout << "-fps [uint: FRAMERATE]" << std::endl;
    }
    else if (choice == "-crf")
    {
        std::cout << "-crf [uint: CONTROL RATE FACTOR]\t(DEFAULT=18) Recommended crf optional to change" << std::endl;
    }
    else if (choice == "-thr")
    {
        std::cout << "-thr [uint: THREADS IN]\t(OPT) Number of threads to utilize, doesn't effect libx265/HEVC encoding." << std::endl;
    }
    else if (choice == "-numraw")
    {
        std::cout << "-numraw [uint: NUMBER FRAMES RAW TO COLLECT]\t(OPT) Used only if you want to collect raw frames of the stream. May slow down framerate." << std::endl;
    }
    else if (choice == "-verbose")
    {
        std::cout << "-verbose [int: 0 or 1]\t(OPT) Used only if you wish to see exacty what ffmpeg will say from command line." << std::endl;
    }
    else if (choice == "-view")
    {
        std::cout << "-view [int: 0 or 1]\t(DEFAULT=0) Whether or not to preview the frames as you capture them. Does slow down fps." << std::endl;
        // break;
    }
    else if (choice == "-bagfile")
    {
        std::cout << "-bagfile [string: BAG FILE TO READ IN]\t(OPT) Used only in case you wish to read from a precorded bag file. Please note that this file name cannot have any spaces." << std::endl;
        // break;
    }
    else if (choice == "-jsonfile")
    {
        std::cout << "-jsonfile [string: JSON FILE PATH]\tFor intel realsense file configuration if needed. Please note that this json file name cannot have any spaces" << std::endl;
        // break;
    }
    else
    {
        std::cout << "Choice: " << choice << std::endl;
        std::cout << "Usage: ./bin/decompress" << std::endl;
        std::cout << "-h\t FLAG For help." << std::endl;
        std::cout << "-dir [string: DIRECTORY PATH TO SAVE VIDEO FILES]\t Please note that the directory cannot have any spaces" << std::endl;
        std::cout << "-sec [long: SECONDS TO RECORD FOR]" << std::endl;
        std::cout << "-wd [uint: WIDTH]" << std::endl;
        std::cout << "-ht [uint: HEIGHT]" << std::endl;
        std::cout << "-fps [uint: FRAMERATE]" << std::endl;
        std::cout << "-crf [uint: CONTROL RATE FACTOR]\t(DEFAULT=18) Recommended crf optional to change" << std::endl;
        std::cout << "-thr [uint: THREADS IN]\t(OPT) Number of threads to utilize, doesn't effect libx265/HEVC encoding." << std::endl;
        std::cout << "-numraw [uint: NUMBER FRAMES RAW TO COLLECT]\t(OPT) Used only if you want to collect raw frames of the stream. May slow down framerate." << std::endl;
        std::cout << "-verbose [int: 0 or 1]\t(OPT) Used only if you wish to see exacty what ffmpeg will say from command line." << std::endl;
        std::cout << "-view [int: 0 or 1]\t(DEFAULT=0) Whether or not to preview the frames as you capture them. Does slow down fps." << std::endl;
        std::cout << "-bagfile [string: BAG FILE TO READ IN]\t(OPT) Used only in case you wish to read from a precorded bag file. Please note that this file name cannot have any spaces." << std::endl;
        std::cout << "-jsonfile [string: JSON FILE PATH]\tFor intel realsense file configuration if needed. Please note that this json file name cannot have any spaces" << std::endl;
        std::cout << "-color [int: 0 or 1]\tWhether to save color RGB stream as well" << std::endl;
        std::cout << "-crf_color [int: 0-50]\t Control rate factor for RGB video" << std::endl;
        std::cout << "-depth_lossless [int: 0 or 1]\t If you wish to store the depth frames losslessly compressed, causes high CPU usage." << std::endl;
        // std::cout << "-rosbag [int: 0 or 1]\tWhether to save color .bag file and not compress" << std::endl;
    }
}
/**
 * @brief Takes in an input parser as long as the command is a number
 * @param input
 * @param cmd
 * @param prev_val
 * @return int--- Returns -5 if no command successful
 */
int parse_integer_cmd(InputParser &input, std::string cmd, int prev_val)
{
    int ret = prev_val;
    if (input.cmdOptionExists(cmd))
    {
        std::string opt = input.getCmdOption(cmd);
        if (opt.empty())
        {
            return -5;
        }
        if (!sscanf(opt.c_str(), "%d", &ret))
        {
            return -5;
        }
    }
    return ret;
}
/**
 * @brief Checks if a file exists or not
 *
 * @param path
 * @return bool
 *
 */
bool does_file_exist(std::string path)
{
    FILE *fp = NULL;
    if ((fp = fopen(path.c_str(), "r")))
    {
        fclose(fp);
        return true;
    }
    return false;
}

/**
 * @brief Checks if a directory path exists
 *
 * @param path
 * @return true
 * @return false
 */
bool does_dir_exist(std::string path)
{
    struct stat info;
    const char *pathname = path.c_str();
    if (stat(pathname, &info) != 0)
    {
        return false;
    }
    else if (info.st_mode & S_IFDIR)
    {
        return true;
    }
    else
    {
        return false;
    }
}
/**
 * @brief Builds a FFmpeg Command String
 *
 * @param pix_fmt
 * @param encoder
 * @param path_name
 * @param time_run
 * @param count_threads
 * @param width
 * @param height
 * @param fps
 * @param crf
 * @param ffmpeg_verbose
 * @param depth_lossless
 * @param typ
 * @return std::string
 */
std::string build_ffmpeg_cmd(std::string pix_fmt, std::string pix_fmt_out, std::string encoder, std::string path_name, long time_run,
                             int count_threads, int width, int height, int fps, int crf,
                             bool ffmpeg_verbose, bool depth_lossless, int typ)
{

    // typ 0 is LSB frames
    // typ 1 is MSB frames
    // type 2 is Color frames
    std::string thread_counter = (count_threads > 0 && count_threads < 8) ? " -threads " + std::to_string(count_threads) : "";
    std::string banner = (ffmpeg_verbose) ? " -loglevel repeat+level+debug " : " -loglevel error -nostats "; //" -loglevel trace";
    std::string ffmpeg_command;
    if (depth_lossless)
    {
        std::cout << "depth_lossless is a depricated feature" << std::endl;
        return "";
    }

    if (typ == 0 || typ == 2)
    {

        ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an -i - -vf format=" + pix_fmt + ",format=" + pix_fmt_out + " -color_range 2 -c:v " + encoder + " -color_range 2 -preset veryfast " + " -pix_fmt " + pix_fmt_out + " -crf " + std::to_string(crf) + " -movflags +write_colr " + path_name;
        return ffmpeg_command;
    }
    else if (typ == 1)
    {
        ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an -i - -c:v " + encoder + " -preset veryfast " + " -crf " + std::to_string(crf) + " " + path_name;
        return ffmpeg_command;
    }
    return "";
}

/**
 * @brief Starts a recording for Intel realsense,
 *        piping it through ffmpeg for compression using libx264rgb,
 *        two video files will be created one representing the upper 8 MSBs (most siginificant bits) of the 16 bit frame
 *        and the other storing the lower 8 LSBs. Another optional stream of RGB color can also be stored though at the sacrafice of more processing power needed
 *        Another optional argument is that you can read from a rosbag file. Or you could also configure JSON settings for cameras
 *
 * @param dirname   - Existing directory to write video to
 * @param time_run  - Time in seconds to record for
 * @param bag_file_dir - Bag file path to use
 * @param width  - Width in pixels of the video
 * @param height - Height in pixels of the video
 * @param fps - framerate of the video
 * @param crf_lossy - Control rate factor (CRF) of the lossy LSB video
 * @param count_threads - Number of threads to use for ffmpeg
 * @param ffmpeg_verbose - Whether or not to enable output of ffmpeg streams, only for debugging purposes
 * @param collect_raw - Whether or not to collect raw frames
 * @param num_frm_collect - Number of raw frames to collect
 * @param show_preview - Whether to show preview of frames collected
 * @param json_file - Select a particular JSON preset
 * @param color - Whether to store color RGB frames or not
 * @param crf_color - CRF of lossy color RGB video
 * @param depth_lossless - Whether or not to store the depth frames losslessly. Taxing on CPU.
 * @return int
 */
int startRecording(std::string dirname, long time_run, std::string bag_file_dir, unsigned int width = 1280U,
                   unsigned int height = 720U, unsigned int fps = 30U, int crf_lsb = 25, int count_threads = 2,
                   bool ffmpeg_verbose = false, bool collect_raw = false, int num_frm_collect = 0, bool show_preview = false,
                   std::string json_file = "", bool color = false, int crf_color = 30, bool depth_lossless = false)
{
    if (fps <= 0U || fps > 60U || height > 1080U || width > 1920U || time_run > 24L * 60 * 60 * 7 * 5 || crf_lsb > 50 || crf_lsb < -1 || crf_color < -1 || crf_color > 50 || time_run <= 0L)
    {
        std::cout << width << "x" << height << "@" << fps << "fps, Time run: " << time_run << " CRF: " << crf_lsb << std::endl;
        return 0;
    }
    if (fps > 30U)
    {
        std::cout << "FPS too high must be less than 30: " << fps << " fps" << std::endl;
        return 0;
    }
    if (height % 2 != 0 || width % 2 != 0 || width > 1280 || height > 720 || width < 640 || height < 480)
    {
        std::cout << "Dimensions incorrect: " << width << "x" << height << "@" << fps << "fps" << std::endl;
        return 0;
    }
    if (collect_raw && num_frm_collect < 0)
    {
        std::cout << "Numbec_cpp_propertiesr frames collect greater than 1800 at: " << num_frm_collect << std::endl;
        return 0;
    }
    int width_color = 640;  // width / 2;
    int height_color = 480; // height / 2;
    std::string path_raw = dirname + "test_raw";
    std::string path_msb = dirname + "test_msb.mp4";
    std::string path_lsb = dirname + "test_lsb.mp4";
    std::string color_lsb_path = dirname + "test_color.mp4";
    std::cout << ffmpeg_verbose << std::endl;

    std::string str_lsb = build_ffmpeg_cmd("gray10le", "yuv420p10le", "libx264", path_lsb, time_run, count_threads, width, height, fps, crf_lsb, ffmpeg_verbose, depth_lossless, 0);
    // std::string str_msb = build_ffmpeg_cmd("gray", "yuvj420p", "libx264", path_msb, time_run, count_threads, width, height, fps, 0, ffmpeg_verbose, depth_lossless, 1);
    std::string str_msb = build_ffmpeg_cmd("rgb24", "rgb24", "libx264rgb", path_msb, time_run, count_threads, width, height, fps, 0, ffmpeg_verbose, depth_lossless, 1);

    std::string color_lsb = build_ffmpeg_cmd("rgb24", "yuv420p", "libx264", color_lsb_path, time_run, count_threads, width_color, height_color, fps, crf_color, ffmpeg_verbose, depth_lossless, 2);
    // std::string raw_cmd = build_ffmpeg_cmd("gray16le", "gray16le", "rawvideo", path_raw, time_run, count_threads, width, height, fps, 0, ffmpeg_verbose, depth_lossless, 4);
    if (str_lsb == "" || str_msb == "")
    {
        std::cerr << "Error with build_ffmpeg_cmd " << std::endl;
        return 0;
    }
    if (color_lsb != "")
    {
        std::cout << "FFmpeg command for color stream too " << std::endl;
        std::cout << color_lsb << std::endl;
    }
    std::cout << "FFmpeg command for str_lsb LSB frames:" << std::endl;
    std::cout << str_lsb << std::endl;
    std::cout << "FFmpeg command for str_msb MSB frames:" << std::endl;
    std::cout << str_msb << std::endl;
    FILE *pipe_lsb = NULL;
    FILE *pipe_msb = NULL;
    FILE *p_pipe_raw = NULL;
    FILE *color_pipe = NULL;
#ifdef _WIN32
    p_pipe_raw = fopen(path_raw.c_str(), "w");

    if (p_pipe_raw == NULL)
    {
        std::cerr << "fopen error" << std::endl;
        return 0;
    }

    if (!(pipe_lsb = _popen(str_lsb.c_str(), "wb")) || !(pipe_msb = _popen(str_msb.c_str(), "wb")))
    {
        std::cerr << "_popen error" << std::endl;
        exit(1);
    }
    // }
    if (color)
    {
        if (!(color_pipe = _popen(color_lsb.c_str(), "wb")))
        {
            std::cerr << "_popen error 2" << std::endl;
            exit(1);
        }
    }
#else

    p_pipe_raw = fopen(path_raw.c_str(), "w");
    if (p_pipe_raw == NULL)
    {
        std::cerr << "fopen error" << std::endl;
        return 0;
    }
    /*
    if (!(pipe_lsb = popen(str_lsb.c_str(), "w")) )
    {
        std::cerr << "popen error" << std::endl;
        return 0;
    }
    */
    if (!(pipe_msb = popen(str_msb.c_str(), "w")))
    {
        std::cerr << "popen error" << std::endl;
        return 0;
    }
    if (color)
    {
        if (!(color_pipe = popen(color_lsb.c_str(), "w")))
        {
            std::cerr << "popen error 2" << std::endl;
            return 0;
        }
    }
#endif
    int q = 0;
    if (show_preview)
    {
#if __has_include(<example.hpp>)

        window app(1280, 960, "Camera Depth Device");

#endif
        q = 1;
    }
    rs2::context ctx;
    rs2::pipeline pipe(ctx);
    rs2::config cfg;
    // std::vector<rs2::pipeline

    if (json_file != "" && does_file_exist(json_file))
    {
        // auto device = ctx.query_devices();
        // auto dev = device[0];
        for (rs2::device &&dev : ctx.query_devices())
        {
            rs400::advanced_mode advanced_mode_dev = dev.as<rs400::advanced_mode>();
            if (!advanced_mode_dev.is_enabled())
            {
                // If not, enable advanced-mode
                advanced_mode_dev.toggle_advanced_mode(true);
                std::cout << "Advanced mode enabled. " << std::endl;
            }
            std::ifstream fp_file(json_file);
            std::string preset_json((std::istreambuf_iterator<char>(fp_file)), std::istreambuf_iterator<char>());

            advanced_mode_dev.load_json(preset_json);
        }
    }
    if (bag_file_dir.size() > 1 && does_dir_exist(bag_file_dir))
    {
        // cfg.enable_record_to_file("abag_file.bag");
        cfg.enable_device_from_file(bag_file_dir, false);
    }
    auto device = ctx.query_devices();
    auto dev = device[0];
    cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, fps); // Realsense configuration
    if (!(json_file.size() > 0))
    {
        auto advanced_mode_dev = dev.as<rs400::advanced_mode>();
        STDepthTableControl depth_table = advanced_mode_dev.get_depth_table();
        // depth_table.depthUnits = 0.001;
        // std::cout << "Depth units"
        depth_table.depthUnits = 1000;               // in micro meters
        depth_table.depthClampMin = 0;               // 100 mm
        depth_table.depthClampMax = (int)pow(2, 16); // pow(2, 16);// 1000 mm
        advanced_mode_dev.set_depth_table(depth_table);
        // if (!advanced_mode_dev.is_enabled())
        // {
        //     // If not, enable advanced-mode
        //     advanced_mode_dev.toggle_advanced_mode(true);
        //     std::cout << "Advanced mode enabled. " << std::endl;
        // }
        std::cout << "Depth max: " << advanced_mode_dev.get_depth_table().depthClampMax << "mm Depth unit: " << advanced_mode_dev.get_depth_table().depthUnits << std::endl;
    }
    if (color)
    {
        cfg.enable_stream(RS2_STREAM_COLOR, width_color, height_color, RS2_FORMAT_RGB8, fps);
    }
    pipe.start(cfg); // Start the pipe with the cfg

    /** Libav Help **/
    const AVCodec *codec;
    AVCodecContext *avctx = NULL;
    int ret = 0;
    int erre = 0;

    bool encode_video = false;
    FILE *lossy_out = NULL;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = {0, 0, 1, 0xb7};
    if (!(codec = avcodec_find_encoder_by_name("libx264")))
    {
        fprintf(stderr, "Codec libx264rgb not found\n");
        // std::cerr << "Codec libx264rgb not found" << std::endl;
        return 0;
    }
    avctx = avcodec_alloc_context3(codec);
    if (!avctx)
    {
        fprintf(stderr, "Could not allocate video codec context\n");
        // std::cerr << "Could not allocate video codec context" << std::endl;
        return 0;
    }
    pkt = av_packet_alloc();
    if (!pkt)
    {
        fprintf(stderr, "Couldn't allocate packet\n");
        // std::cerr << "Couldn't allocate packet" << std::endl;
        return 0;
    }

    // avctx->bit_rate = 8000;
    avctx->width = width;
    avctx->height = height;
    // Set frame rate
    avctx->time_base = (AVRational){1, (int)fps};
    avctx->framerate = (AVRational){(int)fps, 1};
    // avctx->gop_size = 5;
    // avctx->max_b_frames = 1;
    avctx->pix_fmt = AV_PIX_FMT_YUV420P10LE;
    avctx->delay = 2;
    avctx->thread_count = 4; //((int)std::thread::hardware_concurrency() <= 0) ? 2 : (int)std::thread::hardware_concurrency();
    // avctx->thread_type = FF_THREAD_FRAME;
    // avctx-
    //  if (codec->id == AV_CODEC_ID_H264)
    //  {
    
    av_opt_set(avctx->priv_data, "preset", "veryfast", 0);
    // av_opt_set(avctx->priv_data, "x265-params", "b-adapt=0:rc-lookahead=0:scenecut=0:frame-threads=1", 0);
    // av_opt_set(avctx->priv_data, "tune", "zerolatency", 0); // zerolatency needed for faster compression
    av_opt_set(avctx->priv_data, "crf", std::to_string(crf_lsb).c_str(), 0);
    //}
    ret = avcodec_open2(avctx, codec, NULL);
    if (ret < 0)
    {
        fprintf(stderr, "Could not open codec\n");
        return 0;
    }
    lossy_out = fopen(path_lsb.c_str(), "wb");
    if (!lossy_out)
    {
        fprintf(stderr, "Could not open %s\n", path_lsb.c_str());
        return 0;
    }
    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate video frame\n");
        return 0;
    }
    frame->format = avctx->pix_fmt;
    frame->width = avctx->width;
    frame->height = avctx->height;
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0)
    {
        fprintf(stderr, "Could not allocate the video frame data\n");
        return 0;
    }
    // filename = argv[1];

    /**End Libav help **/

    int num_bytes = width * height; //* bytes_per_pixel * sizeof(uint8_t);
    long counter = 0;
    std::string frame_file_nm = dirname + "frame_num_";
    int i = 0;
    int k = 0;
    float min_dis = 0.0f;
    float max_dis = 16.0f;
    std::vector<uint8_t> store_frame_lsb(height * width * 2, (uint8_t)0);
    std::vector<uint8_t> store_frame_msb(height * width * 3, (uint8_t)0);
    rs2::frameset frameset;
    rs2::colorizer coloriz;
    rs2::frame color_frame;
    rs2::frame depth_frame_in;
    rs2::frame rgb_frame;
    rs2::threshold_filter thresh_filter;
    thresh_filter.set_option(RS2_OPTION_MIN_DISTANCE, min_dis); // start at 0.0 meters away
    thresh_filter.set_option(RS2_OPTION_MAX_DISTANCE, max_dis); // Will not record anything beyond 16 meters away
    std::map<int, rs2::frame> render_frames;
    uint16_t *color_data = NULL;
    encode_video = 1;
    timer tStart;

    while ((long)time_run * fps > counter)
    {
        try
        {
            frameset = pipe.wait_for_frames(5000); // No frames seen for 5 seconds exit
        }
        catch (const rs2::error &e)
        {
            std::cout << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
            break;
        }
        if (depth_frame_in = frameset.get_depth_frame())
        {
            depth_frame_in = thresh_filter.process(depth_frame_in); // Filter frames that are between these two depths
            uint8_t *p_depth_frame_char = (uint8_t *)depth_frame_in.get_data();
            // std::copy(p_depth_frame_char, p_depth_frame_char + (num_bytes * 2), store_frame_lsb.begin());
            // uint8_t *df =
            //  rs2::depth_frame rse =  (rs2::depth_frame)depth_frame_in;
            //  const int stride= rse.get_stride_in_bytes();
            //  //
            //  // uint8_t *df = (uint8_t *)depth_frame_in.get_data();
            //  if(!color_data)
            //  {  //prepare dummy color plane for P010LE format, half the size of Y
            //      //we can't alloc it in advance, this is the first time we know realsense stride
            //      //the stride will be at least width * 2 (Realsense Z16, VAAPI P010LE)
            //      color_data = new uint16_t[((stride/2)*720)/2];
            //      for(i=0;i<(1280*720)/2;++i)
            //          color_data[i] = UINT16_MAX / 2; //dummy middle value for U/V, equals 128 << 8, equals 32768
            //  }
            for (i = 0, k = 0; i < num_bytes * 2; i += 2, k += 3)
            {
                store_frame_msb[k] = p_depth_frame_char[i + 1] >> 2;
            }
            if(encode_video){
                encode_video = write_frame_own(avctx, frame , pkt, lossy_out, p_depth_frame_char, 0);
                //frame->pts = avctx->frame_number;
                //++(frame->pts);
            }

            // if (encode_video)
            // {
            //     //

            //     encode_video = !write_video_frame(oc, &video_st, (uint8_t *)depth_frame_in.get_data(), time_run, (uint8_t*)color_data, stride);
            // }else{
            //     fprintf(stderr, "\n\nError encoding video only %ld\n\n", counter);
            // }

            if (!fwrite(store_frame_msb.data(), 1, height * width * 3U, pipe_msb))
            //|| !fwrite(store_frame_lsb.data(), 1, height * width * 2U, pipe_lsb))

            {
                std::cout << "Error with fwrite frames" << std::endl;
                break;
            }

            if (color)
            {
                if ((rgb_frame = frameset.get_color_frame()))
                {
                    if (!fwrite((uint8_t *)rgb_frame.get_data(), 1, height_color * width_color * 3, color_pipe))
                    {
                        std::cerr << "Error with fwrite frames 2" << std::endl;
                        break;
                    }
                }
            }
            /*
#if __has_include(<opencv2/opencv.hpp>)
            if (show_preview)
            {

                if (counter % 15 == 0)
                {


                    color_frame = coloriz.process(depth_frame_in);
                    cv::Mat img(cv::Size(width, height), CV_8UC3, (uint8_t *)color_frame.get_data(), cv::Mat::AUTO_STEP);
                    cv::imshow("Raw Preview", img);
                    std::string text = "Time: " + std::to_string(tStart.milliseconds_elapsed() / 1000.0);
                    cv::putText(img,                         // target image
                                text,                        // text
                                cv::Point(10, img.rows / 2), // top-left position
                                cv::FONT_HERSHEY_DUPLEX,
                                1.0,
                                CV_RGB(0, 0, 0), // font color
                                2);
                    if ((char)cv::waitKey(25) == 27) //
                    {
                        break;
                    }

                }
            }
#endif
*/
            if (collect_raw)
            {
                if (counter < (long)num_frm_collect)
                {
                    std::string bin_out = frame_file_nm + std::to_string(counter + 1) + "_data.bin";
                    FILE *p_file;
                    if ((p_file = fopen(bin_out.c_str(), "wb")))
                    {
                        // uint16_t* bit_data = (uint16_t*)depth_frame_in.get_data();
                        fwrite(p_depth_frame_char, 1, num_bytes * 2, p_file);
                        fclose(p_file);
                    }
                    // if (!fwrite((uint8_t*)depth_frame_in.get_data(), 1, num_bytes*2, p_pipe_raw))
                    // {
                    //     std::cout << "Error with fwrite frames raw" << std::endl;
                    //     break;
                    // }
                }
            }
            //             if (show_preview)
            //             {
            // #if __has_include(<example.hpp>)

            //                 render_frames[0] = coloriz.process(depth_frame_in);
            //                 app.show(render_frames);

            // #endif
            //             }
            ++counter;
        }
    }
    unsigned long long milliseconds_ellapsed = tStart.milliseconds_elapsed();
#if __has_include(<opencv2/opencv.hpp>)
    if (show_preview)
    {
        cv::destroyAllWindows();
    }
#endif

    pipe.stop();
    encode_video = write_frame_own(avctx, NULL, pkt, lossy_out, NULL, 1);
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO){
        fwrite(endcode, 1, sizeof(endcode), lossy_out);
    }
    fclose(lossy_out);

    avcodec_free_context(&avctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    // //return write_frame(oc, ost->enc, ost->st, get_video_frame(ost, time_run, data, color_data, stride), ost->tmp_pkt);
    // //write_frame(oc, (&video_st)->enc, (&video_st)->st, NULL, (&video_st)->tmp_pkt);
    // av_write_trailer(oc);
    // delete [] color_data;
    // /* Close each codec. */
    // if (have_video)
    //     close_stream(oc, &video_st);
    // // if (have_audio)
    // //     close_stream(oc, &audio_st);

    // if (!(fmt->flags & AVFMT_NOFILE))
    //     /* Close the output file. */
    //     avio_closep(&oc->pb);

    // /* free the stream */
    // avformat_free_context(oc);

    try
    {
#ifdef _WIN32
        // if (!rosbag)
        //{
        fflush(pipe_lsb);
        fflush(pipe_msb);
        _pclose(pipe_lsb);
        _pclose(pipe_msb);
        //}
        fflush(p_pipe_raw);
        fclose(p_pipe_raw);

        if (color)
        {
            fflush(color_pipe);
            _pclose(color_pipe);
            color_pipe = NULL;
        }
        p_pipe_lsb = NULL;
        pipe_msb = NULL;
#else
        // if(!rosbag){
        if (pipe_lsb)
        {
            fflush(pipe_lsb);
            pclose(pipe_lsb);
            pipe_lsb = NULL;
        }

        fflush(pipe_msb);
        pclose(pipe_msb);
        // }
        fflush(p_pipe_raw);
        fclose(p_pipe_raw);

        if (color)
        {
            fflush(color_pipe);
            pclose(color_pipe);
            color_pipe = NULL;
        }
        pipe_lsb = NULL;
        pipe_msb = NULL;
#endif
    }
    catch (std::exception &c)
    {
        std::cerr << "_pclose/pclose error" << std::endl;
        return 0;
    }

    std::cout << "Time run for in seconds: " << (milliseconds_ellapsed / 1000.0) << std::endl;
    return 1;
}
int main(int argc, char *argv[])
try
{
    // namespace po = boost::program_options;
    // using boost::program_options::detail::cmdline;
    std::cout << "RUNNING multicam c++ command line......" << std::endl;
    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    if (argc > 34 || argc < 2)
    {
        // std::cout << " THIS FAILED HERE 1: argc: " << argc << std::endl;
        print_usage("");
        return EXIT_FAILURE;
    }
    InputParser input(argc, argv);
    if (input.cmdOptionExists("-h"))
    {
        std::cout << "Command line utility for compressing depth data and rgb data and infared from a camera." << std::endl;
        print_usage();
        return EXIT_FAILURE;
    }
    std::string dir = "";
    long sec = 0;
    int width = 1280;
    int height = 720;
    int fps = 30;
    int crf = 18;
    int thr = 1;
    int numraw = 0;
    int verbose = 0;
    int view = 0;
    int color = 0;
    int crf_color = 30;
    int depth_lossless = 0;
    // int rosbag = 0;
    // int opt;
    // po::options_description desc("Allowed options");
    // desc.add_options()
    //     ("help", "produce help message")
    //     ("optimization", po::value<int>(&opt)->default_value(10),
    // "optimization level")
    //     ("include-path,I", po::value< std::vector<std::string> >(),
    // "include path")
    //     ("input-file", po::value< std::vector<std::string> >(), "input file")
    // ;

    std::string bagfile = "";
    std::string jsonfile = "";
    if (!input.cmdOptionExists("-dir"))
    {
        std::cout << "ERROR: " << std::endl;
        print_usage("-dir");
        return EXIT_FAILURE;
    }
    if (!input.cmdOptionExists("-sec"))
    {
        std::cout << "ERROR: " << std::endl;
        print_usage("-sec");
        return EXIT_FAILURE;
    }

    dir = input.getCmdOption("-dir");
    if (dir.empty() || !does_dir_exist(dir))
    {
        std::cout << "ERROR -dir" << std::endl;
        print_usage("-dir");
        return EXIT_FAILURE;
    }
    std::string sec_in = input.getCmdOption("-sec");
    if (sec_in.empty() || !sscanf(sec_in.c_str(), "%ld", &sec))
    {
        std::cout << "Parse ERROR: " << input.getCmdOption("-sec") << std::endl;
        print_usage("-sec");
        return EXIT_FAILURE;
    }

    if (input.cmdOptionExists("-bagfile"))
    {
        bagfile = input.getCmdOption("-bagfile");
        if (bagfile.empty() || !does_file_exist(bagfile))
        {
            std::cout << "Error bagfile" << std::endl;
            print_usage("-bagfile");
            return EXIT_FAILURE;
        }
    }
    if (input.cmdOptionExists("-jsonfile"))
    {
        jsonfile = input.getCmdOption("-jsonfile");
        if (jsonfile.empty() || !does_file_exist(jsonfile))
        {
            std::cout << "Error jsonfile" << std::endl;
            print_usage("-jsonfile");
            return EXIT_FAILURE;
        }
    }
    width = parse_integer_cmd(input, "-wd", width);
    height = parse_integer_cmd(input, "-ht", height);
    fps = parse_integer_cmd(input, "-fps", fps);
    crf = parse_integer_cmd(input, "-crf", crf);
    thr = parse_integer_cmd(input, "-thr", thr);
    numraw = parse_integer_cmd(input, "-numraw", numraw);
    verbose = parse_integer_cmd(input, "-verbose", verbose);
    view = parse_integer_cmd(input, "-view", view);
    color = parse_integer_cmd(input, "-color", color);
    crf_color = parse_integer_cmd(input, "-crf_color", crf_color);
    depth_lossless = parse_integer_cmd(input, "-depth_lossless", depth_lossless);
    // rosbag = parse_integer_cmd(input, "-rosbag", rosbag);
    if (width == -5 || width < 200 || width > 1920)
    {
        print_usage("-wd");
        return EXIT_FAILURE;
    }
    if (height == -5 || height < 200 || height > 1080)
    {
        print_usage("-ht");
        return EXIT_FAILURE;
    }
    if (fps == -5 || fps > 30 || fps < 1)
    {
        print_usage("-fps");
        return EXIT_FAILURE;
    }
    if (crf < -1 || crf > 51)
    {
        print_usage("-crf");
        return EXIT_FAILURE;
    }
    if (thr < 1)
    {
        print_usage("-thr");
        return EXIT_FAILURE;
    }
    if (numraw < 0 || (numraw >= fps * sec && (long)height * width * fps * 2 * sec > (long)5e+11))
    {
        print_usage("-numraw");
        return EXIT_FAILURE;
    }
    if (verbose != 0 && verbose != 1)
    {
        print_usage("-verbose");
        return EXIT_FAILURE;
    }
    if (view != 0 && view != 1)
    {
        print_usage("-view");
        return EXIT_FAILURE;
    }
    if (color < 0 || color > 1)
    {
        print_usage("-color");
        return EXIT_FAILURE;
    }
    if (crf_color > 50 || crf_color < -1)
    {
        print_usage("-crf_color");
        return EXIT_FAILURE;
    }
    if (depth_lossless < 0 || depth_lossless > 1)
    {
        print_usage("-depth_lossless");
        return EXIT_FAILURE;
    }
    // if(rosbag < 0 || rosbag > 1){
    //     print_usage("-rosbag");
    //     return EXIT_FAILURE;
    // }
    std::cout << "dir: " << dir << std::endl;
    std::cout << "sec: " << sec << std::endl;
    std::cout << "bagfile: " << bagfile << std::endl;
    std::cout << "width: " << width << std::endl;
    std::cout << "height: " << height << std::endl;
    std::cout << "fps: " << fps << std::endl;
    std::cout << "crf: " << crf << std::endl;
    std::cout << "thr: " << thr << std::endl;
    std::cout << "verbose:" << verbose << std::endl;
    std::cout << "numraw: " << numraw << std::endl;
    std::cout << "view: " << view << std::endl;
    std::cout << "jsonfile: " << jsonfile << std::endl;
    std::cout << "color: " << color << std::endl;
    std::cout << "crf_color " << crf_color << std::endl;
    std::cout << "depth_lossless: " << depth_lossless << std::endl;
    // std::cout << "rosbag: " << bool(rosbag) << std::endl;
    int retval = startRecording(dir, sec, bagfile, width, height, fps, crf, thr, bool(verbose), bool(numraw), numraw, bool(view), jsonfile, bool(color), crf_color, bool(depth_lossless));
    if (!retval)
    {
        std::cout << "FAILURE WHILE RECORDING" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "SUCCESS, CLOSING...." << std::endl;
    return EXIT_SUCCESS;
}
catch (const rs2::error &e)
{
    std::cout << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception &e)
{
    std::cout << e.what() << std::endl;
    std::cout << "Other error" << std::endl;
    return EXIT_FAILURE;
}
