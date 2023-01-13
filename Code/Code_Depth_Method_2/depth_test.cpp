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


float equat_L(int dist, int w){
    return ((float)dist - 0.5) / (float)w;
}
float equat_Ha(int d, int np, int w){
    float l_d = equat_L(d, w);
    int p = np / w;
    float ph = p / (float)2;
    float l_d_r = (l_d / ph);
    float diff = l_d_r - 1;
    int mod_val = (int)l_d_r % 2;
    if (mod_val <= 1){
        return (float)mod_val + diff;
    }else{
        return 2.0f - (float)mod_val - diff;
    }
}

float equat_Hb(int d, int np, int w){
    float l_d = equat_L(d, w);
    int p = np / w;
    float p4 = p / (float)4;
    float ph = p / (float)2;
    float l_d_r = ( (l_d - p4) / ph);
    
    int mod_val = (int)l_d_r % 2;
    float diff = l_d_r - mod_val;
    if (mod_val <= 1){
        return (float)mod_val + diff;
    }else{
        return 2.0f - (float)mod_val - diff;
    }
}

float equat_mL(float L, int np, int w){
    int p = np / w;
    float holder = floorf32((4.0f*L/p) - .5f);
    int mod_val = (int)holder % 4;
    float diff = holder - mod_val;
    return (float)mod_val + diff;
}

float equat_L0(float L,int np, int w){
    int p = np / w;
    float p4 = p / (float)4;
    float p8 = p / (float)8;

    float holder = L - p8;
    int mod_val = (int)holder % p;
    float diff = holder - mod_val;
    return L - ((float)mod_val + diff) + p4*equat_mL(L, np, w) - p8;
}

float delta_f(float L, float Ha, float Hb, int np, int w){
    int p = np/w;
    float ag = equat_mL(L, np, w);
    if (ag == 0.0f){
        return Ha*(p/2.0f);
    }else if (ag == 1.0f){
        return Hb*(p/2.0f);
    }
    else if (ag == 2.0f){
        return (1-Ha)*(p/2.0f);
    } else if (ag == 3.0f){
        return (1-Hb)*(p/2.0f);
    }else{
        printf("what is happending\n");
        return 0.0f;
    }

}

int back_orig(int L_, int Ha_, int Hb_, int w, int np){
    float L = L_ / float(255);
    float Ha = Ha_ / float(255);
    float Hb = Hb_ / float(255);

    return (int)(w*(equat_L0(L, np, w) + delta_f(L, Ha, Hb, np, w)));
}
int main(){

    int i = 0;
    int w = 65535;
    int n_p = 512;
    int p = n_p / w;
    for(i = 1; i <= w; i++){
        float L = equat_L(i, w);
        float Ha = equat_Ha(i, n_p, w);
        float Hb = equat_Hb(i, n_p, w);

        
    }
    return 0;
}