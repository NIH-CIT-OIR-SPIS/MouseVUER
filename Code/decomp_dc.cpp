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
extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx;
static int width, height;
static enum AVPixelFormat pix_fmt;
static AVStream *video_stream = NULL, *audio_stream = NULL;
static const char *src_filename = NULL;
static const char *video_dst_filename = NULL;
static const char *audio_dst_filename = NULL;
static FILE *video_dst_file = NULL;
static FILE *audio_dst_file = NULL;

static uint8_t *video_dst_data[4] = {NULL};
static int video_dst_linesize[4];
static int video_dst_bufsize;

static int video_stream_idx = -1, audio_stream_idx = -1;
// static AVFrame *frame = NULL;
static AVPacket *pkt = NULL;
static int video_frame_count = 0;
static int audio_frame_count = 0;
// g++ -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -fopenmp -O3 -Wall decomp_dc.cpp  -pthread -lavcodec -lavformat -lavutil -lswresample -lm -lz -lswscale -o dmux_decode && ./dmux_decode Testing_DIR/test_lsb.mp4 Testing_DIR/test_msb.mp4 testout_r

static AVFormatContext *fmt_ctx_msb = NULL;
static AVCodecContext *video_dec_ctx_msb = NULL;
static int width_msb, height_msb;
static enum AVPixelFormat pix_fmt_msb;
static AVStream *video_stream_msb = NULL, *audio_stream_msb = NULL;
static const char *src_filename_msb = NULL;
static const char *video_dst_filename_msb = NULL;
static const char *audio_dst_filename_msb = NULL;
static FILE *video_dst_file_msb = NULL;
static FILE *audio_dst_file_msb = NULL;

static uint8_t *video_dst_data_msb[4] = {NULL};
static int video_dst_linesize_msb[4];
static int video_dst_bufsize_msb;

static int video_stream_idx_msb = -1, audio_stream_idx_msb = -1;
static AVFrame *frame_msb = NULL;
static AVPacket *pkt_msb = NULL;
static int video_frame_count_msb = 0;
static int audio_frame_count_msb = 0;
static uint8_t store_buf[1280 * 720 * 2] = {0};
static int count_loop = 0;
static int count_msb = 0;
static int count_lsb = 0;
// static int local_count_msb = 0;
// static int local_count_lsb = 0;
static AVFrame *lst_lsb[10] = {NULL};
static AVFrame *lst_msb[10] = {NULL};
static int output_video_frame(AVFrame *frame)
{
    AVRational fel;
    // if (frame->width != width || frame->height != height ||
    //     frame->format != pix_fmt)
    // {
    //     /* To handle this change, one could call av_image_alloc again and
    //      * decode the following frames into another rawvideo file. */
    //     fprintf(stderr, "Error: Width, height and pixel format have to be "
    //                     "constant in a rawvideo file, but the width, height or "
    //                     "pixel format of the input video changed:\n"
    //                     "old: width = %d, height = %d, format = %s\n"
    //                     "new: width = %d, height = %d, format = %s\n",
    //             width, height, av_get_pix_fmt_name(pix_fmt),
    //             frame->width, frame->height,
    //             av_get_pix_fmt_name(frame->format));
    //     return -1;
    // }

    if (video_frame_count == 449)
    {
        printf("video_frame n:%d coded_n:%d, pts_n: "
               "%" PRId64 "\n",
               video_frame_count, frame->coded_picture_number, frame->pts);
    }
    video_frame_count++;
    /* copy decoded frame to destination buffer:
     * this is required since rawvideo expects non aligned , av_ts2timestr(frame->pts, &->time_base)data */
    // av_image_copy(video_dst_data, video_dst_linesize,
    //               (const uint8_t **)(frame->data), frame->linesize,
    //               pix_fmt, width, height);

    // /* write to rawvideo file */
    // fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
    return 0;
}

static int output_audio_frame(AVFrame *frame)
{
    /*
    size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(frame->format);
    printf("audio_frame n:%d nb_samples:%d pts:%s\n",
           audio_frame_count++, frame->nb_samples,
           av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));


    fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);
    */
    return 0;
}
static void extract_yuv_plane(AVFrame *frame, int width, int height)
{
    int x = 0, y = 0, i = 0;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            store_buf[i++] = frame->data[0][y * frame->linesize[0] + x];
        }
    }
    // for(i = 0; i < width*height*2; ++i){
    //     memcpy(store_buf + i, frame->data[0] + i, 1);
    // }
    // int linesize = frame->linesize[0];
    // uint8_t *data = frame->data[0];

    // for (i = 0; i < height; i++) {
    //     memcpy(buf, data, width);
    //     buf += width;
    //     data += linesize;
    // }
    // return store_buf;
    // return buf;
}
/*
static int decode_packet(AVCodecContext *dec, AVCodecContext *dec_msb,
                         const AVPacket *pkt, const AVPacket *pkt_msb, AVFrame *frame, AVFrame *frame_msb, int flush, int read_frm_lsb, int read_frm_msb)
{
    int ret = 0, ret_msb = 0;
    int64_t er = 0, er_msb = 0;
    int count_lsb = 0, count_msb = 0;
    int i = 0, j = 0;
    AVFrame *lsb_lst[6] = {NULL};
    AVFrame *msb_lst[6] = {NULL};
    if (read_frm_lsb >= 0)
    {
        ret = avcodec_send_packet(dec, pkt);
        if (ret < 0)
        {
            fprintf(stderr, "Error sending a packet for decoding \n");
            return ret;
        }
    }
    if (read_frm_msb >= 0){
        ret_msb = avcodec_send_packet(dec_msb, pkt_msb);
        if (ret_msb < 0 && count_loop >= 3)
        {
            fprintf(stderr, "Error sending a packet for decoding\n");
            return ret_msb;
        }
    }
    // fprintf(stderr, "read_frm_lsb %d read_frm_msb %d\n", read_frm_lsb, read_frm_msb);
    // if(flush){
    //     avcodec_send_frame(dec, NULL);
    //     avcodec_send_frame(dec_msb, NULL);
    // }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(dec, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {

            break;
        }
        else if (ret < 0)
        {

            fprintf(stderr, "Error during decoding lsb: ret: %d\n", ret);
            exit(1);
        }
        lsb_lst[count_lsb++] = frame;
        er = frame->best_effort_timestamp;
        count_loop_lsb++;
        // av_frame_unref(frame);
    }
    // av_frame_unref(frame);
    if(read_frm_msb >= 0){
        while (ret_msb >= 0)
        {
            ret_msb = avcodec_receive_frame(dec_msb, frame_msb);
            if (ret_msb == AVERROR(EAGAIN) || ret_msb == AVERROR_EOF)
            {

                break;
            }
            else if (ret_msb < 0)
            {
                fprintf(stderr, "Error during decoding msb: ret: %d\n", ret_msb);
                exit(1);
            }
            msb_lst[count_msb++] = frame_msb;
            er_msb = frame_msb->best_effort_timestamp;
            count_loop_msb++;
            // av_frame_unref(frame_msb);
        }
    }
    while (lsb_lst[i] != NULL)
    {
        // extract_yuv_plane(lsb_lst[i], width, height);
        av_frame_unref(lsb_lst[i]);
        i++;
    }

    while (msb_lst[j] != NULL)
    {
        // extract_yuv_plane(msb_lst[j], width, height);
        av_frame_unref(msb_lst[j]);
        j++;
    }
    if (count_loop < 3)
    {
        fprintf(stderr, ">>>>>>\n");
        fprintf(stderr, "ret: %d, ret_msb: %d, er: %" PRId64 " er_msb:%" PRId64 " \n", ret, ret_msb, er, er_msb);
    }
    fprintf(stderr, "lsb: %d, msb: %d, i: %d, j: %d, count_loop: %d, count_loop_lsb: %d count_loop_msb %d\n", count_lsb, count_msb, i, j, count_loop, count_loop_lsb, count_loop_msb);
    ++count_loop;
    av_frame_unref(frame);
    av_frame_unref(frame_msb);

    return ret == AVERROR_EOF || ret_msb == AVERROR_EOF || ret_msb == AVERROR(EAGAIN) || ret == AVERROR(EAGAIN) ? 0 : 1;

}*/

static int decode_packet(AVCodecContext *dec, const AVPacket *pkt, AVFrame *frame, int typ, int *count)
{
    int ret = 0;
    // extract_yuv_plane(frame_in, 1280, 720);

    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0)
    {
        fprintf(stderr, "Error submitting a packet for decoding \n");
        return ret;
    }

    // get all the available frames from the decoder
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(dec, frame);
        if (ret < 0)
        {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
            {
                //fprintf(stderr, "called here ret: %d, count_loop %d, before: %d\n", ret, count_loop, before);
                return 0;
            }
            fprintf(stderr, "Error during decoding \n");
            return ret;
        }

        // write the frame data to output file
        if (*count >= 0)
        {
            // ret = output_video_frame(frame_in);

            if (typ == 0)
            {
                // set = 1;
                lst_lsb[*count] = frame;
                ++(*count);
                ++count_lsb;
                ret = output_video_frame(lst_lsb[(*count - 1)]);
                
                //fprintf(stderr, " >>> lsb count_lsb: %d, *count %d\n", count_lsb, *count);
            }
            else if (typ == 1)
            {
                // set = 1;
                lst_msb[*count] = frame;
                ++(*count);
                ++count_msb;
                ret = output_video_frame(lst_msb[(*count - 1)]);
                //fprintf(stderr, "  &&& msb count_msb: %d, *count %d\n", count_msb, *count);
            }

            //++count_loop;
        }

        av_frame_unref(frame);
        if (ret < 0)
        {
            return ret;
        }
    }

    return 0;
}

static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
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

static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry
    {
        enum AVSampleFormat sample_fmt;
        const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        {AV_SAMPLE_FMT_U8, "u8", "u8"},
        {AV_SAMPLE_FMT_S16, "s16be", "s16le"},
        {AV_SAMPLE_FMT_S32, "s32be", "s32le"},
        {AV_SAMPLE_FMT_FLT, "f32be", "f32le"},
        {AV_SAMPLE_FMT_DBL, "f64be", "f64le"},
    };
    *fmt = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++)
    {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt)
        {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int read_frm_lsb = -1;
    int read_frm_msb = -1;
    int *pt_y = NULL;
    int *pt_x = NULL;
    int i = 0, j = 0, y = 0, x = 0;

    AVFrame *frame_in = NULL;
    pt_x = &x;
    pt_y = &y;
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s  input_file video_output_file audio_output_file\n"
                        "API example program to show how to read frames from an input file.\n"
                        "This program reads frames from a file, decodes them, and writes decoded\n"
                        "video frames to a rawvideo file named video_output_file, and decoded\n"
                        "audio frames to a rawaudio file named audio_output_file.\n",
                argv[0]);
        exit(1);
    }
    src_filename = argv[1];
    src_filename_msb = argv[2];
    video_dst_filename = argv[3];
    // video_dst_filename = argv[2];
    // audio_dst_filename = argv[3];

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0)
    {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }
    /* open input file, and allocate format context */
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
    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx_msb, NULL) < 0)
    {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }
    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0)
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

    if (open_codec_context(&video_stream_idx_msb, &video_dec_ctx_msb, fmt_ctx_msb, AVMEDIA_TYPE_VIDEO) >= 0)
    {
        video_stream = fmt_ctx_msb->streams[video_stream_idx_msb];

        // video_dst_file = fopen(video_dst_filename, "wb");
        if (!video_dst_file)
        {
            fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
            ret = 1;
            goto end;
        }

        /* allocate image where the decoded image will be put */
        width_msb = video_dec_ctx_msb->width;
        height_msb = video_dec_ctx_msb->height;
        pix_fmt_msb = video_dec_ctx_msb->pix_fmt;
        ret = av_image_alloc(video_dst_data_msb, video_dst_linesize_msb,
                             width_msb, height_msb, pix_fmt_msb, 1);
        if (ret < 0)
        {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            goto end;
        }
        video_dst_bufsize_msb = ret;
    }

    if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0)
    {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dst_file = fopen(audio_dst_filename, "wb");
        if (!audio_dst_file)
        {
            fprintf(stderr, "Could not open destination file %s\n", audio_dst_filename);
            ret = 1;
            goto end;
        }
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    // if (!audio_stream && !video_stream)
    // {
    //     fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
    //     ret = 1;
    //     goto end;
    // }

    frame_in = av_frame_alloc();
    if (!frame_in)
    {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }
    frame_msb = av_frame_alloc();
    if (!frame_msb)
    {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
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
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (video_stream)
    {
        printf("Demuxing video from file '%s' into '%s'\n", src_filename, video_dst_filename);
    }

    if (video_stream_msb)
    {
        printf("Demuxing msb video from file '%s' into '%s'\n", src_filename_msb, video_dst_filename);
    }
    /* read frames from the file */
    // int k = 0;
    while (count_msb < 1799 && count_lsb < 1799)
    {
        while (*pt_x < 10)
        {
            // check if the packet belongs to a stream we are interested in, otherwise
            // skip it
            if (av_read_frame(fmt_ctx, pkt) >= 0)
            {
                if (pkt->stream_index == video_stream_idx)
                {

                    ret = decode_packet(video_dec_ctx, pkt, frame_in, 0, pt_x);
                }

                av_packet_unref(pkt);

                if (ret < 0)
                {
                    break;
                }
            }
            else
            {
                break;
            }
            // printf("%d\n", ++k);
        }
        *pt_x = 0;
        while (*pt_y < 10)
        {

            if (av_read_frame(fmt_ctx_msb, pkt_msb) >= 0)
            {
                // check if the packet belongs to a stream we are interested in, otherwise
                // skip it
                if (pkt_msb->stream_index == video_stream_idx_msb)
                    ret = decode_packet(video_dec_ctx_msb, pkt_msb, frame_msb, 1, pt_y);

                av_packet_unref(pkt_msb);
                if (ret < 0)
                    break;
                // printf("%d\n", ++k);
            }
            else
            {
                break;
            }
        }
        *pt_y = 0;
        *pt_x = 0;
        i = 0;
        //fprintf(stderr, "hi\n");
        while (i < 10) 
        {
            av_frame_unref(lst_lsb[i]);
            lst_lsb[i] = NULL;
            ++i;
        }
        j = 0;
        while (j < 10)
        {
            av_frame_unref(lst_msb[j]);
            lst_msb[j] = NULL;
            ++j;
        }
        
    }

    *pt_x = -1;
    *pt_y = -1;

    if (video_dec_ctx)
    {
        decode_packet(video_dec_ctx, NULL, frame_in, 0, pt_x);
    }
    if (video_dec_ctx_msb)
    {
        decode_packet(video_dec_ctx_msb, NULL, frame_msb, 1, pt_y);
    }
    // decode_packet(video_dec_ctx_msb, NULL, frame_msb);

    /*
    if(video_stream_msb){
        printf("Demuxing msb video from file '%s' into '%s'\n", src_filename_msb, video_dst_filename);

    }
    while (av_read_frame(fmt_ctx_msb, pkt_msb) >= 0)
    {
        // check if the packet belongs to a stream we are interested in, otherwise
        // skip it
        if (pkt_msb->stream_index == video_stream_idx_msb)
            ret = decode_packet(video_dec_ctx_msb, pkt_msb, frame_msb);

        av_packet_unref(pkt_msb);
        if (ret < 0)
            break;
        // printf("%d\n", ++k);
    }
    */

    // if (audio_dec_ctx)
    //     decode_packet(audio_dec_ctx, NULL, frame_in);

    printf("Demuxing succeeded.\n");

    if (video_stream)
    {
        printf("Play the output video file with the command:\n"
               "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
               av_get_pix_fmt_name(pix_fmt), width, height,
               video_dst_filename);
    }
    /*
    if (audio_stream)
    {
        enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
        int n_channels = audio_dec_ctx->channels;
        const char *fmt;

        if (av_sample_fmt_is_planar(sfmt))
        {
            const char *packed = av_get_sample_fmt_name(sfmt);
            printf("Warning: the sample format the decoder produced is planar "
                   "(%s). This example will output the first channel only.\n",
                   packed ? packed : "?");
            sfmt = av_get_packed_sample_fmt(sfmt);
            n_channels = 1;
        }

        if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
            goto end;

        printf("Play the output audio file with the command:\n"
               "ffplay -f %s -ac %d -ar %d %s\n",
               fmt, n_channels, audio_dec_ctx->sample_rate,
               audio_dst_filename);
    }
    */
end:
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&video_dec_ctx_msb);
    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    avformat_close_input(&fmt_ctx_msb);
    if (video_dst_file)
        fclose(video_dst_file);
    // if (audio_dst_file)
    //     fclose(audio_dst_file);
    av_packet_free(&pkt);
    av_packet_free(&pkt_msb);
    av_frame_free(&frame_in);
    av_frame_free(&frame_msb);
    av_free(video_dst_data[0]);
    av_free(video_dst_data_msb[0]);
    return ret < 0;
}
