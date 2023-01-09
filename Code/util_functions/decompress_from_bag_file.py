import pyrealsense2 as rs
import numpy as np
import cv2
import os
import argparse
import time
import traceback
from PIL import Image
import imageio.v3 as iio
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
    pipeline = rs.pipeline()


    config = rs.config()
    rs.config.enable_device_from_file(config, bag_file, repeat_playback=False)

    config.enable_stream(rs.stream.depth, rs.format.z16, 30)
    config.enable_stream(rs.stream.color, rs.format.rgb8, 30)
    config.enable_stream(rs.stream.infrared, 1, 1280, 720,  rs.format.y8, 30)
    # Start streaming from file
    profile = pipeline.start(config)
    playback=profile.get_device().as_playback()
    playback.set_real_time(False)

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
            frames = pipeline.wait_for_frames(1000)
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
                depth_image = np.array(aligned_depth_frame.get_data(), dtype=np.uint16)
                # save to image array 
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

# #####################################################
# ##               Read bag from file                ##
# #####################################################


# # First import library
# import pyrealsense2 as rs
# # Import Numpy for easy array manipulation
# import numpy as np
# # Import OpenCV for easy image rendering
# import cv2
# # Import argparse for command-line options
# import argparse
# # Import os.path for file path manipulation
# import os.path

# # Create object for parsing command-line options
# parser = argparse.ArgumentParser(description="Read recorded bag file and display depth stream in jet colormap.\
#                                 Remember to change the stream fps and format to match the recorded.")
# # Add argument which takes path to a bag file as an input
# parser.add_argument("-i", "--input", type=str, help="Path to the bag file")
# # Parse the command line arguments to an object
# args = parser.parse_args()
# # Safety if no parameter have been given
# if not args.input:
#     print("No input paramater have been given.")
#     print("For help type --help")
#     exit()
# # Check if the given file have bag extension
# if os.path.splitext(args.input)[1] != ".bag":
#     print("The given file is not of correct file format.")
#     print("Only .bag files are accepted")
#     exit()
# try:
#     # Create pipeline
#     pipeline = rs.pipeline()

#     # Create a config object
#     config = rs.config()

#     # Tell config that we will use a recorded device from file to be used by the pipeline through playback.
#     rs.config.enable_device_from_file(config, args.input, repeat_playback=False)
#     # Configure the pipeline to stream the depth stream
#     # Change this parameters according to the recorded bag file resolution
#     config.enable_stream(rs.stream.depth, rs.format.z16, 30)
#     config.enable_stream(rs.stream.color, rs.format.rgb8, 30)
#     config.enable_stream(rs.stream.infrared, 1, 1280, 720,  rs.format.y8, 30)
#     # Start streaming from file
#     profile = pipeline.start(config)
#     playback=profile.get_device().as_playback()
#     playback.set_real_time(True)

#     align_to = rs.stream.color
#     align = rs.align(align_to)
#     # Create opencv window to render image in
#     cv2.namedWindow("Depth Stream", cv2.WINDOW_AUTOSIZE)
    
#     # Create colorizer object
#     colorizer = rs.colorizer()

#     # Streaming loop
#     while True:
#         # Get frameset of depth
#         frames = pipeline.wait_for_frames()

#         # Get depth frame
#         depth_frame = frames.get_depth_frame()

#         # Colorize depth frame to jet colormap
#         depth_color_frame = colorizer.colorize(depth_frame)

#         # Convert depth_frame to numpy array to render image in opencv
#         depth_color_image = np.asanyarray(depth_color_frame.get_data())

#         # Render image in opencv window
#         cv2.imshow("Depth Stream", depth_color_image)
#         key = cv2.waitKey(1)
#         # if pressed escape exit program
#         if key == 27:
#             cv2.destroyAllWindows()
#             break

# finally:
#     pass