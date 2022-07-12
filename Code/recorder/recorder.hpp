#include <bits/stdc++.h>
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
#define STREAM_FRAME_RATE 30                  /* 25 images/s */
#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P10LE 
    /**
     * @brief OutputStream
     *
     */
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

    /**
     * @brief Encodes a frame of video and stores it in a packet.
     *
     * @param fmt_ctx  The format context.
     * @param c       The codec context.
     * @param st      The stream.
     * @param frame   The frame to encode.
     * @param pkt     The packet to store the encoded frame in.
     * @return int    0 on success, negative on error.
     */
     int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                           AVStream *st, AVFrame *frame, AVPacket *pkt);

    /**
     * @brief Initializes the output stream.
     * #ifndef MULTICAM_MAIN_H
#define MULTICAM_MAIN_H
#ifdef __cplusplus
     * @param crf  The CRF to use.
     */
    void add_stream(OutputStream *ost, AVFormatContext *oc,
                           const AVCodec **codec,
                           enum AVCodecID codec_id, const char *crf);

    /**
     * @brief Allocates a picture and sets its fields to default values.
     *
     * @param pix_fmt  The pixel format.
     * @param width    The width of the frame.
     * @param height   The height of the frame.
     * @return AVFrame*  The frame.
     */
     AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);

    /**
     * @brief Sets values for the output stream and codec values as well as any additional arguments.
     *
     * @param oc  The output context.
     * @param codec  The codec to use.
     * @param ost  The output stream.
     * @param opt_arg  The additional arguments.
     */
    void open_video(AVFormatContext *oc, const AVCodec *codec,
                           OutputStream *ost, AVDictionary *opt_arg);

    /**
     * @brief Get the video frame object
     *
     * @param ost The output stream.
     * @param time_run How long the video has been running.
     * @param data The data to use.
     * @return AVFrame*  The frame.
     */
    AVFrame *get_video_frame(OutputStream *ost, long time_run, uint8_t *data);

    /**
     * @brief Write the video frame using the encoding feature as well as checking for writeable frame.
     *
     * @param oc  The output context.
     * @param ost  The output stream.
     * @param data  The uint8_t data buffer to use for the frame->data[0] field.
     * @param time_run  How long the video has been running.
     * @return int 0 on success, negative on error.
     */
     int write_video_frame(AVFormatContext *oc, OutputStream *ost, uint8_t *data, long time_run);
    /**
     * @brief Free all stream related resources.
     *
     * @param oc The output context.
     * @param ost The output stream.
     */
     void close_stream(AVFormatContext *oc, OutputStream *ost);
}

/**
 * @brief Class Parser for the command line arguments.
 * 
 */
class InputParser
{
public:
    InputParser(int &argc, char **argv);

    const std::string &getCmdOption(const std::string &option) const;

    bool cmdOptionExists(const std::string &option) const;
private:
    std::vector<std::string> tokens;
};

/**
 * @brief print_usage statement
 * @param   choice: prints a specific usage statement given a value
 */
void print_usage(std::string choice = "");

/**
 * @brief Takes in an input parser as long as the command is a number
 * @param input
 * @param cmd
 * @param prev_val
 * @return int--- Returns -5 if no command successful
 */
int parse_integer_cmd(InputParser &input, std::string cmd, int prev_val);


/**
 * @brief Checks if a file exists or not
 *
 * @param path
 * @return bool
 *
 */
bool does_file_exist(std::string path);

/**
 * @brief Checks if a directory path exists
 *
 * @param path
 * @return true
 * @return false
 */
bool does_dir_exist(std::string path);