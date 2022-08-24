#!/usr/bin/python3
import os
import argparse
import numpy as np
import subprocess as sp
import multiprocessing
import time
import shlex
import signal 
import sys
import platform

HEIGHT = 720
WIDTH = 1280

HEIGHT_RGB = int(HEIGHT / 2)
WIDTH_RGB = int(WIDTH / 2)

def run_processes_parallel(cmd_lst):
    """
    Start up two server processes in parallel. Send them both signals to terminate.
    """
    try:
        p_list = []
        for cmd in cmd_lst:
            p_list.append(sp.Popen(cmd, shell=True))
        
        # Wait for the processes to terminate
        for p in p_list:
            p.wait()
    
    # Send the processes a signal to terminate
    finally:
        if p_list is not []:
            for p in p_list:
                p.send_signal(signal.SIGQUIT)

def extract_frame_command(input_vid, frame_num, output_dir):
    if frame_num <= 0:
        command = 'ffmpeg -loglevel error -nostats -y -i {:s} {:s}/frame_%06d.tif'.format(input_vid, output_dir)
    else:
        command = 'ffmpeg -loglevel error -nostats -y -i {:s} -vframes {:d} {:s}/depth_frame_%06d.tif'.format(input_vid, frame_num, output_dir)
    #sp.call(command, shell=True)
    return command

def extract_rgb_frame_command(input_vid_mp4, frame_num, output_dir):
    if frame_num <= 0:
        command = 'ffmpeg -loglevel error -nostats -y -i {:s} {:s}/rgb_frame_%06d.tif'.format(input_vid_mp4, output_dir)
    else:
        command = 'ffmpeg -loglevel error -nostats -y -i {:s} -vframes {:d} {:s}/rgb_frame_%06d.tif'.format(input_vid_mp4, frame_num, output_dir)
    return command

def options():
    parser = argparse.ArgumentParser(description='Extract frames from video')
    parser.add_argument('-i', '--input', help='Input video', required=True)
    parser.add_argument('-irgb', '--input_rgb', help='Input video', required=True)
    parser.add_argument('-o', '--output', help='Output directory', required=True)
    parser.add_argument('-f', '--frame', help='Number of frames to uncompress, input <=0 to decompress all frames', required=True)
    return parser.parse_args()


def parallel_call(input_vid, input_vid_mp4, output_dir, frame_num):
    cmd_lst = []
    cmd_lst.append(extract_frame_command(input_vid, frame_num, output_dir))
    cmd_lst.append(extract_rgb_frame_command(input_vid_mp4, frame_num, output_dir))
    p1 = multiprocessing.Process(target=run_processes_parallel, args=(cmd_lst,))
    p1.start()
    try:
        p1.join()
    except Exception as e:
        p1.terminate()
        p1.join()
        print("Error here: {}".format(e))


def main():
    args = options()
    input_vid = args.input
    input_vid_mp4 = args.input_rgb
    output_dir = args.output
    frame_num = int(args.frame)
    parallel_call(input_vid, input_vid_mp4, output_dir, frame_num)
        #pass
    #run_processes_parallel(cmd_lst)
    if platform.system() == 'Linux':
        os.system("stty echo")


if __name__ == '__main__':
    print('Extracting frames')
    main()
    print('Done')