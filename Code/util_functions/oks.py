import numpy as np
import pandas as pd
import json
import math

def find_std_dev(vect_1, vect_2):
    # vect_of is numpy matrix with x coordinates and y coordinates
    # 2 columns, 1 row per point
    euclidean_dist = np.sqrt( (vect_1[:,0] - vect_2[:,0])**2 + (vect_1[:,1] - vect_2[:,1])**2 )
    return np.std(euclidean_dist)

def find_std_dev_all(dict_keypoints_to_coords):
    # dict_keypoints_to_coords is a dictionary with keypoint names as keys
    # and numpy matrices as values
    # each matrix is a list of x,y coordinates
    # 2 columns, 1 row per point
    std_devs = {}
    for keypoint in dict_keypoints_to_coords:
        std_devs[keypoint] = find_std_dev(dict_keypoints_to_coords[keypoint])
    # std_devs is a dictionary with keypoint names as keys
    # The values are tuples of the form (x_std_dev, y_std_dev)
    return std_devs

def calc_oks_step(area_bounding_box, xycoord_1, xycoord_2, std_dev_xy):
    # xycoord_1 and xycoord_2 are tuples of the form (x,y)
    # std_dev_xy is a tuple of the form (x_std_dev, y_std_dev)
    # returns a float

    # calculate euclidean distance between xycoord_1 and xycoord_2
    euclidean_dist = math.sqrt( (xycoord_1[0] - xycoord_2[0])**2 + (xycoord_1[1] - xycoord_2[1])**2 )
    s_i = math.sqrt(area_bounding_box)
    k_i = std_dev_xy
    return math.exp( -1 * (euclidean_dist**2) / (2 * (s_i * k_i)**2) )


def calc_oks(dict_list):

    # dict_list is a list of dictionaries each dictionary is organized as follows
    # dict_list[i] = {'image_path': <path to image str>, 'keypoints': <list of keypoints>, 'keypoints_coords': <list of keypoints coordinates first labeler> , 
    # 'keypoints_coords2': <list of keypoints coordinates second labeler>, 'area': <area of bounding box float>}

    for i in range(len(dict_list)):
        # find standard deviation for each keypoint
        dict_list[i]['keypoints_std_dev'] = find_std_dev_all(dict_list[i]['keypoints_coords'], dict_list[i]['keypoints_coords2'])
    
    # calculate oks
    oks = 0
    for i in range(len(dict_list)):
        for j in range(len(dict_list[i]['keypoints'])):
            oks += calc_oks_step(dict_list[i]['area'], dict_list[i]['keypoints_coords'][j], dict_list[i]['keypoints_coords2'][j], dict_list[i]['keypoints_std_dev'][j])
    
    # oks /= len(dict_list)
    
    return oks




    # # dict_keypoints_to_coords is a dictionary with keypoint names as keys
    # # and numpy matrices as values
    # # each matrix is a list of x,y coordinates
    # # 2 columns, 1 row per point
    # # dict_keypoints_to_std_devs is a dictionary with keypoint names as keys
    # # and tuples of the form (x_std_dev, y_std_dev) as values
    # # area_bounding_box is a float
    # # returns a float

    # # for each keypoint, calculate the oks for that keypoint
    # # then average the oks for all keypoints
    # oks = 0
    # dict_std_dev
    # for keypoint in dict_keypoints_to_coords:

    # for keypoint in dict_keypoints_to_coords:
        
    #     oks += calc_oks_step(dict_keypoints_to_coords['area'], dict_keypoints_to_coords['a1'], dict_keypoints_to_coords['a2'], )
    # oks /= len(dict_keypoints_to_coords)
    # return oks

