import pyrealsense2 as rs

import numpy as np
import cv2
import os
import argparse
import time

import traceback
from PIL import Image

def make_sep_ir_rgb_depth(bag_file: str, dir: str):
    if not os.path.isfile(bag_file):
        print("Bag file does not exist")
        return
    # Check if directory exists
    if not os.path.isdir(dir):
        os.mkdir(dir)
    rgb_dir = os.path.join(dir, "rgb")
    depth_dir = os.path.join(dir, "depth")
    ir_dir = os.path.join(dir, "ir")
    if not os.path.isdir(rgb_dir):
        os.mkdir(rgb_dir)
    if not os.path.isdir(depth_dir):
        os.mkdir(depth_dir)
    if not os.path.isdir(ir_dir):
        os.mkdir(ir_dir)

    # Create pipeline
    pipeline = rs.pipeline()
    config = rs.config()
    rs.config.enable_device_from_file(config, bag_file, repeat_playback=False)
    # Start streaming

    # Enable all streams

    config.enable_stream(rs.stream.depth, 1280, 720, rs.format.z16, 30)
    config.enable_stream(rs.stream.color, 1280, 720, rs.format.bgr8, 30)
    config.enable_stream(rs.stream.infrared, 1, 1280, 720,  rs.format.y8, 30)
    profile = pipeline.start(config)
    playback=profile.get_device().as_playback() # get playback device
    playback.set_real_time(True) # disable real-time playback
    # End at end of stream
    # Create an align object
    # rs.align allows us to perform alignment of depth frames to others frames
    # The "align_to" is the stream type to which we plan to align depth frames.
    align_to = rs.stream.color
    align = rs.align(align_to)

    # Streaming loop

    # starting time
    start = time.time()
    # Get file duration
    print("File duration: ", playback.get_duration())
    print("Starting time: ", start)
    # start pipeline

    try:
        frm_nm = 0
        while True:

            # Get frameset of color and depth
            frames = pipeline.wait_for_frames()
            if not frames:
                print("No more frames")
                break
            # frames.get_depth_frame() is a 640x360 depth image
            frm_nm += 1
            # Align the depth frame to color frame
            aligned_frames = align.process(frames)

            # Get aligned frames
            aligned_depth_frame = aligned_frames.get_depth_frame()
            color_frame = aligned_frames.get_color_frame()
            ir_frame = frames.get_infrared_frame()


            if aligned_depth_frame:
                # save as np.uint16
                depth_image = np.asanyarray(aligned_depth_frame.get_data())
                # save full np.uint16 to tif
                cv2.imwrite(os.path.join(depth_dir, "depth_frm_" + str(frm_nm) + ".tif"), depth_image)
            
            if color_frame:
                # Convert images to numpy arrays
                color_image = np.asanyarray(color_frame.get_data())
                # save as np.uint8
                cv2.imwrite(os.path.join(rgb_dir, "rgb_frm_" + str(frm_nm) + ".png"), color_image)
            
            if ir_frame:
                # Convert images to numpy arrays
                ir_image = np.asanyarray(ir_frame.get_data())
                # save as np.uint8
                cv2.imwrite(os.path.join(ir_dir, "ir_frm_" + str(frm_nm) + ".png"), ir_image)
    except:
        traceback.print_exc()


    finally:
        pipeline.stop()

    # time_ellapsed
    print("Time ellapsed: ", time.time() - start)



            

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--file", help="bag file name", required=True)
    parser.add_argument("-o", "--output", help="output folder", required=True)
    args = parser.parse_args()
    make_sep_ir_rgb_depth(args.file, args.output)

if __name__ == "__main__":
    main()

