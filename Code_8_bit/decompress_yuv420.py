import os
import subprocess as sp
import numpy
import cv2
import shlex
import argparse

def run_decompress_ffmpeg_cmd(lsb_file: str, msb_file: str, time_start_sec: int, time_duration_sec: int, output_dir: str):
    num_vframes = time_duration_sec * 30
    basename_get_lsb = os.path.basename(lsb_file)[:-4]
    basename_get_msb = os.path.basename(msb_file)[:-4]
    raw_lsb = os.path.join(output_dir, basename_get_lsb + "_raw.mkv")
    raw_msb = os.path.join(output_dir, basename_get_msb + "_raw.mkv")
    cmd_str = "ffmpeg -y -ss {:d} -i {:s} -i {:s} -map 0:v -c:v rawvideo -pix_fmt yuv420p -vframes {:d} {:s} -map 1:v -c:v rawvideo -pix_fmt yuv420p-vframes {:d} {:s}".format(time_start_sec, lsb_file, msb_file, num_vframes, raw_lsb, num_vframes, raw_msb)
    # if os.path.exists(raw_lsb) or os.path.exists(raw_msb):
    #     ans = input("File already exists. Do you wish to overwrite Y/N?")
    #     if ans == "Y" or ans == "y":
    #         sp.call(shlex.split(cmd_str))
    #     else:
    #         print("Aborting")
    #         return

    try:
        sp.call(shlex.split(cmd_str))
    except Exception as e:
        print("Error in running command: {}".format(e))
    #ffmpeg -y -ss 60 -i Testing_DIR/test_lsb_5000_out.mp4 -i Testing_DIR/test_msb_5001_out.mp4 -map 0:v -c:v rawvideo -pix_fmt yuv420p  -vframes 600 Testing_DIR/output_lsb_5000.mkv -map 1:v  -c:v rawvideo -pix_fmt yuv420p -vframes 600 Testing_DIR/output_msb_5001.mkv

def parse_cmd():
    parser = argparse.ArgumentParser(description="Decompress YUV420 video")
    
    parser.add_argument("-l", "--lsb", help="Path to LSB video", required=True)
    parser.add_argument("-m", "--msb", help="Path to MSB video", required=True)
    parser.add_argument("-s", "--start", help="Start time in seconds", required=True)
    parser.add_argument("-d", "--duration", help="Duration in seconds", required=True)
    parser.add_argument("-o", "--output", help="Output directory", required=True)

    # Print help if no arguments are provided or if -h or --help is provided

    args = parser.parse_args()
    return args

def main():
    args = parse_cmd()
    run_decompress_ffmpeg_cmd(args.lsb, args.msb, int(args.start), int(args.duration), args.output)

if __name__ == "__main__":
    main()