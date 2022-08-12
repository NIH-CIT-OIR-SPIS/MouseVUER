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
#include <bitset>

#include <filesystem> // C++17
#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#endif

// #include <boost/program_options/cmdline.hpp>
// #include <boost/program_options/options_description.hpp>
// #include <boost/program_options/detail/cmdline.hpp>
// // #include <boost/program_options/detail/config_file.hpp>
// // #include <boost/program_options/parsers.hpp>
// #include <boost/program_options/variables_map.hpp>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <sys/stat.h>
//#define frm_group_size 3
#define W 1280
#define H 720
#define MAXN 100001
}

static int frm_group_size = 3;
static int print_psnr = 1;
const int shift_by = 2; // 0;
static const int shift_back_by = 16 - (8 - shift_by);

namespace buff_global
{
    int video_frame_count = 0;
    uint16_t store_depth[1280 * 720] = {0};
    // uint16_t store_depth_lsb[1280 * 720] = {0};
    uint16_t store_raw_depth[1280 * 720] = {0};
    // uint16_t store_raw_depth_lsb[1280 * 720] = {0};
    std::vector<double> psnr_vector;
    std::string input_raw_dir_gl = "";
    std::string output_dir_gl = "";
}

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
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->tokens.begin(), this->tokens.end(), option);
        return itr != this->tokens.end();
    }

private:
    std::vector<std::string> tokens;
};

// InputParser::InputParser(int &argc, char **argv)
// {
//     for (int i = 1; i < argc; ++i)
//     {
//         this->tokens.push_back(std::string(argv[i]));
//     }
// }

// const std::string &InputParser::getCmdOption(const std::string &option) const
// {
//     std::vector<std::string>::const_iterator itr;
//     itr = std::find(this->tokens.begin(), this->tokens.end(), option);
//     if (itr != this->tokens.end() && ++itr != this->tokens.end())
//     {
//         return *itr;
//     }
//     static const std::string empty_string("");
//     return empty_string;
// }

// bool InputParser::cmdOptionExists(const std::string &option) const
// {
//     return this->tokens.end() != std::find(this->tokens.begin(), this->tokens.end(), option);
// }

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
    struct stat hd;
    const char *pathname = path.c_str();
    if (stat(pathname, &hd) != 0)
    {
        return false;
    }
    else if (hd.st_mode & S_IFDIR)
    {
        return true;
    }
    else
    {
        return false;
    }
}
static int exec_ffprobe(std::string str_cmd)
{
    const char *cmd = str_cmd.c_str();
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return atoi(result.c_str());
}
void print_usage(std::string choice = "")
{

    if (choice == "" || choice == "-help" || choice == "-h")
    {
        std::cout << "Usage: " << std::endl;
        std::cout << " [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << " -ilsb <input_file>  : required Input lsb video file" << std::endl;
        std::cout << " -imsb <input_file>  : optional Input msb video file" << std::endl;
        std::cout << " -hd <header txt file> : required Header file will be named something like video_header.txt" << std::endl;
        std::cout << " -o <directory name>    : required Output directory for decompressed files " << std::endl;
        std::cout << " -cmp <raw_file_dir> : optional Input raw file directory" << std::endl;
        std::cout << " -help                  : Print this help" << std::endl;
        std::cout << " -print_psnr <input_file> : Print psnr all " << std::endl;
        std::cout << " -sz <frame_group_size> : optional frame group size" << std::endl;
    }
    else if (choice == "-print_psnr")
    {
        std::cout << "Usage: " << std::endl;
        std::cout << " -print_psnr <INT> 0 or 1" << std::endl;
    }
    else if (choice == "-sz")
    {
        std::cout << "Usage: " << std::endl;
        std::cout << "  -sz <frame_group_size>" << std::endl;
    }
    else if (choice == "-hd")
    {
        std::cout << " -hd <header txt file> : required Header file" << std::endl;
    }
    else if (choice == "-ilsb")
    {
        std::cout << " -ilsb <input_file>  : required Input lsb video file" << std::endl;
    }
    else if (choice == "-imsb")
    {
        std::cout << " -imsb <input_file>  : optional Input msb video file" << std::endl;
    }
    else if (choice == "-o")
    {
        std::cout << " -o <directory name>    : required Output directory for decompressed files " << std::endl;
    }
    else if (choice == "-cmp")
    {
        std::cout << " -cmp <raw_file_dir> : optional Input raw file directory, IE where all the raw files lie, for developer only" << std::endl;
    }
    else
    {
        std::cout << "Usage: " << std::endl;
        std::cout << " [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << " -ilsb <input_file>  : required Input lsb video file" << std::endl;
        std::cout << " -imsb <input_file>  : optional Input msb video file" << std::endl;
        std::cout << " -hd <header txt file> : required Header file" << std::endl;
        std::cout << " -o <directory name>    : required Output directory for decompressed files " << std::endl;
        std::cout << " -cmp <raw_file_dir> : optional Input raw file directory" << std::endl;
        std::cout << " -help                  : Print this help" << std::endl;
        std::cout << " -print_psnr <input_file> : Print psnr all " << std::endl;
        std::cout << " -sz <frame_group_size> : optional frame group size" << std::endl;
    }
}

template <typename T>
double getAverage(std::vector<T> const &v)
{
    if (v.empty())
    {
        return 0;
    }

    double sum = 0.0;
    for (const T &i : v)
    {
        sum += (double)i;
    }
    return sum / v.size();
}

static inline int abs_diff(int a, int b)
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

double img_mean(uint16_t *arr, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; ++i)
    {
        sum += (double)arr[i];
    }
    return sum / (double)n;
}

double img_variance(uint16_t *arr, int n, double mean = -1.0)
{
    if (mean < 0.0)
    {
        mean = img_mean(arr, n);
    }
    double vari = 0.0;
    for (int i = 0; i < n; ++i)
    {
        vari += ((double)arr[i] - mean) * ((double)arr[i] - mean);
    }
    return vari / (double)n;
}

double img_covariance(uint16_t *compressed, uint16_t *raw, int n, double mean_comp = -1.0, double mean_raw = -1.0)
{
    double sum_all = 0.0;
    if (mean_comp < 0.0)
    {
        mean_comp = img_mean(compressed, n);
    }
    if (mean_raw < 0.0)
    {
        mean_raw = img_mean(raw, n);
    }

    for (int i = 0; i < n; ++i)
    {
        sum_all += ((double)compressed[i] - mean_comp) * ((double)raw[i] - mean_raw);
    }
    return (sum_all) / (double)(n - 1);
}
double get_ssim(uint16_t *compressed, uint16_t *raw, int siz = 0, int pr_lum = 0, int pr_contr = 0, int pr_struct = 0)
{
    if (siz <= 0)
    {
        siz = H * W;
    }
    double k1 = 0.01;
    double k2 = 0.03;
    double dyn_range = 65535.0;
    double c1 = (k1 * dyn_range) * (k1 * dyn_range);
    double c2 = (k2 * dyn_range) * (k2 * dyn_range);
    double c3 = c2 / 2;
    double mean_compr = img_mean(compressed, siz);
    double mean_raw = img_mean(raw, siz);
    double var_compr = img_variance(compressed, siz, mean_compr);
    double var_raw = img_variance(raw, siz, mean_raw);
    double covar = img_covariance(compressed, raw, siz, mean_compr, mean_raw);
    if (pr_lum){
        double lum = (2 * mean_compr * mean_raw + c1) / ( mean_compr * mean_compr + mean_raw * mean_raw + c1);
    
        printf("Luminance SSIM: %f\n", lum);
    }
    if (pr_contr || pr_struct){
        double std_var1 = sqrt(var_compr);
        double std_var_raw = sqrt(var_raw);
        if(pr_contr){
            printf("Contrast SSIM: %f\n", ( (2*std_var1*std_var_raw + c2) / (var_compr + var_raw + c2) ));
        }
        if(pr_struct){
            printf("Structure SSIM: %f\n", ( (covar + c3) / (std_var1 * std_var_raw + c3) ));
        }
    }

    return ((2 * mean_compr * mean_raw + c1) * (2 * covar + c2)) / ((mean_compr * mean_compr + mean_raw * mean_raw + c1) * (var_compr + var_raw + c2));
}
static double get_psnr(uint16_t *m0, uint16_t *m1)
{
    long cg = 65535U * 65535U;
    uint sum_sq = 0;
    double mse = 0;
    //#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < H * W; ++i)
    {
        int p1 = m0[i];
        int p2 = m1[i];
        int err = abs_diff(p2, p1);
        sum_sq += (err * err);
    }
    // res = res / (height * width);

    mse = (double)sum_sq / (H * W);
    return (10.0 * log10(cg / mse));
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
        // fprintf(stderr, "Could not find %s stream in input file '%s'\n",
        //         av_get_media_type_string(type), src_filename);
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

static void read_max_min(int *max, int *min, int *depth_units)
{
    std::ifstream infile("Testing_DIR/video_head_file.txt");
    // fprintf(stderr, "Reading header file\n");
    int a, b, c;
    while (infile >> a >> b >> c)
    {
        *max = a;
        *min = b;
        *depth_units = c;
    }
    printf("Max %d Min %d Depth_units in micrometers: %d\n", *max, *min, *depth_units);
    infile.close();
}

static int output_both_buffs(uint8_t *frame_lsb, uint8_t *frame_msb, int max_d, int min_d)
{
    using namespace buff_global;
    int i = 0, y = 0, count = 0;
    // int count2 = 0;
    double psnr_val = 0;
    double ssim_val = 0;
    // double psnr_val_6_bit = 0;
    uint16_t max = 0;
    uint16_t curr_lsb = 0;
    uint16_t store_num = 0;
    uint16_t curr_msb = 0;
    FILE *out_write = NULL;

    FILE *read_raw = NULL;
    std::string raw_str = input_raw_dir_gl + "/frame_num_" + std::to_string(video_frame_count) + "_data.bin";
    std::string outer = output_dir_gl + "/file_out_comp" + std::to_string(video_frame_count) + ".bin";

    if (!(out_write = fopen(outer.c_str(), "wb")))
    {
        printf("Could not open file for writing\n");
        return -1;
    }

    if (!(read_raw = fopen(raw_str.c_str(), "rb")))
    {
        if (print_psnr)
        {
            printf("No raw files to read not compressing\n");
        }
    }
    else
    {
        fread(store_raw_depth, sizeof(uint16_t), H * W, read_raw);
    }
    if (max_d - min_d <= 1023 || frame_msb == NULL)
    {
        for (i = 0, y = 0; i < H * W * 2; i += 2, ++y)
        {
            store_num = (uint16_t)frame_lsb[i] | (((uint16_t)frame_lsb[i + 1]) << 8);

            store_num += min_d;
            store_depth[y] = store_num;
            // store_depth_lsb[y] = store_num;
        }
    }
    else
    {
        for (i = 0, y = 0; i < H * W * 2; i += 2, ++y)
        {

            curr_lsb = ((uint16_t)frame_lsb[i] | (((uint16_t)frame_lsb[i + 1]) << 8));

            curr_msb = ((uint16_t)frame_msb[y]) << shift_back_by; // 10;

            store_num = curr_lsb | curr_msb;

            store_depth[y] = store_num;
            // store_depth_lsb[y] = curr_lsb;
        }
    }
    // if (video_frame_count == 200)
    // {
    //     lineRead(store_depth_lsb);
    // }

    // loop_image_patch(store_depth, W, H); //Will utilize later
    if (read_raw != NULL)
    {
        psnr_val = get_psnr(store_depth, store_raw_depth);
        if (print_psnr)
        {
            printf("PSNR: %f\n", psnr_val);
        }
        // ssim_val = get_ssim(store_depth, store_raw_depth, H*W, 1, 1, 1);
        // printf("SSIM: %f\n", ssim_val);
        psnr_vector.push_back(psnr_val);
    }
    else
    {
        if (psnr_vector.empty())
        {
            psnr_vector.push_back(0);
        }
    }
    // psnr_val = get_psnr(store_depth, store_raw_depth);
    // // psnr_val_6_bit = get_psnr_6bit(store_depth_lsb , store_raw_depth_lsb);
    // // printf("PSNR value 6 bit = %f\n", psnr_val_6_bit);
    // printf("PSNR value = %f\n", psnr_val);
    ++video_frame_count;
    if (print_psnr)
    {
#if __has_include(<opencv2/opencv.hpp>)

        cv::Mat dec_img_color(cv::Size(W, H), CV_8UC3);
        dec_img_color = 0;
        cv::Mat dec_img(cv::Size(W, H), CV_16U, store_depth, cv::Mat::AUTO_STEP);
        cv::Mat raw_img(cv::Size(W, H), CV_16U, store_raw_depth, cv::Mat::AUTO_STEP);

        cv::normalize(dec_img, dec_img, 0, 65535, cv::NORM_MINMAX);

        float alpha = 1.0 / 255;
        // alpha = 0.7f;

        // cv::convertScaleAbs(dec_img, dec_img_color, alpha);
        // cv::convertScaleAbs(raw_img, raw_img_color, alpha);
        dec_img.convertTo(dec_img_color, CV_8U, alpha);

        /// dec_img.convertTo()

        // uint8_t *ptr_dec_img_color = dec_img_color.data;
        // psnr_val = get_psnr(store_depth, store_raw_depth);
        // printf("Get other PSNR value = %f\n", psnr_val);
        // raw_img.convertTo(raw_img, CV_8U, 0.7);
        cv::applyColorMap(dec_img_color, dec_img_color, 9);

        if (video_frame_count == 30)
        {
            cv::imwrite("Testing_DIR/dec_img_30.png", dec_img_color);
        }

        if (read_raw != NULL)
        {
            cv::Mat raw_img_color(cv::Size(W, H), CV_8UC3);
            raw_img_color = 0;
            cv::normalize(raw_img, raw_img, 0, 65535, cv::NORM_MINMAX);
            raw_img.convertTo(raw_img_color, CV_8U, alpha);
            cv::applyColorMap(raw_img_color, raw_img_color, 9);
            cv::namedWindow("Raw Image", cv::WINDOW_AUTOSIZE);
            cv::imshow("Raw Image", raw_img_color);
            if (video_frame_count == 30)
            {
                cv::imwrite("Testing_DIR/raw_img_30.png", raw_img_color);
            }
        }

        cv::namedWindow("Decompressed Image", cv::WINDOW_AUTOSIZE);
        cv::imshow("Decompressed Image", dec_img_color);
        if ((char)cv::waitKey(25) == 27) //
        {
            cv::destroyAllWindows();
            return -6;
        }
        // 10872 vs 637

#endif
    }
    fwrite(store_depth, sizeof(uint16_t), H * W, out_write);
    fclose(out_write);
    // printf("max: %u\n", max);
    // fprintf(stderr, "video_frame_count: %d\n", video_frame_count);

    // fprintf(stderr, "lsb_frame pts_n: %" PRId64 " time_stamp: %" PRId64 " \n", frame_lsb->pts, frame_lsb->best_effort_timestamp);
    // fprintf(stderr, "msb_frame pts_n: %" PRId64 " time_stamp: %" PRId64 " \n", frame_msb->pts, frame_msb->best_effort_timestamp);

    return 0;
}

// static int unpack_bframes(AVCodecContext *dec, const AVPacket *pkt, AVFrame *frame, int typ, int *count, uint8_t *data[10],
//                           int *ptr_frm_count, int debug_flag)
// {
//     /* Decode B-frames regardless of pts */

// }
static int decode_packet(AVCodecContext *dec, const AVPacket *pkt, AVFrame *frame, int typ, int *count, uint8_t *data[10],
                         int *ptr_frm_count, int debug_flag)
{
    int ret = 0;
    int channel = 0;
    // submit the packet to the decoder
    // dec->frame_skip_threshold =
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
                if (debug_flag == 1)
                {
                    // AVPictureType pict_type = av_frame_get_pict_type(frame);
                    // AVPictureType pict_type = frame->pict_type;
                    // printf("Picture frame->pict_type = %d\n", frame->pict_type);

                    printf("wq *count: %d ret %d \n", *count, ret);
                }

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
                // printf("frame->linesize[0] : %d\n", frame->linesize[0]);
                memcpy(data[*count], frame->data[0], frame->linesize[0] * frame->height);

                ++(*count);
                ++(*ptr_frm_count);
                // fprintf(stderr, " lsb count: %d, ptr_frm_count: %d, timstamp: %" PRId64 " \n", *count, *ptr_frm_count, frame->best_effort_timestamp);
                // fprintf(stderr, " >>> lsb (*ptr_frm_count): %d, *count %d\n", (*ptr_frm_count), *count);
            }
            else if (typ == 1)
            {

                data[*count] = (uint8_t *)malloc(frame->linesize[channel] * frame->height); // 2 is the red channel
                // printf("frame->linesize[2]: %d\n", frame->linesize[2]);
                memcpy(data[*count], frame->data[channel], frame->linesize[channel] * frame->height);

                // std::copy(frame->data[0], frame->data[0] + frame->linesize[0] * frame->height, std::back_inserter(msb_buf[*count]));
                ++(*count);
                ++(*ptr_frm_count);
                // fprintf(stderr, " msb count: %d, ptr_frm_count: %d, timstamp: %" PRId64 " \n", *count, *ptr_frm_count, frame->best_effort_timestamp);
            }
        }

        if (debug_flag == 1)
        {
            printf("type: %d *count: %d\n", typ, *count);
        }
        av_frame_unref(frame);
        if (ret < 0)
        {
            return ret;
        }
    }

    return 0;
}

// void sieve(int spf[MAXN])
// {
//     spf[1] = 1;
//     for (int i=2; i<MAXN; i++){

//         // marking smallest prime factor for every
//         // number to be itself.
//         spf[i] = i;
//     }

//     // separately marking spf for every even
//     // number as 2
//     for (int i=4; i<MAXN; i+=2) {
//         spf[i] = 2;
//     }
//     for (int i=3; i*i<MAXN; i++)
//     {
//         // checking if i is prime
//         if (spf[i] == i)
//         {
//             // marking SPF for all numbers divisible by i
//             for (int j=i*i; j<MAXN; j+=i) {

//                 // marking spf[j] if it is not
//                 // previously marked
//                 if (spf[j]==j) {
//                     spf[j] = i;
//                 }
//             }
//         }
//     }
// }

// int smallest_prime_factor(int x, int spf[MAXN])
// {
//     std::vector<int> ret;
//     while (x != 1)
//     {
//         ret.push_back(spf[x]);
//         x = x / spf[x];
//     }
//     //find minimum value in the vector ret

//     int min = ret[0];
//     for (int i = 1; i < ret.size(); i++)
//     {
//         if (ret[i] < min)
//         {
//             min = ret[i];
//         }
//     }

//     return min;
//     //return ret;
// }

static int decompress(int max_d, int min_d, int depth_units, int num_frames_lsb, int num_frames_msb,
                      std::string output_dir, std::string input_file_lsb, std::string input_file_msb, std::string input_raw_file_dir)
{

    int ret = 0;
    int i = 0, j = 0, y = 0, x = 0, frm_count_lsb = 0, frm_count_msb = 0;
    int *pt_y = NULL, *pt_x = NULL;
    int *pt_lsb_frm_count = NULL, *pt_msb_frm_count = NULL;
    const char *src_filename = input_file_lsb.c_str();
    char *src_filename_msb = NULL;
    bool take_msb = true;
    AVFormatContext *fmt_ctx = NULL;
    AVFormatContext *fmt_ctx_msb = NULL;
    int video_stream_idx = -1;
    int video_stream_idx_msb = -1;
    AVCodecContext *video_dec_ctx = NULL;
    AVCodecContext *video_dec_ctx_msb = NULL;
    int width_msb, height_msb;
    int width, height;
    enum AVPixelFormat pix_fmt_msb, pix_fmt;
    AVStream *video_stream = NULL;
    AVStream *video_stream_msb = NULL;

    uint8_t *video_dst_data[4] = {NULL};
    uint8_t *video_dst_data_msb[4] = {NULL};

    int video_dst_linesize[4];
    int video_dst_linesize_msb[4] = {0};
    uint8_t *lsb_frame_buf[frm_group_size] = {NULL};
    uint8_t *msb_frame_buf[frm_group_size] = {NULL};
    int video_dst_bufsize = 0;
    int video_dst_bufsize_msb = 0;
    int counte = 0;

    int lsb_frames_dec = 0;
    int msb_frames_dec = 0;
    int both = 0;
    AVPacket *pkt = NULL;
    AVPacket *pkt_msb = NULL;
    AVFrame *frame = NULL;
    AVFrame *frame_msb = NULL;
    pt_x = &x;
    pt_y = &y;
    pt_lsb_frm_count = &frm_count_lsb;
    pt_msb_frm_count = &frm_count_msb;
    namespace buff = buff_global;

    buff::input_raw_dir_gl = input_raw_file_dir;
    buff::output_dir_gl = output_dir;
    if (input_file_msb != "")
    {
        src_filename_msb = (char *)malloc(input_file_msb.size() + 1);
        strcpy(src_filename_msb, input_file_msb.c_str());
    }
    if (max_d - min_d <= 1023)
    {
        printf("max_d - min_d <= 1023\n");
        take_msb = false;
    }

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0)
    {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }
    /* open input file, and allocate format context */

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
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
        // video_dec_ctx->
        // video_dec_ctx->delay = 2;
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
        // video_dec_ctx->skip_frame = AVDISCARD_NONE;
        // video_dec_ctx->b_f
        // video_dec_ctx->flags |= CODEC_FLAG_GRAY;
    }
    if (take_msb)
    {
        if (avformat_open_input(&fmt_ctx_msb, src_filename_msb, NULL, NULL) < 0)
        {
            fprintf(stderr, "Could not open source file %s\n", src_filename_msb);
            exit(1);
        }
        /* retrieve stream information */
        if (take_msb && avformat_find_stream_info(fmt_ctx_msb, NULL) < 0)
        {
            fprintf(stderr, "Could not find stream information\n");
            exit(1);
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
            // video_dec_ctx_msb->delay = 2;
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
    }
    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }
    if (take_msb)
    {
        frame_msb = av_frame_alloc();
        if (!frame_msb)
        {
            fprintf(stderr, "Could not allocate frame\n");
            ret = AVERROR(ENOMEM);
            goto end;
        }
    }

    pkt = av_packet_alloc();
    if (!pkt)
    {
        fprintf(stderr, "Could not allocate packet\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (take_msb)
    {
        pkt_msb = av_packet_alloc();
        if (!pkt_msb)
        {
            fprintf(stderr, "Could not allocate packet\n");
            ret = AVERROR(ENOMEM);
            goto end;
        }
    }

    // for (i = 0; i < frm_group_size; ++i)
    // {
    //     lsb_frame_buf[i] = (uint8_t *)malloc(2560 *);
    //     msb_frame_buf[i] = (uint8_t *)malloc(video_dst_bufsize_msb);
    // }

    if (take_msb)
    {
        while (*pt_lsb_frm_count < num_frames_lsb || *pt_msb_frm_count < num_frames_msb)
        {
            while (*pt_x < frm_group_size)
            {
                // check if the packet belongs to a stream we are interested in, otherwise
                // skip it
                if (av_read_frame(fmt_ctx, pkt) >= 0)
                {
                    if (pkt->stream_index == video_stream_idx)
                    {

                        if (lsb_frames_dec < 5)
                        {
                            ret = decode_packet(video_dec_ctx, pkt, frame, 0, pt_x, lsb_frame_buf, pt_lsb_frm_count, 1);
                        }
                        else
                        {
                            ret = decode_packet(video_dec_ctx, pkt, frame, 0, pt_x, lsb_frame_buf, pt_lsb_frm_count, 0);
                        }
                    }

                    av_packet_unref(pkt);

                    if (ret < 0)
                    {
                        fprintf(stderr, "Error while called packet\n");
                        break;
                    }
                    ++lsb_frames_dec;
                }
                else
                {
                    fprintf(stderr, "Error 2 while called packet\n");
                    ++counte;
                    break;
                }
            }

            //*pt_x = 0;
            if (lsb_frames_dec < 5)
            {
                printf("ls_b frames dec: %d, *pt_x %d\n", lsb_frames_dec, *pt_x);
            }

            while (*pt_y < frm_group_size)
            {

                if (av_read_frame(fmt_ctx_msb, pkt_msb) >= 0)
                {
                    // check if the packet belongs to a stream we are interested in, otherwise
                    // skip it
                    if (pkt_msb->stream_index == video_stream_idx_msb)
                    {
                        ret = decode_packet(video_dec_ctx_msb, pkt_msb, frame_msb, 1, pt_y, msb_frame_buf, pt_msb_frm_count, 0);
                    }
                    av_packet_unref(pkt_msb);
                    if (ret < 0)
                    {
                        fprintf(stderr, "Error 3 while called packet\n");
                        break;
                    }
                    ++msb_frames_dec;
                    // printf("%d\n", ++k);
                }
                else
                {
                    fprintf(stderr, "Error 4 while called packet\n");
                    // ret = -1
                    ++counte;
                    break;
                }
            }

            if (msb_frames_dec != lsb_frames_dec)
            {
                fprintf(stderr, "Something wrong with number of frames on iter, msb_frames_dec = %d, lsb_frames_dec = %d, *pt_x %d *pt_y %d\n", msb_frames_dec, lsb_frames_dec, *pt_x, *pt_y);
            }

            for (i = 0; i < frm_group_size; ++i)
            {
                if (lsb_frame_buf[i] != NULL && msb_frame_buf[i] != NULL)
                {
                    output_both_buffs(lsb_frame_buf[i], msb_frame_buf[i], max_d, min_d);
                    free(lsb_frame_buf[i]);
                    free(msb_frame_buf[i]);
                    lsb_frame_buf[i] = NULL;
                    msb_frame_buf[i] = NULL;
                    ++both;
                }

                if (lsb_frame_buf[i] == NULL && msb_frame_buf[i] != NULL)
                {
                    printf("Why is this happening lsb %d, msb %d, both %d ?\n", lsb_frames_dec, msb_frames_dec, both);
                }
                // if (lsb_frame_buf[i] != NULL)
                // {
                //     free(lsb_frame_buf[i]);
                //     lsb_frame_buf[i] = NULL;
                // }
                // if (msb_frame_buf[i] != NULL)
                // {
                //     free(msb_frame_buf[i]);
                //     msb_frame_buf[i] = NULL;
                // }
            }
            *pt_y = 0, *pt_x = 0;
            if (counte > 10)
            {
                break;
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
        printf("lsb_frames_dec: %d\n", lsb_frames_dec);
        printf("msb_frames_dec: %d\n", msb_frames_dec);
        *pt_x = -1;
        *pt_y = -1;

        if (video_dec_ctx_msb)
        {
            decode_packet(video_dec_ctx_msb, NULL, frame_msb, 1, pt_x, msb_frame_buf, pt_msb_frm_count, 0);
        }
    }
    else
    {

        while (*pt_lsb_frm_count < num_frames_lsb)
        {
            while (*pt_x < frm_group_size)
            {
                // check if the packet belongs to a stream we are interested in, otherwise
                // skip it
                if (av_read_frame(fmt_ctx, pkt) >= 0)
                {
                    if (pkt->stream_index == video_stream_idx)
                    {

                        ret = decode_packet(video_dec_ctx, pkt, frame, 0, pt_x, lsb_frame_buf, pt_lsb_frm_count, 0);
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
                    ++counte;
                    break;
                }
            }
            *pt_x = 0;

            for (i = 0; i < frm_group_size; ++i)
            {
                if (lsb_frame_buf[i] != NULL)
                {

                    output_both_buffs(lsb_frame_buf[i], NULL, max_d, min_d);
                    free(lsb_frame_buf[i]);
                    lsb_frame_buf[i] = NULL;
                }
            }

            if (counte > 10)
            {
                break;
            }
        }
        *pt_x = -1;
        *pt_y = -1;
    }

    if (video_dec_ctx)
    {
        decode_packet(video_dec_ctx, NULL, frame, 0, pt_x, lsb_frame_buf, pt_lsb_frm_count, 0);
    }

end:
    std::cout << "Average PSNR: " << getAverage(buff_global::psnr_vector) << std::endl;
    // if (src_filename_msb != NULL)
    // {
    //     free(src_filename_msb);
    // }
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&video_dec_ctx_msb);
    avformat_close_input(&fmt_ctx);
    avformat_close_input(&fmt_ctx_msb);
    // if (audio_dst_file)
    //     fclose(audio_dst_file);
    av_packet_free(&pkt);
    av_packet_free(&pkt_msb);
    av_frame_free(&frame);
    av_frame_free(&frame_msb);
    av_free(video_dst_data[0]);
    av_free(video_dst_data_msb[0]);
    for (i = 0; i < frm_group_size; i++)
    {
        if (lsb_frame_buf[i] != NULL)
        {
            free(lsb_frame_buf[i]);
        }
        if (msb_frame_buf[i] != NULL)
        {
            free(msb_frame_buf[i]);
        }
    }
#if __has_include(<opencv2/opencv.hpp>)
    cv::destroyAllWindows();
#endif
    return 1;
}

long double getCompressionRatio(std::uintmax_t sz, int num_frames)
{
    std::uintmax_t calc_raw_sz = (std::uintmax_t)num_frames * H * W * 2;
    return (long double)calc_raw_sz / (long double)sz;
}
int main(int argc, char *argv[])
{
    int deref1 = 0, deref2 = 0, deref3 = 0; // deref4 = 0, deref5 = 0, deref6 = 0;
    int ret = 0;
    if (argc > 13 || argc < 2)
    {
        std::cout << " THIS FAILED HERE 1: argc: " << argc << std::endl;
        print_usage();
        return EXIT_FAILURE;
    }

    InputParser input(argc, argv);
    if (input.cmdOptionExists("-help") || input.cmdOptionExists("-h"))
    {
        std::cout << "Command line utility for compressing depth data and rgb data and infared from a camera." << std::endl;
        print_usage("");
        return EXIT_FAILURE;
    }

    if (!input.cmdOptionExists("-ilsb"))
    {
        std::cout << "Input lsb file is required" << std::endl;
        print_usage("");
        return EXIT_FAILURE;
    }

    if (!input.cmdOptionExists("-hd"))
    {
        std::cout << "Input header file and output directory are required" << std::endl;
        print_usage("-hd");
        return EXIT_FAILURE;
    }

    if (!input.cmdOptionExists("-o"))
    {
        std::cout << "Output directory is required" << std::endl;
        print_usage("-o");
        return EXIT_FAILURE;
    }
    std::string input_lsb_file = input.getCmdOption("-ilsb");

    if (!does_file_exist(input_lsb_file))
    {
        std::cout << "Input lsb file does not exist" << std::endl;
        print_usage("-ilsb");
        return EXIT_FAILURE;
    }
    std::string ffprobe_cmd;
    ffprobe_cmd = "ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 " + input_lsb_file;
    std::string ffprobe_cmd2;
    std::string header_file = input.getCmdOption("-hd");

    int *ptr_max = &deref1, *ptr_min = &deref2, *ptr_depth_units = &deref3;
    if (!does_file_exist(header_file))
    {
        std::cout << "Input header file does not exist" << std::endl;
        print_usage("-hd");
        return EXIT_FAILURE;
    }
    else
    {
        read_max_min(ptr_max, ptr_min, ptr_depth_units);
    }

    std::string output_dir = input.getCmdOption("-o");
    std::string input_msb_file;
    std::string input_raw_file_dir;

    int num_frames_lsb = 0;
    int num_frames_msb = 0;
    num_frames_lsb = exec_ffprobe(ffprobe_cmd);
    if (input.cmdOptionExists("-imsb"))
    {
        if (!does_file_exist(input.getCmdOption("-imsb")))
        {
            std::cout << "Input msb file does not exist" << std::endl;
            print_usage("-imsb");
            return EXIT_FAILURE;
        }
        input_msb_file = input.getCmdOption("-imsb");
        ffprobe_cmd2 = "ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 " + input_msb_file;
        num_frames_msb = exec_ffprobe(ffprobe_cmd2);
    }

    if (input.cmdOptionExists("-cmp"))
    {
        if (!does_dir_exist(input.getCmdOption("-cmp")))
        {
            std::cout << "Input raw file directory does not exist" << std::endl;
            print_usage("-cmp");
            return EXIT_FAILURE;
        }
        input_raw_file_dir = input.getCmdOption("-cmp");
    }
    printf("num_frames_lsb: %d num_frames_msb %d\n", num_frames_lsb, num_frames_msb);
    // int print_psnr = 1;
    if (input.cmdOptionExists("-print_psnr"))
    {
        print_psnr = parse_integer_cmd(input, "-print_psnr", print_psnr);
        if (print_psnr != 0 && print_psnr != 1)
        {
            std::cout << "Invalid value for -print_psnr" << std::endl;
            print_usage("-print_psnr");
            return EXIT_FAILURE;
        }
    }

    // int i = 100;

    // for (; i > 3; i--)
    // {
    //     if (num_frames_lsb % i == 0)
    //     {
    //         frm_group_size = i;
    //         break;
    //     }
    // }
    frm_group_size = 50;
    printf("frm_group_size: %d\n", frm_group_size);

    std::uintmax_t total_compressed_sz = 0;
    try
    {
        total_compressed_sz = std::filesystem::file_size(input_lsb_file);
        if (input_msb_file != "")
        {
            total_compressed_sz += std::filesystem::file_size(input_msb_file);
        }
    }
    catch (std::filesystem::filesystem_error &e)
    {
        std::cout << e.what() << '\n';
    }

    timer tStart;
    ret = !decompress(deref1, deref2, deref3, num_frames_lsb, num_frames_msb, output_dir, input_lsb_file, input_msb_file, input_raw_file_dir);
    unsigned long long milliseconds_ellapsed = tStart.milliseconds_elapsed();
    std::cout << "Time decompress run for in seconds: " << (milliseconds_ellapsed / 1000.0) << std::endl;
    if (ret)
    {

        std::cout << "Decompression failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Compression ratio is: " << getCompressionRatio(total_compressed_sz, num_frames_lsb) << std::endl;
    std::cout << buff_global::video_frame_count << " frames were compressed" << std::endl;
    return EXIT_SUCCESS;
}
// make: Nothing to be done for 'all'.
// Max 65535 Min 0 Depth_units in micrometers: 1000
// num_frames_lsb: 27000 num_frames_msb 27000
// frm_group_size: 50
// type: 0 *count: 1
// wq *count: 1 ret -11
// type: 0 *count: 2
// wq *count: 2 ret -11
// type: 0 *count: 3
// wq *count: 3 ret -11
// type: 0 *count: 4
// wq *count: 4 ret -11
// type: 0 *count: 5
// wq *count: 5 ret -11
// lsb_frames_dec: 27000
// msb_frames_dec: 27000
// Average PSNR: 71.6306
// Time decompress run for in seconds: 440.141
// Compression ratio is: 21.7228
// 27000 frames were compressed