# Depth Recorder For Intel RealSense Cameras

### Noah Cubert


Uses intel realsense depth camera software, as well as ffmpeg to record data from cameras

This is a guide on how to run the program 
To install with linux please follow directions here https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_linux.md

Install opencv with the following directions
https://linuxize.com/post/how-to-install-opencv-on-ubuntu-20-04/

Install ffmpeg and build following the directions seen here:
https://www.reddit.com/r/ffmpeg/comments/7zjoi3/compiling_ffmpeg_on_ubuntu_with_multilib_x265/


# DIRECTIONS FOR ACTUAL SETUP

1) First Remove all previous Changes

```
rm -rf ~/ffmpeg_build ~/ffmpeg_sources ~/bin/{ffmpeg,ffprobe,ffplay,x264,x265,nasm} && \
sed -i '/ffmpeg_build/d' ~/.manpath && \
hash -r
```

2) Then run
```
sudo apt-get update -qq && sudo apt-get -y install \
  autoconf \
  automake \
  build-essential \
  cmake \
  git-core \
  libass-dev \
  libfreetype6-dev \
  libgnutls28-dev \
  libmp3lame-dev \
  libsdl2-dev \
  libtool \
  libva-dev \
  libvdpau-dev \
  libvorbis-dev \
  libxcb1-dev \
  libxcb-shm0-dev \
  libxcb-xfixes0-dev \
  meson \
  ninja-build \
  pkg-config \
  texinfo \
  wget \
  yasm \
  zlib1g-dev && \
  sudo apt-get -y install libunistring-dev && \
  sudo apt install libunistring-dev libaom-dev libdav1d-dev
```

3) Then run
```
mkdir -p ~/ffmpeg_sources ~/bin
```

4) Then run

```
sudo apt-get -y install nasm && \
sudo apt-get -y install libx264-dev && \
sudo apt-get -y install libx265-dev libnuma-dev && \
sudo apt-get -y install libvpx-dev && \
sudo apt-get -y install libfdk-aac-dev
```

5) Then run
```
cd ~/ffmpeg_sources && \
wget -O ffmpeg-snapshot.tar.bz2 https://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2 && \
tar xjvf ffmpeg-snapshot.tar.bz2 && \
cd ffmpeg 
```

6) Then edit files as you wish and save changes

7) Then run
```
cd ~/ffmpeg_sources/ffmpeg/ && \
PATH="$HOME/bin:$PATH" PKG_CONFIG_PATH="$HOME/ffmpeg_build/lib/pkgconfig" ./configure \
  --prefix="$HOME/ffmpeg_build" \
  --pkg-config-flags="--static" \
  --extra-cflags="-I$HOME/ffmpeg_build/include" \
  --extra-ldflags="-L$HOME/ffmpeg_build/lib" \
  --extra-libs="-lpthread -lm" \
  --extra-libs="-lpthread" \
  --ld="g++" \
  --bindir="$HOME/bin" \
  --enable-gpl \
  --enable-gnutls \
  --enable-libfdk-aac \
  --enable-libvpx \
  --enable-libx264 \
  --enable-libx265 \
  --enable-nonfree \
  --disable-static --enable-shared && \
PATH="$HOME/bin:$PATH" VAR1USE="$(grep -c ^processor /proc/cpuinfo)" make -j$VAR1USE && \
make -j$VAR1USE install && \
hash -r && \
source ~/.profile

```
8) After compiling to make any more subsequent edits to fftools please use the command found in step 7 to update the build


9) Then you will have to edit certain files by running the load_library.sh script
```
cd ~/DepthCameraRecord/ && \
chmod +x load_library.sh && \
sudo ./load_library.sh && \
sudo ldconfig
```


Then plug in your Intel RealSense Camera into the computer's USB port

Then run:
```
(make clean && make) || make
```


Then in order to see a list of options do the following
```
./bin/multicam -h
```

For example here is a simple recording for 30 seconds with files saved to Testing_DIR/
```
./bin/multicam -dir Testing_DIR/ -sec 30 -fps 30 -crf 23 -thr 2 -jsonfile Default.json
```







Or for more complex commands
```
make clean && make && python3 run_multi_cam_parser.py --dir Testing_DIR/ --time_run 15 --bag_file Testing_DIR/Large_Drive_Bag_File.bag
```
This can load a bag file



This can collect raw frames to compare two bag files in h265_recorder_ffmpeg/
```
make clean && make && python3 run_multi_cam_parser.py --dir Testing_DIR/ --time_run 30 --crf 15 --collect_raw --num_raw 900 --bag_file <YOUR BAG FILE>.bag
&& ./bin/decompress -vi Testing_DIR/test_lossy.h265 -raw Testing_DIR/test_raw -vs
```


To compare two videos side by side use the following:
```
python3 play_video.py -vid1 Testing_DIR/test_lossy.h265

```

To look at frame rates
```
ffprobe Testing_DIR/test_lossy.h265 -count_frames -show_entries stream=nb_read_frames,avg_frame_rate,r_frame_rate
```

To decompress and visualize:
```
make clean && make  ./bin/decompress -vi Testing_DIR/test_lossy.h265 -raw Testing_DIR/test_raw -vs
```

To look at stats of a video please see the following from (https://ffmpeg.org/ffprobe-all.html)
```
ffprobe -f lavfi movie=Testing_DIR/test_lsb.mp4,signalstats -show_entries frame_tags=lavfi.signalstats.YMAX,lavfi.signalstats.YMIN,lavfi.signalstats.YAVG,lavfi.signalstats.YBITDEPTH,lavfi.signalstats.YDIF
```

To see the ssim of the video please see the ssim options and psnr options for ffmeg x264 encoder.


