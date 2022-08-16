
# Depth Recording Tool

### Authors
Noah Cubert

Ghadi Salem


Uses Intel&reg; RealSense&trade; depth camera software, as well as FFmpeg to record data from cameras


### Install python3 modules
```
sudo apt update && \
sudo apt install python3-pip && \
python3 -m pip install scapy && \
python3 -m pip install pycrypto && \
python3 -m pip install ffmpeg-python && \
python3 -m pip install numpy
```


### Install librealsense with Ubuntu 20.04 (as of 7/1/2022)

This is a guide on how to run the program 
To install with linux please follow directions here https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_linux.md

### Install Boost, FFmpeg, GLFW3 and others using the following commands:

Type the following into the command line:
```
sudo apt-get -y install libboost-all-dev && \
sudo apt-get -y install libglfw3 && \
sudo apt-get -y install libglfw3-dev && \
sudo apt-get -y install freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev  && \
sudo apt-get -y install ffmpeg && \
sudo apt-get -y install libavcodec-dev libavutil-dev libavformat-dev libswscale-dev
```

### OPTIONAL Install Intel QuickSync
```
sudo apt install intel-media-va-driver-non-free x264 libva-dev libmfx-dev intel-media-va-driver-non-free x264 intel-media-va-driver-non-free i965-va-driver-shaders && \
export LIBVA_DRIVER_NAME=iHD
```
More info https://trac.ffmpeg.org/wiki/Hardware/VAAPI


### Install OpenCV with the following directions

https://linuxize.com/post/how-to-install-opencv-on-ubuntu-20-04/

## If the above doesn't work
from 
https://docs.opencv.org/4.6.0/d7/d9f/tutorial_linux_install.html
1) Install manually using guide below.
```
sudo apt install -y g++ && \
sudo apt install -y cmake && \
sudo apt install -y make && \
sudo apt install -y wget unzip && \
sudo apt install -y git && \
cd ~/ && \
wget -O opencv.zip https://github.com/opencv/opencv/archive/4.x.zip && \
unzip opencv.zip && \
mv opencv-4.x opencv && \
mkdir -p build && cd build && \
cmake -DOPENCV_GENERATE_PKGCONFIG=ON ../opencv && \
make -j5 && \
sudo make install && \
sudo ldconfig -v
```

2) Then 
```
sudo nano /usr/local/lib/pkgconfig/opencv4.pc
```

3) And if nothing is there then copy the following to the opencv4.pc file:
```
# Package Information for pkg-config

prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include/opencv4

Name: OpenCV
Description: Open Source Computer Vision Library
Version: 4.6.0 
Libs: -L${exec_prefix}/lib -lopencv_gapi -lopencv_highgui -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_video -lopencv_calib3d -lopencv_features2d -lopencv_dnn -lopencv_flann -lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lopencv_core
Libs.private: -ldl -lm -lpthread -lrt
Cflags: -I${includedir}
```

4) Then do 
```
sudo nano ~/.bashrc
```

5) Now copy the following at the bottom of the .bashrc file
```
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig
export PKG_CONFIG_PATH
```

6) Now execute the command
```
sudo ldconfig -v
```





### Directions for Use
Then plug in your Intel RealSense Camera into the computer's USB port

Then run:
```
make -j4
```

Or for a new run try
```
make clean && make -j4
```

Then in order to see a list of options do the following
```
./bin/multicam -h
```

### Examples
For example here is a simple recording for 30 seconds with files saved to Testing_DIR/
```
./bin/multicam -dir Testing_DIR/ -sec 30 -fps 30 -crf 23 -thr 4 -jsonfile Default.json
```

For command help type:
```
./bin/multicam -h
```


To look at frame rates
```
ffprobe Testing_DIR/test_lossy.h265 -count_frames -show_entries stream=nb_read_frames,avg_frame_rate,r_frame_rate
```



To look at stats of a video please see the following from (https://ffmpeg.org/ffprobe-all.html)
```
ffprobe -f lavfi movie=Testing_DIR/test_lsb.mp4,signalstats -show_entries frame_tags=lavfi.signalstats.YMAX,lavfi.signalstats.YMIN,lavfi.signalstats.YAVG,lavfi.signalstats.YBITDEPTH,lavfi.signalstats.YDIF
```

## Run multicam program
```
make -j4 && ./bin/multicam -sec 60 -thr 4 -fps 30 -crf 22 -numraw 450 -max_depth 1000 -dir Testing_DIR/ -bagfile ~/Downloads/fishy-fish.bag && ./bin/decompress Testing_DIR/test_lsb.mp4 Testing_DIR/test_msb.mp4 

```
### Decompress
```
./bin/decompress -ilsb Testing_DIR/test_lsb.mp4  -imsb Testing_DIR/test_msb.mp4 -hd Testing_DIR/video_head_file.txt -cmp Testing_DIR -o Testing_DIR -print_psnr 0
```

## Automate GDB
```
for i in {1..20}; do gdb -q -ex 'set pagination off' -ex 'set args -dir Testing_DIR/ -sec 15 -numraw 450 -crf 17 -bagfile ~/Downloads/One_Mouse_5_minutes_depth.bag' -ex run  ./bin/multicam -ex quit; done
```
