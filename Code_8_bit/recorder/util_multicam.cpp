#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#endif

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/rs_advanced_mode.hpp>

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/detail/cmdline.hpp>

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
#include <random>

#include "recorder.hpp" // My own header file

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
#include <libavutil/buffer.h>
#include <libavutil/frame.h>
#include <libavcodec/avfft.h>
#include <libavformat/avio.h>

    //#define STREAM_DURATION 10.0
    /* default pix_fmt */

    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
    {
        AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

        // printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
        //        av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
        //        av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
        //        av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
        //        pkt->stream_index);
    }

    int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
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
    void add_stream(OutputStream *ost, AVFormatContext *oc,
                    const AVCodec **codec,
                    enum AVCodecID codec_id, const char *crf, AVPixelFormat pix_fmt, int width, int height, int fps)
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
        // printf("hih\n");
        c->codec_id = codec_id;
        // c->flags = AV_CODEC_FLAG_GRAY;
        // c->bit_rate = 1 * 1000000;

        /* Resolution must be a multiple of two. */
        c->width = width;
        c->height = height;
        c->thread_count = 4;

        // av_opt_set(c->priv_data, "crf", "24", 0);
        //  av_opt_set(c->priv_data, "
        //  c->active_thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        // ost->st->r_frame_rate =
        ost->st->time_base = (AVRational){1, fps};
        // ost->st->nb_frames = 450;
        c->time_base = ost->st->time_base;
        c->framerate = (AVRational){fps, 1};
        // c->noise_reduction = 1;
        // c->bits_per_raw_sample = 10;
        // c->bit_rate = (80 * 8 * 10000000) / 60;
        //  c->field_order = AV_FIELD_UNKNOWN;
        //  c->field_order = AV_F;
        //  c->frame_size =
        //   c->rc_min_rate
        //   c->gop_size = 5; /* emit one intra frame every twelve frames at most */
        //   c->max_b_frames = 3;
        c->pix_fmt = pix_fmt;
        // c->chromaoffest = 5;
        if (c->codec_id == AV_CODEC_ID_H264)
        {

            /* just for testing, we also add B frames */
            av_opt_set(c->priv_data, "preset", "veryfast", 0);
            av_opt_set(c->priv_data, "noise_reduction", "20", 0); // 45

            av_opt_set(c->priv_data, "crf", crf, 0);

            // av_opt_set(c->priv_data, "x264-params", "qpmin=0:qpmax=59:chroma_qp_offset=-2", 0);
            av_opt_set(c->priv_data, "x264-params", "qpmin=0:qpmax=59", 0);
            if (pix_fmt == AV_PIX_FMT_YUV420P)
            {
                av_opt_set(c->priv_data, "x264-params", "profile=baseline:level=3.0", 0);
                av_opt_set(c->priv_data, "x264-params", "bframes=3:ref=3:scenecut=0", 0);
                // av_opt_set(c->priv_data, "color_range", "pc", 0);
            }
        }
        else if (c->codec_id == AV_CODEC_ID_HEVC)
        {
            av_opt_set(c->priv_data, "preset", "ultrafast", 0);
            av_opt_set(c->priv_data, "crf", crf, 0);
            av_opt_set(c->priv_data, "tune", "zerolatency", 0);
            av_opt_set(c->priv_data, "x265-params", "qpmin=0:qpmax=59", 0);
        }
        else
        {
            fprintf(stderr, "Could not find encoder for '%s'\n",
                    avcodec_get_name(codec_id));
            exit(1);
        }

        /* Some formats want stream headers to be separate. */
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        {
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
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
        ret = av_frame_get_buffer(picture, 0);
        if (ret < 0)
        {
            fprintf(stderr, "Could not allocate frame data.\n");
            exit(1);
        }

        return picture;
    }

    void open_video(AVFormatContext *oc, const AVCodec *codec,
                    OutputStream *ost, AVDictionary *opt_arg, AVPixelFormat pix_fmt)
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
        ost->frame = alloc_picture(pix_fmt, c->width, c->height);

        if (!ost->frame)
        {
            fprintf(stderr, "Could not allocate video frame\n");
            exit(1);
        }

        // ost->tmp_frame = NULL;
        /* copy the stream parameters to the muxer */
        ret = avcodec_parameters_from_context(ost->st->codecpar, c);
        if (ret < 0)
        {
            fprintf(stderr, "Could not copy the stream parameters\n");
            exit(1);
        }
    }

    AVFrame *get_video_frame(OutputStream *ost, uint8_t *data)
    {
        AVCodecContext *c = ost->enc; // 64 + x + i * 5

        // /* check if we want to generate more frames */
        // if (av_compare_ts(ost->next_pts, c->time_base,(int64_t)time_run, (AVRational){1, 1}) > 0){
        //     return NULL;
        // }
        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally; make sure we do not overwrite it here */
        if (av_frame_make_writable(ost->frame) < 0)
        {
            fprintf(stderr, "Frame not writable\n");
            exit(1);
        }
        // printf("HIeeeee\n");
        // fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height, data);
        ost->frame->data[0] = data;

        ost->frame->pts = ost->next_pts++;

        return ost->frame;
    }

    /*
     * encode one video frame and send it to the muxer
     * return 1 when encoding is finished, 0 otherwise
     */
    int write_video_frame(AVFormatContext *oc, OutputStream *ost, uint8_t *data)
    {
        return write_frame(oc, ost->enc, ost->st, get_video_frame(ost, data), ost->tmp_pkt);
    }

    void close_stream(AVFormatContext *oc, OutputStream *ost)
    {
        avcodec_free_context(&ost->enc);
        av_frame_free(&ost->frame);
        // av_frame_free(&ost->tmp_frame);
        av_packet_free(&ost->tmp_pkt);
        sws_freeContext(ost->sws_ctx);
        swr_free(&ost->swr_ctx);
    }
}

InputParser::InputParser(int &argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        this->tokens.push_back(std::string(argv[i]));
    }
}

const std::string &InputParser::getCmdOption(const std::string &option) const
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

bool InputParser::cmdOptionExists(const std::string &option) const
{
    return this->tokens.end() != std::find(this->tokens.begin(), this->tokens.end(), option);
}

/**
 * @brief print_usage statement
 * @param   choice: prints a specific usage statement given a value
 */
void print_usage(std::string choice)
{
    if (choice == "-ir")
    {
        std::cout << "Usage: -ir <int> 0 or 1. 0 for no IR, 1 for IR" << std::endl;
    }
    else if (choice == "-align_to_color")
    {
        std::cout << "Usage: -align_to_color <int> 0 or 1. 0 for no alignment and regular recording, 1 for depth alignment to color, NOTE High Processing COST" << std::endl;
    }
    else if (choice == "-disp_shift")
    {
        std::cout << "Usage -disp_shift <0-255>  Disparity Shifty larger value for objects closer to camera." << std::endl;
    }
    else if (choice == "-sv_addr")
    {
        std::cout << "Usage:  -sv_addr <server_address> valid ip address " << std::endl;
    }
    else if (choice == "-port")
    {
        std::cout << "Usage: -port <port number> between 1024 and 65535" << std::endl;
    }
    else if (choice == "-depth_unit")
    {
        std::cout << "Usage: ./depth_unit -depth_unit <int>  Depth units represent the measurement length of a single depth step. A Depth step is difference between each discrete depth value in a depth image." << std::endl;
        std::cout << "For example if my depth unit is 1000 then each difference in 1 mm in the image will result in one value difference in the actual depth values." << std::endl;
        std::cout << "To obtain back the true depth distance one needs to use the following formula: for each i in depth array: dist = depth_unit * depth_value[i] , where the dist is measured in micrometers" << std::endl;
        std::cout << "1 depth_unit is about a micrometer, so 1000 micrometers is the default depth resolution (~ 1 mm). " << std::endl;
        std::cout << "Max depth unit is 100000 micrometers" << std::endl;
        std::cout << "Min viable depth unit is 40 micrometers" << std::endl;
    }
    else if (choice == "-min_depth")
    {
        std::cout << "Usage: -min_depth <int> depth diff of 1023 or lower depth you will only be using one stream " << std::endl;
    }
    else if (choice == "-max_depth")
    {
        std::cout << "Usage: -max_depth <int> depth diff of 1023 or lower depth you will only be using one stream " << std::endl;
    }
    else if (choice == "-depth_lossless")
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
        std::cout << "Usage: -max_depth <int> max_depth default is 65535 " << std::endl;
        std::cout << "Usage: -min_depth <int> min_depth default is 0 " << std::endl;
        std::cout << "Usage: -depth_unit <int>  Depth units represent the measurement length of a single depth step. A Depth step is difference between each discrete depth value in a depth image." << std::endl;
        std::cout << "For example if my depth unit is 1000 then each difference in 1 mm in the image will result in one value difference in the actual depth values." << std::endl;
        std::cout << "To obtain back the true depth distance one needs to use the following formula: for each i in depth array: dist = depth_unit * depth_value[i] , where the dist is measured in micrometers" << std::endl;
        std::cout << "1 depth_unit is about a micrometer, so 1000 micrometers is the default depth resolution (~ 1 mm). " << std::endl;
        std::cout << "Max depth unit is 100000 micrometers" << std::endl;
        std::cout << "Min viable depth unit is 40 micrometers" << std::endl;
        std::cout << "Usage: -port <port number> between 1024 and 65535" << std::endl;
        std::cout << "Usage:  -sv_addr <server_address> valid ip address " << std::endl;
        std::cout << "Usage -disp_shift <0-255>  Disparity Shifty larger value for objects closer to camera." << std::endl;
        std::cout << "Usage: -align_to_color <int> 0 or 1. 0 for no alignment and regular recording, 1 for depth alignment to color, NOTE High Processing COST" << std::endl;
        // std::cout << "-rosbag [int: 0 or 1]\tWhether to save color .bag file and not compress" << std::endl;
    }
}

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