#!/usr/bin/env python3
import numpy as np
from PIL import Image
import os
import cv2
import glob
import argparse


def make_video_from_images(file_names, output_file_name, fps=30):
    """
    :param file_names: list of file names of images
    :param output_file_name: output file name of video
    :param fps: frames per second
    :return: None
    """
    # get the shape of images
    img = cv2.imread(file_names[0])
    height, width, layers = img.shape

    # define the codec and create VideoWriter object
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    video = cv2.VideoWriter(output_file_name, fourcc, fps, (width, height))

    # write the video
    for file_name in file_names:

        img_n = cv2.imread(file_name)
        # Normalize the image
        img_n = cv2.normalize(img_n, None, alpha=0, beta=255, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_8U)

        video.write(img_n)

    
    # release everything if job is finished
    video.release()
    cv2.destroyAllWindows()



def main():
    # get the file names

    parser = argparse.ArgumentParser()
    parser.add_argument('--input_dir', type=str, default='images', help='input directory of images')
    parser.add_argument('--output_file_name', type=str, default='video.mp4', help='output file name of video')
    parser.add_argument('--fps', type=int, default=30, help='frames per second')
    args = parser.parse_args()
    file_names = glob.glob(os.path.join(args.input_dir, '*.png'))
    file_names.sort()
    # make the video
    make_video_from_images(file_names, args.output_file_name, args.fps)

if __name__ == "__main__":
    main()
