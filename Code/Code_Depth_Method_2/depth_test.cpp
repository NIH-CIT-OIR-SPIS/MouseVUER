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
#include<cmath>
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

int main(){
    std::cout << "Hello World!" << std::endl;
    return 0;
}