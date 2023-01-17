#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/rs_advanced_mode.hpp>
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
#include <filesystem> // C++17
#include <bits/stdc++.h>
#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#endif

// #include <boost/program_options/cmdline.hpp>
// #include <boost/program_options/options_description.hpp>
// #include <boost/program_options/detail/cmdline.hpp>
// // #include <boost/program_options/detail/config_file.hpp>
// // #include <boost/program_options/parsers.hpp>
// #include <boost/program_options/variables_map.hpp>
#include <cmath>
extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <sys/stat.h>
// #define frm_group_size 3
#define W 1280
#define H 720
#define MAXN 100001
}

// define my own doubleing point type

namespace eqta
{
    int dist = 0;
    int w = (int)pow(2, 16);
    int np = 512;
    double p = (double)np / (double)w;
}

double equat_L(int dist)
{
    return ((double)dist + 0.5) / (double)eqta::w;
}

double equat_H_a(int d)
{
    double l_d = equat_L(d);
    double help = l_d / (eqta::p / 2.0f);
    double hold = std::fmod(help, 2.0f);
    if (hold <= 1.0f)
    {
        return hold;
    }
    else
    {
        return 2.0f - hold;
    }
    // double ph = eqta::p / 2.0f;
    // double l_d_r = (l_d / ph);
    // std::cout << "l_d: " << l_d << std::endl;
    // std::cout << "ph: " << ph << std::endl;
    // std::cout << "l_d_r: " << l_d_r << std::endl;
    // double diff = l_d_r - 1;
    // int mod_val = (int)l_d_r % 2;
    // if (mod_val <= 1){
    //     return (double)mod_val + diff;
    // }else{
    //     return 2.0f - (double)mod_val - diff;
    // }
}

double equat_H_b(int d)
{
    double l_d = equat_L(d);
    double help = (l_d - (eqta::p / 4.0f)) / (eqta::p / 2.0f);
    double hold = std::fmod(help, 2.0f);
    if (hold <= 1.0f)
    {
        return hold;
    }
    else
    {
        return 2.0f - hold;
    }
}

double equat_mL(double L)
{
    double help = floorf32((4.0f * L / eqta::p) - .5f);
    return std::fmod(help, 4.0f);
}

double equat_L0(double L_)
{
    std::cout << "L_: " << L_ << std::endl;
    double p4 = eqta::p / 4.0f;
    double p8 = eqta::p / 8.0f;
    double holder = L_ - p8;
    double mod_val = std::fmod(holder, eqta::p);
    return L_ - mod_val + p4 * equat_mL(L_) - p8;
}

double delta_f(double L_, double Ha_, double Hb_)
{
    double ag = equat_mL(L_);
    if (ag == 0.0f)
    {
        return Ha_ * (eqta::p / 2.0f);
    }
    else if (ag == 1.0f)
    {
        return Hb_ * (eqta::p / 2.0f);
    }
    else if (ag == 2.0f)
    {
        return (1.0f - Ha_) * (eqta::p / 2.0f);
    }
    else if (ag == 3.0f)
    {
        return (1.0f - Hb_) * (eqta::p / 2.0f);
    }
    else
    {
        std::cout << "ERROR: ag is not in [0, 3]" << std::endl;
        return -5.0f;
    }
}

int back_to_orig(double L_, double Ha_, double Hb_)
{
    return (int) (eqta::w * (equat_L0(L_) + delta_f(L_, Ha_, Hb_)));
}



int main()
{

    int i = 0;
    int w = 65535;
    int n_p = 512;
    int p = n_p / w;
    double L = 0.0, Ha = 0.0, Hb = 0.0;
    int L_ = 0, Ha_ = 0, Hb_ = 0;
    int orig = 0;
    int count = 0;
    for (i = 0; i <= w; i++)
    {
        L = equat_L(i);
        Ha = equat_H_a(i);
        Hb = equat_H_b(i);
        std::cout << "depth_val: " << i << std::endl;
        std::cout << "L: " << L << std::endl;
        std::cout << "Ha: " << Ha << std::endl;
        std::cout << "Hb: " << Hb << std::endl;

        L_ = (int)(L * 255);
        Ha_ = (int)(Ha * 255);
        Hb_ = (int)(Hb * 255);

        std::cout << "L_: " << L_ << std::endl;
        std::cout << "Ha_: " << Ha_ << std::endl;
        std::cout << "Hb_: " << Hb_ << std::endl;

        L = (std::round(L_) / 255.0);
        Ha = (std::round(Ha_) / 255.0);
        Hb = (std::round(Hb_) / 255.0);

        std::cout << "L: " << L << std::endl;
        std::cout << "Ha: " << Ha << std::endl;
        std::cout << "Hb: " << Hb << std::endl;

        orig = back_to_orig(L, Ha, Hb);
        std::cout << "orig: " << orig << std::endl;

        
        if (std::abs(orig - i) > 120)
        {
            std::cout << "ERROR: " << orig << " != " << i << std::endl;
            count++;
        }
    }
    std::cout << "count: " << count << std::endl;
    return 0;
}

