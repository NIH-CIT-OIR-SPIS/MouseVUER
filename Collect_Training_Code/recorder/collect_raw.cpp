#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#endif
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/rs_advanced_mode.hpp>

#include <bits/stdc++.h>
#include <ctime>

extern "C"
{
#include <sys/stat.h>
}
const int width = 1280;
const int height = 720;
const int fps = 30;

const int width_rgb = width;
const int height_rgb = height;
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

void print_usage(std::string choice = "")
{
    std::cout << "Usage: -dir <Directory Path> " << std::endl;
    std::cout << "Usage: -sec <double> time to run for" << std::endl;
    std::cout << "Usage: -jsonfile <STRING> Json file path" << std::endl;
    std::cout << "Usage: -align <int> 1 or 0 (1 to record with mp4 files and align 0 to just record to a regular bagfile unaligned)" << std::endl;
    std::cout << "Usage: -bagfile <STRING> Bag file path" << std::endl;
}

int start_bag_recording(long time_run, std::string dir, std::string path_out, std::string jsonfile, int align_int, std::string bagfile, std::string time_str = "0")
{
    int counter = 0;
    rs2::context ctx;
    rs2::pipeline pipe(ctx);
    rs2::config cfg;
    std::string serial;
    FILE *pipe_depth = NULL;
    FILE *pipe_color = NULL;
    std::string pixf = "gray16le";
    std::string ffmpeg_color = "ffmpeg -loglevel error -nostats -y -f rawvideo -pix_fmt rgb24 -c:v rawvideo -s " + std::to_string(width_rgb) + "x" + std::to_string(height_rgb) + " -t " + std::to_string(time_run) + " -r 30 -an -i - -vcodec libx264rgb -preset ultrafast -movflags +faststart ";
    std::string ffmpeg_cmd = "ffmpeg -loglevel error -nostats -y -f rawvideo -pix_fmt " + pixf + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r 30 -an -i - -vcodec ffv1 -level 3 -threads 4 -coder 1 -context 0 -g 1 -slices 24 -slicecrc 1 -pix_fmt " + pixf + " -movflags +faststart ";
    // std::string ffmpeg_cmd = "ffmpeg -loglevel error -nostats -y -threads 4 -f rawvideo -pix_fmt yuv420p16le -c:v rawvideo -s 1280x720 -t " + std::to_string(time_run) + " -r 30 -an -i - -vcodec  -pix_fmt yuv420p16le -movflags +faststart ";
    // std::string depth_cmd = ffmpeg_cmd + dir + "depth_vid.yuv";
    // ffplay -f rawvideo -pixel_format yuv420p16le -video_size 1280x720 -framerate 30 Testing_DIR/depth_vid.yuv
    std::string color_cmd = ffmpeg_color + "'" + dir + "color_vid" + time_str + ".mp4'";
    std::string depth_cmd = ffmpeg_cmd + "'" + dir + "depth_vid" + time_str + ".mkv'";
    uint8_t *ptr_depth_frame = NULL;
    // uint8_t *ptr_color_frame = NULL;

    int y_comp_16_bit = 2 * height * width; //
    int u_comp_16_bit = y_comp_16_bit / 4;
    int total_size_16_bit = y_comp_16_bit + u_comp_16_bit + u_comp_16_bit;
    if (pixf == "gray16le"){
        total_size_16_bit = y_comp_16_bit;
    }
    // int total_size_16_bit = y_comp_16_bit;
    uint8_t *store_depth = (uint8_t *)malloc(total_size_16_bit * sizeof(uint8_t));
    memset(store_depth, 0, total_size_16_bit * sizeof(uint8_t));
    if (align_int)
    {
        if (!(pipe_depth = popen(depth_cmd.c_str(), "w")) || !(pipe_color = popen(color_cmd.c_str(), "w")))
        {
            std::cout << "Failed to open pipe" << std::endl;
            return 0;
        }
    }
    // Make device list
    // std::vector<rs2::device> dev_list = ctx.query_devices();
    if (!jsonfile.empty())
    {

        for (rs2::device &&dev : ctx.query_devices())
        {
            rs400::advanced_mode advanced_mode_dev = dev.as<rs400::advanced_mode>();
            if (!advanced_mode_dev.is_enabled())
            {
                // If not, enable advanced-mode
                advanced_mode_dev.toggle_advanced_mode(true);
                std::cout << "Advanced mode enabled. " << std::endl;
            }
            std::ifstream fp_file(jsonfile);
            std::string preset_json((std::istreambuf_iterator<char>(fp_file)), std::istreambuf_iterator<char>());

            advanced_mode_dev.load_json(preset_json);
        }
    }
    if (!bagfile.empty())
    {
        cfg.enable_device_from_file(bagfile, false);
    }

    cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, fps);
    cfg.enable_stream(RS2_STREAM_COLOR, width_rgb, height_rgb, RS2_FORMAT_RGB8, fps);

    if (!align_int)
    {
        cfg.enable_record_to_file(path_out);
    }

    // rs2::frame depth;
    // rs2::frame color;
    //  rs2::frame depth_frame_in;
    //  rs2::frame rgb_frame;
    rs2::frameset frameset;
    rs2::align align_to_color(RS2_STREAM_COLOR);
    std::string color_file = dir + "color";
    std::string aligned_depth_file = dir + "aligned_depth";
    pipe.start(cfg); // Start the pipe with the cfg
    timer tStart;
    if (!align_int)
    {
        while ((long)time_run * fps > counter)
        {
            try
            {
                frameset = pipe.wait_for_frames(1000); // No frames seen for 1 second then exit
            }

            catch (const rs2::error &e)
            {
                std::cout << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
                break;
            }
            //rs2::frame aligned_frame = align_to_color.process(frameset);
            auto depth = frameset.get_depth_frame();
            auto color = frameset.get_color_frame();
            if (!depth || !color)
            {
                continue;
            }
            else
            {
                /*
                ptr_depth_frame = (uint8_t *)depth.get_data();
                //ptr_color_frame = (uint8_t *)color.get_data();
                std::copy(ptr_depth_frame, ptr_depth_frame + (width * height * 2), store_depth);
                if (!pipe_depth || !fwrite(store_depth, sizeof(uint8_t), total_size_16_bit, pipe_depth)){
                    std::cout << "Failed to write to pipe" << std::endl;
                    break;
                }

                if(!pipe_color || !fwrite((uint8_t *)color.get_data(), sizeof(uint8_t), 640 * 360 * 3, pipe_color)){
                    std::cout << "Failed to write to pipe" << std::endl;
                    break;
                }
                */
                // ptr_depth_frame = NULL;
                ++counter;
            }
        }
    }
    else
    {
        while ((long)time_run * fps > counter)
        {
            try
            {

                frameset = pipe.wait_for_frames(1000); // No frames seen for 1 second then exit
            }
            catch (const rs2::error &e)
            {
                std::cout << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
                break;
            }
            //rs2::frame aligned_frame = align_to_color.process(frameset);
            frameset = align_to_color.process(frameset);
            auto depth = frameset.get_depth_frame();
            auto color = frameset.get_color_frame();

            if (!depth || !color)
            {
                continue;
            }
            else
            {
                ptr_depth_frame = (uint8_t *)depth.get_data();
                std::copy(ptr_depth_frame, ptr_depth_frame + (width * height * 2), store_depth);
                if (!pipe_depth || !fwrite(store_depth, sizeof(uint8_t), total_size_16_bit, pipe_depth))
                {
                    std::cout << "Failed to write to pipe" << std::endl;
                    break;
                }
                if(!pipe_color || !fwrite((uint8_t *)color.get_data(), sizeof(uint8_t), width_rgb * height_rgb * 3, pipe_color)){
                    std::cout << "Failed to write to pipe" << std::endl;
                    break;
                }
                // ptr_depth_frame = NULL;
                ++counter;
            }

            // Save as PNG
            //             std::string color_filename = color_file + std::to_string(counter) + ".png";
            //             std::string depth_filename = aligned_depth_file + std::to_string(counter) + ".png";
            // #if __has_include(<opencv2/opencv.hpp>)
            //             cv::Mat color_mat(cv::Size(width, height), CV_8UC3, (uint8_t*)color.get_data(), cv::Mat::AUTO_STEP);
            //             cv::Mat depth_mat(cv::Size(width, height), CV_16UC1, (uint16_t *)depth.get_data(), cv::Mat::AUTO_STEP);
            //             cv::imwrite(color_filename, color_mat);
            //             cv::imwrite(depth_filename, depth_mat);
            // #endif
        }
    }
    unsigned long long milliseconds_ellapsed = tStart.milliseconds_elapsed();
    pipe.stop();
    if (pipe_depth != NULL)
    {
        pclose(pipe_depth);
    }
    if (pipe_color != NULL)
    {
        pclose(pipe_color);
    }
    free(store_depth);
    std::cout << "Time run for in seconds: " << (milliseconds_ellapsed / 1000.0) << std::endl;
    std::cout << "Numframes recorded: " << counter << std::endl;
    return 1;
}

int main(int argc, char *argv[])
try
{
    int ret = 0;
    if (argc > 13 || argc < 3)
    {
        std::cout << " THIS FAILED HERE 1: argc: " << argc << std::endl;
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
    long sec = 0;
    int align_int = 0;
    std::string jsonfile;
    std::string out_pth;
    std::string dir;
    std::string bagfile = "";
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
    align_int = parse_integer_cmd(input, "-align", align_int);
    if (align_int != 1 && align_int != 0)
    {
        std::cout << "Error -align" << std::endl;
        print_usage("");
        return EXIT_FAILURE;
    }

    if (sec <= (long)0)
    {
        std::cout << "Error -sec" << std::endl;
        print_usage("-sec");
        return EXIT_FAILURE;
    }

    auto time_1 = std::chrono::system_clock::now();
    std::time_t start_time = std::chrono::system_clock::to_time_t(time_1);
    // Convert to string
    std::stringstream ss;
    ss << std::put_time(std::localtime(&start_time), "%Y-%m-%d-%H-%M-%S");
    std::string start_time_str = ss.str();
    std::cout << "Starting time: " << start_time_str << std::endl;
    std::string path_out = dir + "/" + start_time_str + ".bag";
    ret = start_bag_recording(sec, dir, path_out, jsonfile, align_int, bagfile, start_time_str);
    if (!ret)
    {
        std::cout << "ERROR: " << ret << std::endl;
        return EXIT_FAILURE;
    }

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