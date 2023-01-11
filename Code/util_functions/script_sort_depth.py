import numpy as np
import pandas as pd
import glob
import os
import shutil
import argparse
import natsort
import cv2
from PIL import Image
def sort(dir):
    # get all files in dir
    # get absolute path of dir
    dir = os.path.abspath(dir)

    files = glob.glob(os.path.join(dir, "*.png"))
    # get all file names
    file_names = [os.path.basename(file) for file in files]
    folder_name = os.path.basename(dir)
    parent_name = os.path.dirname(dir)
    # replace all occurences of ':' in file names with '_'
    #new_file_names = [file_name.replace(':', '_') for file_name in file_names]

    # Make new directory in diretory just above this one
    os.umask(0)
    os.chmod(dir, 0o777)
    sorted_dir = os.path.join(parent_name, folder_name + "sorted")
    if not os.path.exists(sorted_dir):
        os.mkdir(sorted_dir)
    
    sorted_depth_dir = os.path.join(sorted_dir, "depth")
    sorted_color_dir = os.path.join(sorted_dir, "color")
    sorted_ir_dir = os.path.join(sorted_dir, "infra")
    if not os.path.exists(sorted_depth_dir):
        os.mkdir(sorted_depth_dir)
    if not os.path.exists(sorted_color_dir):
        os.mkdir(sorted_color_dir)
    if not os.path.exists(sorted_ir_dir):
        os.mkdir(sorted_ir_dir)
    
    # For every file_name in file_names that starts with depth
    print("Copying files")

    for file_name in file_names:
        if file_name.startswith("depth"):
            new_file_name = file_name.replace(':', '_')
            shutil.copy2(os.path.join(dir, file_name), os.path.join(sorted_depth_dir, new_file_name))
        elif file_name.startswith("color"):
            new_file_name = file_name.replace(':', '_')
            shutil.copy2(os.path.join(dir, file_name), os.path.join(sorted_color_dir, new_file_name))
        elif file_name.startswith("infra"):
            new_file_name = file_name.replace(':', '_')
            shutil.copy2(os.path.join(dir, file_name), os.path.join(sorted_ir_dir, new_file_name))
        else:
            print("Could not find file type for file: ", file_name)
    print("Done copying files")

def find_color(color_dir, other_dir):
    # find matching
    dir = os.path.abspath(color_dir)

    files = glob.glob(os.path.join(dir, "*.png"))
    # get all file names
    file_names = [os.path.basename(file) for file in files]
    folder_name = os.path.basename(dir)
    parent_name = os.path.dirname(dir)


    os.umask(0)
    os.chmod(dir, 0o777)
    sorted_dir = os.path.join(parent_name, folder_name + "sorted_nre")
    if not os.path.exists(sorted_dir):
        os.mkdir(sorted_dir)


    # sort alpha numerically
    file_names = natsort.natsorted(file_names)
    color_file_names = [file_name for file_name in file_names if file_name.startswith("color")]
    infra_file_names = [file_name for file_name in file_names if file_name.startswith("infra")]
    depth_file_names = [file_name for file_name in file_names if file_name.startswith("depth")]
    
    dir2 = os.path.abspath(other_dir)

    files2 = glob.glob(os.path.join(dir2, "*.png"))
    # get all file names
    file_names2 = [os.path.basename(file) for file in files2]
    folder_name2 = os.path.basename(dir2)
    parent_name2 = os.path.dirname(dir2)
    lst_pairs = []
    for file_name in color_file_names:
        # see if time stamps match
        time_stamp = parse_str_time(file_name)
        #print(time_stamp)
        for file_name2 in file_names2:
            if file_name2.startswith("depth"):
                #print("hi")
                time_stamp2 = parse_str_time(file_name2).replace(':', '_')
                #print("{}, {}".format(time_stamp, time_stamp2))
                if time_stamp == time_stamp2:
                    # copy file
                    print(file_name2)
                    os.rename(os.path.join(dir2, file_name2), os.path.join(dir2, file_name2.replace(':', '_')))
                    file_name2 = file_name2.replace(':', '_')
                    shutil.copy2(os.path.join(dir2, file_name2), os.path.join(sorted_dir, file_name2))
                    pathe = os.path.join(dir, file_name)
                    path2 = os.path.join(sorted_dir, file_name2)
                    lst_pairs.append((pathe, path2))
    
    print("Done copying files")
    print("Read in images and visualize them overlayed")
    # start images cv2
    for i in range(len(lst_pairs)):
        
        # Overlay and show images
        background = Image.open(lst_pairs[i][0])
        overlay = cv2.imread(lst_pairs[i][1], cv2.IMREAD_UNCHANGED)

        # normalize between 0 255

        overlay = cv2.normalize(overlay, None, 0, 255, cv2.NORM_MINMAX, cv2.CV_8U)

        overlay = Image.fromarray(overlay)
        overlay = overlay.convert("RGBA")
        background = background.convert("RGBA")
        new_img = Image.blend(background, overlay, 0.5)
        # Show image
        back_to_cv = np.array(new_img)

        cv2.imshow("image", back_to_cv)
        
        cv2.waitKey(0)
    cv2.destroyAllWindows()

    
        



def parse_str_time(tm):
    return (tm.split(" ")[1]).split(".png")[0]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--folder", help="folder name color images", required=True)
    parser.add_argument("-f2", "--folder2", help="folder name all images", required=True)
    args = parser.parse_args()
    find_color(args.folder, args.folder2)

if __name__ == "__main__":
    main()


    

    


