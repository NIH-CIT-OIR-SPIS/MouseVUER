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
 * @date 01/06/2023
 *
 */
#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>

#endif

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/rs_advanced_mode.hpp>

// #include <example.hpp>
// // #include <imgui/imgui.h>
#include "../third-party/example.hpp"
#include "../third-party/example-imgui.hpp"

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
#include <random>

#include <jsoncpp/json/json.h>
/*My own defined header file*/
#include "recorder.hpp"

// Timer structure
enum class direction
{
    to_depth,
    to_color
};

/**
 * @brief A timer structure that uses the steady clock
 * @param reset Resets the timer
 * @param milliseconds_elapsed Returns the number of milliseconds elapsed since the timer was reset
 */
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

/**
 * @brief Writes
 * @param filename The name of the file to write to
 * @param txt The text to write to the file
 * @return true if the file was written to successfully, false otherwise
 */
bool write_txt_to_file(std::string filename, std::string txt)
{
    std::ofstream myfile;
    myfile.open(filename);
    if (myfile.is_open())
    {
        myfile << txt;
        myfile.close();
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Checks if a string is a number or not
 * @param str The string to check
 */
bool isNumber(const std::string &str)
{

    // `std::find_first_not_of` searches the string for the first character
    // that does not match any of the characters specified in its arguments
    return !str.empty() &&
           (str.find_first_not_of("[0123456789]") == std::string::npos);
}

/**
 * @brief Splits a string into tokens
 * @param str The string to split
 * @param delim The delimiter to split the string with
 */
std::vector<std::string> split(const std::string &str, char delim)
{
    using namespace std;

    auto i = 0;
    vector<string> list;

    auto pos = str.find(delim);

    while (pos != string::npos)
    {
        list.push_back(str.substr(i, pos - i));
        i = ++pos;
        pos = str.find(delim, pos);
    }

    list.push_back(str.substr(i, str.length()));

    return list;
}

/**
 * @brief Validates an IP address
 * @param ip The IP address to validate
 */
bool validateIP(std::string ip)
{
    using namespace std;
    // split the string into tokens
    vector<string> list = split(ip, '.');

    // if the token size is not equal to four
    if (list.size() != 4)
    {
        return false;
    }

    // validate each token
    for (string str : list)
    {
        // verify that the string is a number or not, and the numbers
        // are in the valid range
        if (!isNumber(str) || stoi(str) > 255 || stoi(str) < 0)
        {
            return false;
        }
    }

    return true;
}
/**
 * @brief Parses a command for ffmpeg
 * @param pix_fmt  The pixel format of the input frames i.e. YUV420P, RGB24, YUV420P10LE, etc.
 * @param pix_fmt_out  The pixel format of the output frames format i.e. YUV420P, RGB24, YUV420P10LE, etc.
 * @param encoder   The encoder to use i.e. libx264, libx265.
 * @param path_out  The path_out is either specifically a file path or could be a server and port name,
 *                  i.e. path_out = " -f -flv rtmp:://server:port/ "
 * @param time_run  The time_run is the time in seconds that the video will be recorded for.
 * @param count_threads  The count_threads is the number of threads to use for the recording.
 * @param width     The width of the input frames.
 * @param height    The height of the input frames.
 * @param fps       The frames per second of the input frames.
 * @param crf       The crf is the constant rate factor for the encoder.
 * @param ffmpeg_verbose  The ffmpeg_verbose is the verbosity of the ffmpeg command line.
 * @param depth_lossless    The depth_lossless is the lossless flag for the encoder.
 * @param typ   The typ is the the type of frames to expect whether LSB or MSB.
 * @return std::string  The ffmpeg command to be executed by th.
 */
std::string build_ffmpeg_cmd(std::string pix_fmt, std::string pix_fmt_out, std::string encoder, std::string path_out, long time_run,
                             int count_threads, int width, int height, int fps, int crf,
                             bool ffmpeg_verbose, bool depth_lossless, int typ)
{

    // typ 0 is LSB frames
    // typ 1 is MSB frames
    // type 2 is Color frames
    // type 3 is IR frames
    std::string thread_counter = (count_threads > 0 && count_threads < 8) ? " -threads " + std::to_string(count_threads) : "";
    std::string banner = (ffmpeg_verbose) ? " -loglevel repeat+level+debug " : " -loglevel error -nostats "; //" -loglevel trace";
    std::string ffmpeg_command;

    std::string re_flag = (path_out.find("rtmp:") != std::string::npos) ? " -re " : "";
    std::string flv_flag = (path_out.find("rtmp:") != std::string::npos) ? " -f flv " : "";
    std::string apostr = "\'";
    if (!path_out.find("rtmp:"))
    {
        apostr += path_out;
        path_out = apostr + "\'";
    }
    std::string tune_latency = ""; //" -tune zerolatency ";
    int num_bframes = 0;

    if (depth_lossless)
    {
        std::cout << "depth_lossless is a depricated feature" << std::endl;
    }
    if (typ == 0)
    {
        // typ 0 is LSB frames which ares stored in luma component of yuv420p10le format
        // note how we have to set bframes to 0 in order to align the lossless RGB frames with the lossy LSB frames.
        // This may however come at cost of quality.
        ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an " + re_flag + " -i -  -c:v " + encoder + " -preset veryfast " + tune_latency + " -pix_fmt " + pix_fmt_out + " -crf " + std::to_string(crf) + " -x264-params bframes=" + std::to_string(num_bframes) + " -movflags +faststart " + flv_flag + " " + path_out;
        // ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an -re -i -  -c:v " + encoder + " -preset veryfast -pix_fmt " + pix_fmt_out + " -crf " + std::to_string(crf) + " -x264-params bframes=" + std::to_string(num_bframes) + " -movflags +faststart -f flv rtmp://127.0.0.1:5000/ ";

        return ffmpeg_command;
    }
    else if (typ == 1 && encoder == "libx264rgb")
    {
        // typ 1 is MSB RGB frames lossless stored in red channel
        // By default B-frames are turned off for lossless encoding. Lossless encoding for libx264rgb encoder is crf of 0
        ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an " + re_flag + " -i - -c:v " + encoder + " -preset veryfast " + tune_latency + " -pix_fmt " + pix_fmt_out + " -crf " + std::to_string(crf) + " -movflags +faststart " + flv_flag + " " + path_out; //
        // ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an -re -i - -c:v " + encoder + " -preset veryfast  -crf " + std::to_string(crf) + " -movflags +faststart -movflags +faststart -f flv rtmp://127.0.0.1:5001/"; //

        return ffmpeg_command;
    }
    else if (typ == 1 && encoder == "libx264")
    {
        ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an " + re_flag + " -i - -c:v " + encoder + " -preset veryfast " + tune_latency + " -pix_fmt " + pix_fmt_out + " -crf " + std::to_string(crf) + " -movflags +faststart " + flv_flag + " " + path_out; //
        // ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an -re -i - -c:v " + encoder + " -preset veryfast  -crf " + std::to_string(crf) + " -movflags +faststart -movflags +faststart -f flv rtmp://127.0.0.1:5001/"; //

        return ffmpeg_command;
    }
    else if (typ == 2 || typ == 3)
    {
        ffmpeg_command = "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an -i - -c:v " + encoder + " -preset veryfast " + " -crf " + std::to_string(crf) + " -movflags +faststart " + flv_flag + " " + path_out;

        return ffmpeg_command;
    }
    std::cout << "Error: Unknown ffmpeg command type" << std::endl;
    return "";
}

/**
 * @brief      Save the intrinsics of the camera to a json file
 * @param      pipe     The pipe
 * @param      dirname  The dirname
 *
 */
void save_intrinsics(rs2::pipeline &pipe, std::string dirname)
{
    std::string path_out = dirname + "/intrinsics.json";
    std::ofstream intrinsics_file;
    intrinsics_file.open(path_out);
    rs2::pipeline_profile profile = pipe.get_active_profile();
    rs2::video_stream_profile depth_profile = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
    rs2::video_stream_profile ir_profile = profile.get_stream(RS2_STREAM_INFRARED, 1).as<rs2::video_stream_profile>();
    rs2::video_stream_profile ir2_profile = profile.get_stream(RS2_STREAM_INFRARED, 2).as<rs2::video_stream_profile>();
    rs2_extrinsics e = ir_profile.get_extrinsics_to(ir2_profile);
    float baseline = e.translation[0];

    rs2_intrinsics depth_intrinsics = depth_profile.get_intrinsics();
    rs2_intrinsics ir_intrinsics = ir_profile.get_intrinsics();
    rs2_intrinsics ir2_intrinsics = ir2_profile.get_intrinsics();

    auto scale = profile.get_device().first<rs2::depth_sensor>().get_depth_scale();
    intrinsics_file << "{" << std::endl;
    intrinsics_file << "    \"width\": " << depth_intrinsics.width << "," << std::endl;
    intrinsics_file << "    \"height\": " << depth_intrinsics.height << "," << std::endl;
    intrinsics_file << "    \"ppx\": " << depth_intrinsics.ppx << "," << std::endl;
    intrinsics_file << "    \"ppy\": " << depth_intrinsics.ppy << "," << std::endl;
    intrinsics_file << "    \"fx\": " << depth_intrinsics.fx << "," << std::endl;
    intrinsics_file << "    \"fy\": " << depth_intrinsics.fy << "," << std::endl;
    intrinsics_file << "    \"coeffs\": [" << depth_intrinsics.coeffs[0] << ", " << depth_intrinsics.coeffs[1] << ", " << depth_intrinsics.coeffs[2] << ", " << depth_intrinsics.coeffs[3] << ", " << depth_intrinsics.coeffs[4] << "]," << std::endl;
    intrinsics_file << "    \"depth_baseline\": " << baseline << "," << std::endl;
    intrinsics_file << "    \"depth_units\": " << scale << std::endl;
    intrinsics_file << "}" << std::endl;
    intrinsics_file.close();
    std::cout << "Intrinsics saved" << std::endl;
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
int startRecording(std::string dirname, long time_run, std::string bag_file_dir, uint16_t max_d = 65535, uint16_t min_d = 0, int depth_u = 1000, unsigned int width = 1280U,
                   unsigned int height = 720U, unsigned int fps = 30U, int crf_lsb = 25, int count_threads = 2,
                   bool ffmpeg_verbose = false, bool collect_raw = false, int num_frm_collect = 0, bool show_preview = false,
                   std::string json_file = "", bool color = false, int crf_color = 30, bool depth_lossless = false, std::string server_address = "", unsigned short port = 1000, uint8_t disparity_shift = 0,
                   bool align_depth_to_color = false, bool near_ir = true)
{
    if (time_run > 2147483646)
    {
        std::cout << "You're running a recording for 60+ years. You better know what you're doing." << std::endl;
    }
    if (fps <= 0U || fps > 60U || height > 1080U || width > 1920U || crf_lsb > 80 || crf_lsb < -1 || crf_color < -1 || crf_color > 50 || time_run <= 0L)
    {
        std::cout << width << "x" << height << "@" << fps << "fps, Time run: " << time_run << " CRF: " << crf_lsb << std::endl;
        return 0;
    }
    if (fps > 60U)
    {
        std::cout << "FPS too high must be less than 30: " << fps << " fps" << std::endl;
        return 0;
    }
    if (height % 2 != 0 || width % 2 != 0 || width > 1280 || height > 720 || width < 640 || height < 480)
    {
        std::cout << "Dimensions incorrect: " << width << "x" << height << "@" << fps << "fps" << std::endl;
        std::cout << "Dimensions must be even" << std::endl;
        return 0;
    }
    if (collect_raw && num_frm_collect < 0)
    {
        std::cout << "Numbec_cpp_propertiesr frames collect greater than 1800 at: " << num_frm_collect << std::endl;
        return 0;
    }

    // const auto processor_count = std::thread::hardware_concurrency();
    int width_color = width;   // width / 2;
    int height_color = height; // height / 2;
    int ret = 0;
    int have_video = 0;
    int encode_video = 0;
    int i = 0, k = 0, y = 0;
    int num_bytes = width * height;
    long counter = 0;
    // std::string path_raw = dirname + "test_raw.mp4"; //(server_address == "" || port <= -1) ? (dirname + "test_raw.mp4") : ("rtmp://"+ server_address + ":" + std::to_string(port) + "/ ");
    /* Bit Shifting Settings */
    const int shift_by = 0;  // 2;
    const int and_val = 0b0; // 0b11;
    int y_comp_8_bit = height * width;
    int u_comp_8_bit = y_comp_8_bit / 4;
    int v_comp_8_bit = y_comp_8_bit / 4;
    int channel_size = 1;
    std::string type_str_msb = "yuv420p";
    std::string encoder_msb = "libx264";
    int total_sz_8_bit = y_comp_8_bit + u_comp_8_bit + v_comp_8_bit;
    std::string path_lsb = (server_address == "" || port <= 1024) ? (dirname + "test_lsb.mp4") : ("rtmp://" + server_address + ":" + std::to_string(port) + "/ ");
    std::string path_msb = (server_address == "" || port <= 1024) ? (dirname + "test_msb.mp4") : ("rtmp://" + server_address + ":" + std::to_string(port + 1) + "/ ");
    std::string color_lsb_path = (server_address == "" || port <= 1024) ? (dirname + "test_color_lsb.mp4") : ("rtmp://" + server_address + ":" + std::to_string(port + 2) + "/ ");
    std::string near_ir_lsb_path = (server_address == "" || port <= 1024) ? (dirname + "test_ir_lsb.mp4") : ("rtmp://" + server_address + ":" + std::to_string(port + 3) + "/ ");
    std::string str_lsb = build_ffmpeg_cmd("yuv420p", "yuv420p", "libx264", path_lsb, time_run, count_threads, width, height, fps, crf_lsb, ffmpeg_verbose, depth_lossless, 0);
    // std::string str_msb = build_ffmpeg_cmd("gray", "yuvj420p", "libx264", path_msb, time_run, count_threads, width, height, fps, 0, ffmpeg_verbose, depth_lossless, 1);
    std::string str_msb = build_ffmpeg_cmd(type_str_msb, type_str_msb, encoder_msb, path_msb, time_run, count_threads, width, height, fps, 0, ffmpeg_verbose, depth_lossless, 1);

    std::string color_lsb = build_ffmpeg_cmd("rgb24", "yuv420p", "libx264", color_lsb_path, time_run, count_threads, width_color, height_color, fps, crf_color, ffmpeg_verbose, depth_lossless, 2);
    // std::string raw_cmd = build_ffmpeg_cmd("gray16le", "gray16le", "rawvideo", path_raw, time_run, count_threads, width, height, fps, 0, ffmpeg_verbose, depth_lossless, 4);
    std::string ir_str = build_ffmpeg_cmd("gray", "yuv420p", "libx264", near_ir_lsb_path, time_run, count_threads, width, height, fps, crf_color, ffmpeg_verbose, depth_lossless, 3);

    if (str_msb == "")
    {
        std::cerr << "Error with build_ffmpeg_cmd " << std::endl;
        return 0;
    }
    if (color_lsb != "")
    {
        std::cout << "FFmpeg command for color stream too " << std::endl;
        std::cout << color_lsb << std::endl;
    }
    if (ir_str != "")
    {
        std::cout << "FFmpeg command for near_ir stream too " << std::endl;
        std::cout << ir_str << std::endl;
    }
    std::cout << "FFmpeg command for str_lsb LSB frames:" << std::endl;
    std::cout << str_lsb << std::endl;
    std::cout << "FFmpeg command for str_msb MSB frames:" << std::endl;
    std::cout << str_msb << std::endl;
    FILE *pipe_lsb = NULL;
    FILE *pipe_msb = NULL;
    FILE *color_pipe = NULL;
    FILE *ir_pipe = NULL;

    int diff = (int)max_d - (int)min_d;
    bool only_10_bits = false;
    // floor( ( (double)(max_d - min_d) * 1000.0)/((double)depth_u)) <= 1023
    if (diff <= 1023)
    {
        only_10_bits = true;
    }
    // FILE *p_pipe_raw = NULL;
#ifdef _WIN32
    if (!(pipe_lsb = _popen(str_lsb.c_str(), "w")))
    {
        std::cerr << "popen error" << std::endl;
        return 0;
    }
    if (diff > 1023)
    {
        if (!(pipe_msb = _popen(str_msb.c_str(), "w")))
        {
            std::cerr << "Not greater than diff or popen error" << std::endl;
            // return 0;
        }
    }
    if (color)
    {
        if (!(color_pipe = _popen(color_lsb.c_str(), "w")))
        {
            std::cerr << "popen error 2" << std::endl;
            return 0;
        }
    }
    if (near_ir)
    {
        if (!(ir_pipe = _popen(ir_str.c_str(), "w")))
        {
            std::cerr << "popen error near_ir" << std::endl;
            return 0;
        }
    }
#else
    if (!(pipe_lsb = popen(str_lsb.c_str(), "w")))
    {
        std::cerr << "popen error" << std::endl;
        return 0;
    }
    if (diff > 1023)
    {
        if (!(pipe_msb = popen(str_msb.c_str(), "w")))
        {
            std::cerr << "Not greater than diff or popen error" << std::endl;
            // return 0;
        }
    }
    if (color)
    {
        if (!(color_pipe = popen(color_lsb.c_str(), "w")))
        {
            std::cerr << "popen error 2" << std::endl;
            return 0;
        }
    }
    if (near_ir)
    {
        if (!(ir_pipe = popen(ir_str.c_str(), "w")))
        {
            std::cerr << "popen error near_ir" << std::endl;
            return 0;
        }
    }
#endif

    rs2::context ctx;
    rs2::pipeline pipe(ctx);
    rs2::config cfg;

    std::string serial;

    auto device = ctx.query_devices();
    auto dev = device[0];

    if (json_file != "" && does_file_exist(json_file))
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

        std::ifstream if_str(json_file);
        Json::Reader reader;
        Json::Value root;
        reader.parse(if_str, root);

        height = std::stoi(root["viewer"]["stream-height"].asString());
        width = std::stoi(root["viewer"]["stream-width"].asString());
        fps = std::stoi(root["viewer"]["stream-fps"].asString());

        // close the file
        if_str.close();
    }
    if (bag_file_dir.size() > 1 && does_file_exist(bag_file_dir))
    {
        cfg.enable_device_from_file(bag_file_dir, false);
    }

    auto advanced_mode_dev = dev.as<rs400::advanced_mode>();
    STDepthTableControl depth_table = advanced_mode_dev.get_depth_table();
    if (!(json_file.size() > 0))
    {

        depth_table.depthUnits = (int32_t)depth_u;  // in micro meters
        depth_table.depthClampMin = (int32_t)min_d; // 100 mm
        depth_table.depthClampMax = (int32_t)max_d; //(int)pow(2, 16); // pow(2, 16);// 1000 mm
        depth_table.disparityShift = (int32_t)disparity_shift;
        advanced_mode_dev.set_depth_table(depth_table);
    }
    std::cout << "Depth max: " << advanced_mode_dev.get_depth_table().depthClampMax << "mm\nDepth unit: " << advanced_mode_dev.get_depth_table().depthUnits << std::endl;
    std::cout << "Depth min: " << advanced_mode_dev.get_depth_table().depthClampMin << "mm" << std::endl;
    std::cout << "Disparity Shift " << advanced_mode_dev.get_depth_table().disparityShift << std::endl;
    cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, fps); // Realsense configurationst
    if (near_ir)
    {
        cfg.enable_stream(RS2_STREAM_INFRARED, 1, width, height, RS2_FORMAT_Y8, fps);
        cfg.enable_stream(RS2_STREAM_INFRARED, 2, width, height, RS2_FORMAT_Y8, fps);
    }
    if (color || align_depth_to_color)
    {
        cfg.enable_stream(RS2_STREAM_COLOR, width_color, height_color, RS2_FORMAT_RGB8, fps);
    }
    std::cout << align_depth_to_color << std::endl;

    std::string frame_file_nm = dirname + "frame_num_";
    int y_comp = height * width; //
    int u_comp = y_comp / 4;
    int v_comp = y_comp / 4;
    int total_sz_lsb_bit = y_comp + u_comp + v_comp;

    uint8_t *store_frame_lsb = (uint8_t *)malloc(total_sz_lsb_bit * sizeof(uint8_t));
    uint8_t *store_frame_msb = (uint8_t *)malloc(total_sz_8_bit * sizeof(uint8_t));
    // uint8_t *store_frame_test = (uint8_t *)malloc(height * width * 1 * sizeof(uint8_t));

    memset(store_frame_lsb, 0, total_sz_lsb_bit);
    memset(store_frame_msb, 0, total_sz_8_bit);
    rs2::frameset frameset;
    rs2::colorizer coloriz;
    rs2::frame color_frame;
    rs2::frame depth_frame_in;
    rs2::frame colour_frame;
    rs2::frame ir_frame;

    std::string text_out = std::to_string(depth_table.depthClampMax) + " " + std::to_string(depth_table.depthClampMin) + " " + std::to_string(depth_table.depthUnits);
    write_txt_to_file(dirname + "video_head_file.txt", text_out);

    uint16_t *ptr_depth_frm_16 = NULL;
    uint8_t *ptr_depth_frm = NULL;

    // encode_video = 0;
    pipe.start(cfg); // Start the pipe with the cfg
    rs2::align align_to_color(RS2_STREAM_COLOR);
    // warm up camera
    for (int i = 0; i < 15; i++)
    {
        frameset = pipe.wait_for_frames();
    }

    // Save the intrinsics of the camera for example the callibration matrix and the distortion coefficients, needed for any rectification or projection
    save_intrinsics(pipe, dirname);

    const int sz_write_color_pipe = height_color * width_color * 3;
    const int sz_write_ir_pipe = height_color * width_color * 2;

    timer tStart;
    while ((long)time_run * fps > counter)
    {
        try
        {
            frameset = pipe.wait_for_frames(1000); // No frames seen for 1.5 second then exit
        }
        catch (const rs2::error &e)
        {
            std::cout << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
            break;
        }

        if (color && align_depth_to_color)
        {

            // auto depth_frameset = frameset.
            frameset = align_to_color.process(frameset);
            depth_frame_in = frameset.get_depth_frame();
        }
        else
        {
            depth_frame_in = frameset.get_depth_frame();
        }

        if (depth_frame_in)
        {
            ptr_depth_frm = (uint8_t *)depth_frame_in.get_data();

            // Remember all data after width * height *2 is 0 so we don't have to put total_sz_lsb_bit here
            // std::copy(ptr_depth_frm, ptr_depth_frm + (width * height * 2), store_frame_lsb);

            for (i = 0, k = 0; i < num_bytes * 2; i += 2, k += channel_size)
            {

                store_frame_msb[k] = ptr_depth_frm[i + 1]; //& (uint8_t)0b00000001;
                // store_frame_lsb[i + 1] &= (uint8_t)0b00000011;
                store_frame_lsb[k] = ptr_depth_frm[i];
                // store_frame_test[y] = store_frame_lsb[i];
            }
            if (pipe_msb == NULL || !fwrite(store_frame_msb, sizeof(uint8_t), total_sz_8_bit, pipe_msb))
            {
                std::cout << "Error with fwrite pipe_msb frames" << std::endl;
                break;
            }

            if (pipe_lsb == NULL || !fwrite(store_frame_lsb, sizeof(uint8_t), total_sz_lsb_bit, pipe_lsb))
            {
                std::cout << "Error with fwrite frames scaled depth" << std::endl;
                break;
            }

            // Write color frame to file
            if (color)
            {
                if ((colour_frame = frameset.get_color_frame()))
                {
                    if (!fwrite((uint8_t *)colour_frame.get_data(), sizeof(uint8_t), sz_write_color_pipe, color_pipe))
                    {
                        std::cerr << "Error with fwrite frames color" << std::endl;
                        break;
                    }
                }
            }
            if (near_ir)
            {
                if ((ir_frame = frameset.get_infrared_frame(2)))
                {
                    if (!fwrite((uint8_t *)ir_frame.get_data(), 1, height * width, ir_pipe))
                    {
                        std::cerr << "Error with fwrite frames near_ir" << std::endl;
                        break;
                    }
                }
            }

#if __has_include(<opencv2/opencv.hpp>)
            // std::cerr << "Why no printing" << std::endl;
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

            if (collect_raw)
            {
                if (counter < (long)num_frm_collect)
                {
                    std::string bin_out = frame_file_nm + std::to_string(counter) + "_data.bin";
                    FILE *p_file;
                    if ((p_file = fopen(bin_out.c_str(), "wb")))
                    {
                        // uint16_t* bit_data = (uint16_t*)depth_frame_in.get_data();
                        fwrite((uint8_t *)depth_frame_in.get_data(), 1, num_bytes * 2, p_file);
                        fclose(p_file);
                    }
                    else
                    {
                        std::cout << "Problem writing raw file " << bin_out << std::endl;
                    }
                }
            }

            ++counter;
            // fflush(pipe_lsb);
            // fflush(pipe_msb);
        }
    }

#if __has_include(<opencv2/opencv.hpp>)
    if (show_preview)
    {
        cv::destroyAllWindows();
    }
#endif

    unsigned long long milliseconds_ellapsed = tStart.milliseconds_elapsed();
    pipe.stop();

    memset(store_frame_lsb, 0, num_bytes);
    memset(store_frame_msb, 0, num_bytes);
    free(store_frame_lsb);
    free(store_frame_msb);

    try
    {
#ifdef _WIN32
        // if (!rosbag)
        //{
        if (pipe_lsb)
        {
            fflush(pipe_lsb);
            _pclose(pipe_lsb);
            pipe_lsb = NULL;
        }
        if (pipe_msb)
        {
            fflush(pipe_msb);
            _pclose(pipe_msb);
            pipe_msb = NULL;
        }

        if (color)
        {
            fflush(color_pipe);
            _pclose(color_pipe);
            color_pipe = NULL;
        }
        if (near_ir)
        {
            fflush(ir_pipe);
            _pclose(ir_pipe);
            ir_pipe = NULL;
        }

#else
        // if(!rosbag){
        if (pipe_lsb)
        {
            fflush(pipe_lsb);
            pclose(pipe_lsb);
            pipe_lsb = NULL;
        }
        if (pipe_msb)
        {
            fflush(pipe_msb);
            pclose(pipe_msb);
            pipe_msb = NULL;
        }

        if (color)
        {
            fflush(color_pipe);
            pclose(color_pipe);
            color_pipe = NULL;
        }
        if (near_ir)
        {
            fflush(ir_pipe);
            pclose(ir_pipe);
            ir_pipe = NULL;
        }

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
    if (argc > 35 || argc < 2)
    {
        // std::cout << " THIS FAILED HERE 1: argc: " << argc << std::endl;
        print_usage("");
        return EXIT_FAILURE;
    }
    InputParser input(argc, argv);
    if (input.cmdOptionExists("-h"))
    {
        std::cout << "Command line utility for compressing depth data and rgb data and near_ir from a camera." << std::endl;
        print_usage();
        return EXIT_FAILURE;
    }
    std::string dir = "";
    // auto processor_count = std::thread::hardware_concurrency();
    long sec = 0;
    int max_depth = 65535;
    int min_depth = 0;
    int depth_unit = 1000;
    int width = 1280;
    int height = 720;
    int fps = 30;
    int crf = 18;
    int thr = 2;
    int numraw = 0;
    int verbose = 0;
    int view = 0;
    int color = 0;
    int crf_color = 30;
    int depth_lossless = 0;
    int port = 1000;
    uint8_t disp_shift = 0;
    int align_int = 0;
    int near_ir = 0;

    std::string bagfile = "";
    std::string jsonfile = "";
    std::string server_address = "";
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
        std::cout << "ERROR -dir (directory not found)" << std::endl;
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

    if (input.cmdOptionExists("-sv_addr"))
    {
        server_address = input.getCmdOption("-sv_addr");
        if (!validateIP(server_address))
        {
            std::cout << "Error server_address" << std::endl;
            print_usage("-sv_addr");
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
    max_depth = parse_integer_cmd(input, "-max_depth", max_depth);
    min_depth = parse_integer_cmd(input, "-min_depth", min_depth);
    depth_unit = parse_integer_cmd(input, "-depth_unit", depth_unit);
    port = parse_integer_cmd(input, "-port", port);
    disp_shift = (uint8_t)parse_integer_cmd(input, "-disp_shift", (int)disp_shift);
    align_int = parse_integer_cmd(input, "-align_to_color", align_int);
    near_ir = parse_integer_cmd(input, "-ir", near_ir);
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
    if (thr < 0)
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
        std::cout << "Depth lossless is a deprecated option. please don't use it" << std::endl;
        depth_lossless = 0;
    }
    std::cout << "max_depth: " << max_depth << std::endl;
    std::cout << "min_depth: " << min_depth << std::endl;
    if (max_depth < 1 || max_depth > 65535 || max_depth < min_depth || max_depth == min_depth)
    {
        print_usage("-max_depth");
        return EXIT_FAILURE;
    }
    if (min_depth < 0 || min_depth > 65535 || min_depth > max_depth || max_depth == min_depth)
    {
        print_usage("-min_depth");
        return EXIT_FAILURE;
    }
    if (depth_unit < 40 || depth_unit > 100000)
    {
        print_usage("-depth_unit");
        return EXIT_FAILURE;
    }
    if (port < 0 || port > 65535)
    {
        print_usage("-port");
        return EXIT_FAILURE;
    }
    if (align_int != 0 && align_int != 1)
    {
        print_usage("-align_int");
        return EXIT_FAILURE;
    }

    if (near_ir < 0 || near_ir > 1)
    {
        print_usage("-ir");
        return EXIT_FAILURE;
    }
    if (sec <= (long)0)
    {
        std::cout << "Error -sec" << std::endl;
        print_usage("-sec");
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
    std::cout << "max_depth: " << max_depth << std::endl;
    std::cout << "min_depth: " << min_depth << std::endl;
    std::cout << "depth_unit: " << depth_unit << std::endl;
    std::cout << "port " << port << std::endl;
    std::cout << "disp_shift: " << disp_shift << std::endl;
    std::cout << "align_int: " << align_int << std::endl;
    std::cout << "near_ir: " << near_ir << std::endl;
    // std::cout << "rosbag: " << bool(rosbag) << std::endl;
    int retval = startRecording(dir, sec, bagfile, (uint16_t)max_depth, (uint16_t)min_depth, (uint16_t)depth_unit, width, height, fps, crf, thr, bool(verbose), bool(numraw), numraw, bool(view), jsonfile, bool(color), crf_color, bool(depth_lossless), server_address, (unsigned short)port, disp_shift, (bool)align_int, (bool)near_ir);
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