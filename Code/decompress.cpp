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
#include <stdexcept>
#include <cstdio>
#include <array>
#include <limits>
#include <numeric>
#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#endif   
extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#define FRM_GROUP_SIZE 10
#define W 1280
#define H 720
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
//static const char *video_dst_filename_msb = NULL;
//static const char *audio_dst_filename_msb = NULL;
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
static uint16_t store_depth[1280 * 720] = {0};
static uint16_t store_depth_lsb[1280 * 720] = {0};
static uint16_t store_raw_depth[1280 * 720] = {0};
static std::vector<double> psnr_vector;
static char * out_write = "Testing_DIR/testout_r.mp4";


template<typename T>
double getAverage(std::vector<T> const& v) {
    if (v.empty()) {
        return 0;
    }
 
    double sum = 0.0;
    for (const T &i: v) {
        sum += (double)i;
    }
    return sum / v.size();
}

/**
 * @brief Loops over an image patch given the patchHeight and patchWidth are fixed 
 * and assuming that we don't have to caclulate on the fly. 
 * 
 * 
 * @param data 
 * @param width 
 * @param height 
 * @return int 
 */
int loop_image_patch(uint16_t *data, int width, int height)
{

    int bw = 0;
    int bh = 0;
    int y = 0, x = 0;
    int numBlocks = 0;
    int sizer = 0;
    float sum = 0;
    int patch_height = 4;
    int patch_width = 4;

    if ((width*height) % (patch_width*patch_height) != 0)
    {
        return 0;
        //numBlocks = (width*height) / (patch_width*patch_height);
    }
    // else
    // {
    //     //numBlocks = (width*height) / (patch_width*patch_height) + 1;
    // }
    
    for(y = 0; y < height; y += patch_height){
        for(x = 0; x < width; x += patch_width){
            sum = 0.0;
            sizer = 0;
            for(bh = 0; bh < patch_height; ++bh){
                for(bw = 0; bw < patch_width; ++bw){
                    sum += data[(y + bh) * width + x + bw];
                    ++sizer;
                }
            }
            // if(sizer != patch_height * patch_width){
            //     printf("Whats going on %d\n ", sizer);
            // }
            sum /= patch_height * patch_width; // calculating average of the image patch
            ++numBlocks;
        }
    }
    //printf("Num Blocks %d\n", numBlocks);
    return numBlocks;
}

static inline int  abs_diff(int a, int b)
{

    if (a - b < 0)
    {
        return (-1 * (a - b));
    }
    else
    {
        return a - b;
    }
}

static double get_psnr(uint16_t * m0, uint16_t * m1)
{
    long cg = 65535U * 65535U;
    uint sum_sq = 0;
    double mse = 0;
//#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i <1280*720; ++i){
        int p1 = m0[i];
        int p2 = m1[i];
        int err = abs_diff(p2, p1);
        sum_sq += (err * err);
    }
    //res = res / (height * width);
    mse = (double)sum_sq / (H*W);
    return (10.0 * log10(cg / mse));
}

static int exec_ffprobe(std::string str_cmd) {
    const char * cmd = str_cmd.c_str();
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return  atoi(result.c_str());
}

static void lineRead( uint16_t * store_depth_lsb_in){
        FILE * out_put_diff = NULL;
        char *output_diff_fl = "Testing_DIR/output_diff_fl.txt";
        if(!(out_put_diff = fopen(output_diff_fl, "w")))
        {
            printf("Could not open file for writing\n");
            exit(1);
        }
        std::vector<int> rand_vec;
        std::ifstream infile("Testing_DIR/output_vec_ind_file.txt");
        int a;
        while(infile >> a){
            //printf("%d\n", a);
            rand_vec.push_back(a);
        }
        infile.close();
        while(rand_vec.size() > 0){

            fprintf(out_put_diff, "%d\n", store_depth_lsb_in[rand_vec.back()]);
            rand_vec.pop_back();
        }
        fclose(out_put_diff);
}

static int output_both_buffs(uint8_t *frame_lsb, uint8_t *frame_msb)
{


    int i = 0, y = 0, count = 0;
    double psnr_val = 0;
    uint16_t max = 0;
    uint16_t curr_lsb = 0;
    uint16_t store_num = 0;
    uint16_t curr_msb = 0;
    FILE *out_write = NULL;
    
    FILE *read_raw = NULL;
    std::string raw_str = "Testing_DIR/frame_num_" + std::to_string(video_frame_count) + "_data.bin";
    std::string outer = "Testing_DIR/file_out_comp" + std::to_string(video_frame_count) + ".bin";

    if(!(out_write = fopen(outer.c_str(), "wb")) || !(read_raw = fopen(raw_str.c_str(), "rb")))
    {
        printf("Could not open file for writing\n");
        return -1;
    }
    fread(store_raw_depth, sizeof(uint16_t), H*W, read_raw);

    for (i = 0, y = 0; i < H*W*2; i += 2, ++y){
        curr_lsb = (uint16_t)frame_lsb[i] |  (((uint16_t)frame_lsb[i+1]) << 8);
        curr_msb =  ((uint16_t)frame_msb[y]) << 10;
        
        store_num = curr_lsb | curr_msb;
        // if(store_lsb > max){
        //     max = store_lsb;
        // }
        store_depth[y] = store_num;
        store_depth_lsb[y] = curr_lsb;
        // if(i == 460800*2 && video_frame_count ==200){
        //     fprintf(stderr, "store_depth[%d] = %u\n", i, curr_lsb);
        // }
    }
    

    if(video_frame_count == 200){
        lineRead(store_depth_lsb);
    }


    loop_image_patch(store_depth, W, H); //Will utilize later


    psnr_val = get_psnr(store_depth, store_raw_depth);
    printf("PSNR value = %f\n", psnr_val);
    psnr_vector.push_back(psnr_val);


#if __has_include(<opencv2/opencv.hpp>)
    cv::Mat raw_img_color(cv::Size(W, H), CV_8UC3);
    raw_img_color = 0;
    cv::Mat dec_img_color(cv::Size(W, H), CV_8UC3);
    dec_img_color = 0;
    cv::Mat dec_img(cv::Size(W, H), CV_16U, store_depth, cv::Mat::AUTO_STEP);
    cv::Mat raw_img(cv::Size(W, H), CV_16U, store_raw_depth, cv::Mat::AUTO_STEP);




    cv::normalize(dec_img, dec_img, 0, 65535, cv::NORM_MINMAX);
    cv::normalize(raw_img, raw_img, 0, 65535, cv::NORM_MINMAX);
    // cv::normalize(dec_img, dec_img_color, 0, 6, cv::NORM_MINMAX);
    // cv::normalize(raw_img, raw_img_color, 0, 255, cv::NORM_MINMAX);
    dec_img.convertTo(raw_img_color, CV_8U, 1.0/255);
    raw_img.convertTo(dec_img_color, CV_8U, 1.0/255);
    //uint8_t *ptr_dec_img_color = dec_img_color.data;


    //psnr_val = get_psnr(store_depth, store_raw_depth);
    //printf("Get other PSNR value = %f\n", psnr_val);
    //raw_img.convertTo(raw_img, CV_8U, 0.7);
    cv::applyColorMap(dec_img_color, dec_img_color, 9);
    cv::applyColorMap(raw_img_color, raw_img_color, 9);
    cv::namedWindow("Decompressed Image", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Raw Image", cv::WINDOW_AUTOSIZE);
    cv::imshow("Decompressed Image", dec_img_color);
    cv::imshow("Raw Image", raw_img_color);
    


    if (video_frame_count == 30){
        cv::imwrite("Testing_DIR/dec_img_30.png", dec_img_color);
        cv::imwrite("Testing_DIR/raw_img_30.png", raw_img_color);
    }
    if ((char)cv::waitKey(25) == 27) //
    {
        cv::destroyAllWindows();
        exit(1);
    }
    //10872 vs 637

#endif   
    fwrite(store_depth, sizeof(uint16_t), 1280*720, out_write);
    fclose(out_write);
    //printf("max: %u\n", max);
    //fprintf(stderr, "video_frame_count: %d\n", video_frame_count);

    // fprintf(stderr, "lsb_frame pts_n: %" PRId64 " time_stamp: %" PRId64 " \n", frame_lsb->pts, frame_lsb->best_effort_timestamp);
    // fprintf(stderr, "msb_frame pts_n: %" PRId64 " time_stamp: %" PRId64 " \n", frame_msb->pts, frame_msb->best_effort_timestamp);
    ++video_frame_count;
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
}

static int decode_packet(AVCodecContext *dec, const AVPacket *pkt, AVFrame *frame, int typ, int *count, uint8_t *data[10],
                         int *ptr_frm_count)
{
    int ret = 0;
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
                // fprintf(stderr, "called here ret: %d, count_loop %d, before: %d\n", ret, count_loop, before);
                return 0;
            }
            fprintf(stderr, "Error during decoding \n");
            return ret;
        }
        if (*count >= 0)
        {
            if (typ == 0)
            {
                data[*count] = (uint8_t *)malloc(frame->linesize[0] * frame->height);

                memcpy(data[*count], frame->data[0], frame->linesize[0] * frame->height);

                ++(*count);
                ++(*ptr_frm_count);
                //fprintf(stderr, " lsb count: %d, ptr_frm_count: %d, timstamp: %" PRId64 " \n", *count, *ptr_frm_count, frame->best_effort_timestamp);
                //fprintf(stderr, " >>> lsb (*ptr_frm_count): %d, *count %d\n", (*ptr_frm_count), *count);
            }
            else if (typ == 1)
            {
                data[*count] = (uint8_t *)malloc(frame->linesize[2] * frame->height); //2 is the red channel

                memcpy(data[*count], frame->data[2], frame->linesize[2] * frame->height);

                // std::copy(frame->data[0], frame->data[0] + frame->linesize[0] * frame->height, std::back_inserter(msb_buf[*count]));
                ++(*count);
                ++(*ptr_frm_count);
                //fprintf(stderr, " msb count: %d, ptr_frm_count: %d, timstamp: %" PRId64 " \n", *count, *ptr_frm_count, frame->best_effort_timestamp);

            }
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
    int *pt_y = NULL, *pt_x = NULL;
    int *pt_lsb_frm_count = NULL, *pt_msb_frm_count = NULL;
    int i = 0, j = 0, y = 0, x = 0, frm_count_lsb = 0, frm_count_msb = 0;
    int num_frames_lsb = 0, num_frames_msb = 0;
    std::string ffprobe_cmd, ffprobe_cmd2;
    uint8_t *lsb_frame_buf[FRM_GROUP_SIZE] = {NULL};
    uint8_t *msb_frame_buf[FRM_GROUP_SIZE] = {NULL};
    AVFrame *frame_in = NULL;
    pt_x = &x;
    pt_y = &y;
    pt_lsb_frm_count = &frm_count_lsb;
    pt_msb_frm_count = &frm_count_msb;
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s  lsb_file msb_file \n"
                        "API example program to show how to read frames from an input file.\n"
                        "This program reads frames from a file, decodes them, and compares them\n",
                argv[0]);
        exit(1);
    }
    src_filename = argv[1];
    src_filename_msb = argv[2];
    //video_dst_filename = argv[3];
    
    ffprobe_cmd = "ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 " + std::string(src_filename);
    ffprobe_cmd2 =  "ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 " + std::string(src_filename_msb);
    num_frames_lsb = exec_ffprobe(ffprobe_cmd);
    num_frames_msb = exec_ffprobe(ffprobe_cmd2);
    printf("num_frames_lsb: %d, num_frames_msb: %d\n", num_frames_lsb, num_frames_msb);
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

        // //video_dst_file = fopen(video_dst_filename, "wb");
        // if (!video_dst_file)
        // {
        //     fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
        //     ret = 1;
        //     goto end;
        // }

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
        // if (!video_dst_file)
        // {
        //     fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
        //     ret = 1;
        //     goto end;
        // }

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

    // if (video_stream)
    // {
    //     printf("Demuxing video from file '%s' into '%s'\n", src_filename, video_dst_filename);
    // }

    // if (video_stream_msb)
    // {
    //     printf("Demuxing msb video from file '%s' into '%s'\n", src_filename_msb, video_dst_filename);
    // }
    /* read frames from the file */
    // int k = 0;
    while (*pt_lsb_frm_count < num_frames_lsb && *pt_msb_frm_count < num_frames_msb)
    {
        while (*pt_x < FRM_GROUP_SIZE)
        {
            // check if the packet belongs to a stream we are interested in, otherwise
            // skip it
            if (av_read_frame(fmt_ctx, pkt) >= 0)
            {
                if (pkt->stream_index == video_stream_idx)
                {

                    ret = decode_packet(video_dec_ctx, pkt, frame_in, 0, pt_x, lsb_frame_buf, pt_lsb_frm_count);
                }

                av_packet_unref(pkt);

                if (ret < 0)
                {
                    fprintf(stderr, "Error while called packet\n");
                    break;
                }
            }
            else
            {
                fprintf(stderr, "Error 2 while called packet\n");
                break;
            }
        }



        *pt_x = 0;
        
        while (*pt_y < FRM_GROUP_SIZE)
        {

            if (av_read_frame(fmt_ctx_msb, pkt_msb) >= 0)
            {
                // check if the packet belongs to a stream we are interested in, otherwise
                // skip it
                if (pkt_msb->stream_index == video_stream_idx_msb)
                    ret = decode_packet(video_dec_ctx_msb, pkt_msb, frame_msb, 1, pt_y, msb_frame_buf, pt_msb_frm_count);

                av_packet_unref(pkt_msb);
                if (ret < 0){
                    fprintf(stderr, "Error 3 while called packet\n");
                    break;
                }
                // printf("%d\n", ++k);
            }
            else
            {
                fprintf(stderr, "Error 4 while called packet\n");
                //ret = -1
                break;
            }
        }
        *pt_y = 0;
        for (i = 0; i < FRM_GROUP_SIZE; ++i)
        {
            if (lsb_frame_buf[i] != NULL && msb_frame_buf[i] != NULL){
                output_both_buffs(lsb_frame_buf[i], msb_frame_buf[i]);
            }
            if (lsb_frame_buf[i] != NULL)
            {
                free(lsb_frame_buf[i]);
                lsb_frame_buf[i] = NULL;
            }
            if (msb_frame_buf[i] != NULL)
            {
                free(msb_frame_buf[i]);
                msb_frame_buf[i] = NULL;
            }
        }
        // fprintf(stderr, "hi\n");
        //  while (i < 10 && lst_lsb[i] != NULL && lst_msb[i] != NULL)
        //  {
        //      ret = output_both_frames(lst_lsb[i], lst_msb[i]);
        //      av_frame_unref(lst_lsb[i]);
        //      av_frame_unref(lst_msb[i]);
        //      lst_lsb[i] = NULL;
        //      lst_msb[i] = NULL;
        //      ++i;
        //  }
    }

    *pt_x = -1;
    *pt_y = -1;

    if (video_dec_ctx)
    {
        decode_packet(video_dec_ctx, NULL, frame_in, 0, pt_x, lsb_frame_buf, pt_lsb_frm_count);
    }
    if (video_dec_ctx_msb)
    {
        decode_packet(video_dec_ctx_msb, NULL, frame_msb, 1, pt_y, msb_frame_buf, pt_msb_frm_count);
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
    std::cout << "Average PSNR: " << getAverage(psnr_vector) << std::endl;
    if (video_stream)
    {
        printf("Play the output video file with the command:\n"
               "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d\n",
               av_get_pix_fmt_name(pix_fmt), width, height);
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
#if __has_include(<opencv2/opencv.hpp>)
    cv::destroyAllWindows();
#endif
    return ret < 0;
}
