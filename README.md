
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
python3 -m pip install numpy && \
sudo apt-get install python3-tk
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
sudo apt-get -y install libjsoncpp-dev && \
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
python3 -m pip install pyrealsense2 && \
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

3) And if nothing is there then copy the following to the opencv4.pc file please replace Version field with your specified version number :

```
# Package Information for pkg-config

prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include/opencv4

Name: OpenCV
Description: Open Source Computer Vision Library
Version: 4.7.0 
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

### Making your json file config
Connect your realsense camera and run
```
realsense-viewer
```

Then make any setting changes you wish to make and save changes using the save icon on the camera. You will be prompted to save it to a .json file
We will denote this json file as YourJsonFile.json

Save this file in the Code_8_bit/ directory

### Directions for Use
Then plug in your Intel RealSense Camera into the computer's USB port

Then run:
```
cd ~/DepthCameraRecordingTool/Code_8_bit/ && mkdir Testing_DIR
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

Decrease crf for better quality at expense of higher cpu processing and possible frame delays
thr just means number of cores you wish to utilize, default is 2, choose a number greater than 8 or 0 to cause it to use all cores
```
./bin/multicam -dir Testing_DIR/ -sec 30 -fps 30 -crf 23 -thr 0 -jsonfile Default.json
```

Record with depth, color and IR stream while aligning depth frames to color

```
./bin/multicam -dir Testing_DIR/ -sec 30 -fps 30 -crf 23 -thr 0 -jsonfile Default.json -color 1 -ir 1 -align_to_color 1
```

Record with just depth, and IR stream for 30 seconds 

```
./bin/multicam -dir Testing_DIR/ -sec 30 -fps 30 -crf 23 -thr 0 -jsonfile Default.json -ir 1
```

For command info  help type:

```
./bin/multicam -h
```

## Run multicam program

```
make -j4 && ./bin/multicam -sec 60 -thr 4 -fps 30 -crf 22 -numraw 450 -max_depth 1000 -dir Testing_DIR/ -bagfile ~/Downloads/fishy-fish.bag && ./bin/decompress Testing_DIR/test_lsb.mp4 Testing_DIR/test_msb.mp4 

```


### Decompress Lossy

Decompressing and comparing against raw files

```
./bin/decompress -ilsb Testing_DIR/test_lsb.mp4  -imsb Testing_DIR/test_msb.mp4 -hd Testing_DIR/video_head_file.txt -cmp Testing_DIR -o Testing_DIR -print_psnr 0
```

### Decompress Lossy

Decompressing regular no comparison

```
./bin/decompress -ilsb Testing_DIR/test_lsb.mp4  -imsb Testing_DIR/test_msb.mp4 -hd Testing_DIR/video_head_file.txt -o Testing_DIR -print_psnr 0
```

### Decompress each raw file seperately

```
ffmpeg -i Testing_DIR/test_lsb_5000_out.mp4 -i Testing_DIR/test_msb_5001_out.mp4 -map 0:v -c:v rawvideo -pix_fmt yuv420p Testing_DIR/output_lsb_5000.mkv -map 1:v -c:v rawvideo -pix_fmt yuv420p Testing_DIR/output_msb_5001.mkv
```


# Directions for Configuring Server Client System

# Hardware Requirements.
Server computer should have more CPU cores for every client (Recommend for 3 or more systems should be at least 6th gen i5 desktop CPU or equivilant)

Client computers can be found for cheap if used (around $150 or less) for example (a used HP EliteDesk 800 G2 Mini Business Desktop with an i5-6500T is sufficiently powerful enough.)

Our configuration:
    Server (1):
        HP EliteDesk 800 G2 Mini Business Desktop with 16GB RAM and 5TB external drive
    
    Clients (multiple):
        HP EliteDesk 800 G2 Mini Business Desktop with 8GB RAM, and 128GB SSD     

Depth Cameras:
    Intel&reg; RealSense&trade; D435 Stereo Depth Cameras

Ubuntu 20.04 LTS is installed on each computer. See more configuration settings in the README.md document


## First please configure for static IP addresses:

### On server
Go to Settings -> Network -> Wired -> [Click on Settings Symbol] -> IPv4 -> Manual:

    Then enter the following into their respective boxes
    In Addresses section
        Address: 192.168.1.234
        Netmask: 255.255.255.0
        Gateway: 192.168.1.1
    In DNS section:
        8.8.4.4,8.8.8.8
    Then hit apply

**NOTE IF YOU HAVE ANY FIREWALL SETTINGS **

1) Open a terminal

2) Type in 

```
sudo ufw status verbose
```
3) You will see either an active status or inactive status

4) If you have an active status add rules so you can accept your client computer cameras

5) Type the following into a terminal 
```
sudo ufw allow proto tcp 192.168.1.201 to any port 5000 && sudo ufw allow proto tcp 192.168.1.201 to any port 5001 && \
sudo ufw allow proto tcp 192.168.1.202 to any port 5002 && sudo ufw allow proto tcp 192.168.1.202 to any port 5003 && \
sudo ufw allow proto tcp 192.168.1.203 to any port 5004 && sudo ufw allow proto tcp 192.168.1.202 to any port 5004 && \
.
.
.
...

```
If need be you can remove all the firewall rules by doing

```
sudo ufw status verbose
```

Looking at the number in the list at which the rule occurs at

```
sudo ufw delete [x] # Where x is the number in the list at which the rule occurs at which you want to delete
```
Key generation

Type the following in to a terminal on your Server

```
cd ~/DepthCameraRecordingTool/Code_8_bit/ && \
make clean && make -j4 && \
python3 make_ssl_keys_cert.py
```

### On Client computers

Go to Settings -> Network -> Wired -> [Click on Settings Symbol] -> IPv4 -> Manual:
    Then enter the following into their respective boxes
    In Addresses section
        Address: 192.168.1.2xx    (Here xx means any two digit number as long as it is unique and not 34)
        Netmask: 255.255.255.0
        Gateway: 192.168.1.1
    In DNS section:
        8.8.4.4,8.8.8.8
    Then hit apply
    

### Copy Entire directory from Server to each client

Copy entire ~/DepthCameraRecordingTool/  directory to storage medium of your choice.
Then For ***each*** client computer:
    Navigate on the client computer to your home directory using the Files application on Ubuntu
    Paste a copy of the DepthCameraRecordingTool in this folder.

Alternativley you could just open up an ssh session to each client and copy the directories over. (This however assumes you are connecting the computers to routers)


### For each Client 
```
cd ~/DepthCameraRecordingTool/Code_8_bit/ && \
make clean && \
make -j4
```

```
crontab -e
```
Then if prompted type in 1 and hit ENTER

Using the arrow keys go down to the very bottom of the file
Then add the following text replacing USER with your client username found if you go to settings and users:
```
@reboot sleep 10; export XAUTHORITY=/home/<USER>/.Xauthority; cd /home/<USER>/DepthCameraRecordingTool/Code_8_bit/ && python3 client_side.py &
```
Then hit the keys CTRL+X then Y then ENTER


Then create a new bash script:
```
sudo nano /etc/NetworkManager/dispatcher.d/70-wifi-wired-exclusive.sh
```

Then copy and paste (Paste using the shortcut CTRL+SHIFT+V) the following into 70-wifi-wired-exclusive.sh:

```
#!/bin/bash
export LC_ALL=C

enable_disable_wifi ()
{
    result=$(nmcli dev | grep "ethernet" | grep -w "connected")
    if [ -n "$result" ]; then
        nmcli radio wifi off
    else
        nmcli radio wifi on
    fi
}

if [ "$2" = "up" ]; then
    enable_disable_wifi
fi

if [ "$2" = "down" ]; then
    enable_disable_wifi
fi
```
Then hit the keys CTRL+X then Y then ENTER

Then run the following command:
```
sudo chmod a+rx /etc/NetworkManager/dispatcher.d/70-wifi-wired-exclusive.sh
```

Then reboot the system
```
sudo reboot
```

Repeat for each client

### Running Server

Go to Settings -> Network -> Wired -> [Click on Settings Symbol] -> IPv4 -> Manual:
    Then enter the following into their respective boxes
    In Addresses section
        Address: 192.168.1.234
        Netmask: 255.255.255.0
        Gateway: 192.168.1.1
    In DNS section:
        8.8.4.4,8.8.8.8
    Then hit apply
    
Running a recording for 30 seconds at a crf of 22, with 4 clients connected
```
python3 server_side.py --dir Testing_DIR/ --num_clients 4 --json Default.json --time_run 30 --crf 22
```


You could also enter in your own json file as well for different settings just be sure that these files are also on the client computers.


# Directions Collecting_Training_Code/

### Running recording lossless compression with GUI

```
python3 collect_aligned_depth_training_vids.py
```

### Decompressing each frame to a TIF file GUI

```
python3 decompress_training_vids_gui.py
```
### Run collect_raw for 30 seconds using ffv1, with aligned frames from directory Collect_Training_Code/

make -j4 && ./bin/collect_raw -dir Testing_DIR/ -sec 30 -align 1 -jsonfile ../Code/Default.json


# Other Helpful Commands for Developers
To look at frame rates

```
ffprobe Testing_DIR/test_lossy.h265 -count_frames -show_entries stream=nb_read_frames,avg_frame_rate,r_frame_rate
```



To look at stats of a video please see the following from (https://ffmpeg.org/ffprobe-all.html)

```
ffprobe -f lavfi movie=Testing_DIR/test_lsb.mp4,signalstats -show_entries frame_tags=lavfi.signalstats.YMAX,lavfi.signalstats.YMIN,lavfi.signalstats.YAVG,lavfi.signalstats.YBITDEPTH,lavfi.signalstats.YDIF
```
Automate GDB

```
for i in {1..20}; do gdb -q -ex 'set pagination off' -ex 'set args -dir Testing_DIR/ -sec 15 -numraw 450 -crf 17 -bagfile ~/Downloads/One_Mouse_5_minutes_depth.bag' -ex run  ./bin/multicam -ex quit; done
```

Play videos side by side ffplay

```
ffplay -f lavfi "movie=left.mp4,scale=iw/2:ih[v0];movie=right.mp4,scale=iw/2:ih[v1];[v0][v1]hstack"
```

See info ffmpeg
```
ffmpeg -h encoder=libx264
```
