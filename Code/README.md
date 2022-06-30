
# Depth Recording Tool

### Authors
Noah Cubert, Ghadi Salem

Uses Intel&reg; RealSense&trade; depth camera software, as well as FFmpeg to record data from cameras

## Install with linux

This is a guide on how to run the program 
To install with linux please follow directions here https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_linux.md

Install opencv with the following directions
https://linuxize.com/post/how-to-install-opencv-on-ubuntu-20-04/

Install ffmpeg and build following the directions seen here:
https://www.reddit.com/r/ffmpeg/comments/7zjoi3/compiling_ffmpeg_on_ubuntu_with_multilib_x265/


## Install FFmpeg
```
sudo apt install ffmpeg
```

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
/bin/multicam -h
```

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

To decompress and visualize, please see subsequent Raw and Compressed Video Recordings, You can also record the PSNR or you recordings here when compared with RAW, should be only for developer use. Takes a long time to run.
```
python3 decompress_vids.py --vidmsb Testing_DIR/test_msb.mp4 --vidlsb Testing_DIR/test_lsb.mp4 --path_img decomp_pngs/ --vidraw Testing_DIR/
```

To look at stats of a video please see the following from (https://ffmpeg.org/ffprobe-all.html)
```
ffprobe -f lavfi movie=Testing_DIR/test_lsb.mp4,signalstats -show_entries frame_tags=lavfi.signalstats.YMAX,lavfi.signalstats.YMIN,lavfi.signalstats.YAVG,lavfi.signalstats.YBITDEPTH,lavfi.signalstats.YDIF
```


