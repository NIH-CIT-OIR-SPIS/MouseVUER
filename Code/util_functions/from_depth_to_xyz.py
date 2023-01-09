import numpy as np
import argparse
import cv2
import os
import random
import json
import matplotlib.pyplot as plt
import open3d as o3d
import imageio.v3 as iio
import traceback
import time


WRITE_SHOW = False
def from_xy_to_world(img_coords, depth_arr, intrinsics_json):
    # there is a pair of x, y values in img_coords = [(x1, y1), (x2, y2) ... (xn,yn)]
    with open(intrinsics_json, 'r') as f:
        intrinsics = json.load(f)
    world_coords = []

    st_time = time.time()
    print("start time ", st_time)
    for i in range(len(img_coords)):
        x, y = img_coords[i]
        depth = depth_arr[y, x]
        z0 = depth * intrinsics['depth_units'] 
        x_world = (x - intrinsics['ppx']) * z0 / intrinsics['fx']
        y_world = (y - intrinsics['ppy']) * z0 / intrinsics['fy']
        z_world = z0
        world_coords.append((x_world, y_world, z_world))
    if WRITE_SHOW:
        height, width = depth_arr.shape
        length = height*width
        jj = np.tile(range(width), height)
        ii = np.repeat(range(height), width)
        z = depth_arr.flatten()* intrinsics['depth_units']
        world_coords_comp = np.dstack([(ii - intrinsics['ppx']) * z / intrinsics['fx']
                                    , (jj - intrinsics['ppy']) * z / intrinsics['fy'], 
                                    z]).reshape((length, 3))
        




        depth_instensity = np.array(256 * depth_arr / 0x0fff,
                                dtype=np.uint8)
        iio.imwrite('grayscale.png', depth_instensity)
        # print("world_coords :", world_coords)
        # print("world_coords_comp: ", world_coords_comp)
        pcd_o3d = o3d.geometry.PointCloud()  # create point cloud object
        pcd_o3d.points = o3d.utility.Vector3dVector(world_coords_comp)  # set pcd_np as the point cloud points
        # Visualize:
        o3d.visualization.draw_geometries([pcd_o3d])

    print("took ", time.time() - st_time, " seconds")
    return world_coords


def read_depth(depth_file):
    depth = iio.imread(depth_file)
    # change to uint16 
    depth = depth.astype(np.uint16)
    

    # print size and shape
    print("depth size: ", depth.size)
    print("depth shape: ", depth.shape)
    return depth

def main():
    # get intrinsics json file from user
    parser = argparse.ArgumentParser()
    help_str = "image coordinates typed in like  \"(x1, y1);(x2, y2); ... ;(xn,yn) \" "
    parser.add_argument("-cal", "--intrinsics", help="intrinsics json file", required=True)
    parser.add_argument("-d", "--depth", help="depth image file", required=True)
    parser.add_argument("-l", "--img_coords", help=help_str, required=False, type=str)
    # parse the arguments for img_coords
    args = parser.parse_args()
    img_coords = "".join(args.img_coords.split())
    # remove all parenthesis
    try:
        
        img_coords = img_coords.replace("(", "")
        img_coords = img_coords.replace(")", "")
        # Get the coordinates
        img_coords = img_coords.replace(";", ",")
        img_store = img_coords.split(",")
        img_store = [int(i) for i in img_store]
        
        if len(img_store) % 2 != 0:
            print(help_str)
            print('egg')
            exit()
        # group into pairs with img_coords[0] = [(img_store[i], img_store[i+1])]
        img_coords = []
        for i in range(0, len(img_store), 2):
            img_coords.append((img_store[i], img_store[i+1]))

        
    except Exception as e:
        print(help_str)
        exit()
    #img_coords = [(img_coords[i], img_coords[i+1]) for i in range(0, len(img_coords), 2)]
    print(img_coords)
    depth = read_depth(args.depth)

    for i in img_coords:
        if i[0] > depth.shape[1] or i[1] > depth.shape[0] or i[0] < 0 or i[1] < 0:
            print("image coordinates out of bounds")
            exit()
    #choose random image coords
    # img_coords = []
    # for i in range(6):
    #     x = random.randint(0, 1280-1)
    #     y = random.randint(0, 720-1)
    #     img_coords.append((x, y))
    world_coords = from_xy_to_world(img_coords, depth, args.intrinsics)
    print(world_coords)

if __name__ == "__main__":
    main()



