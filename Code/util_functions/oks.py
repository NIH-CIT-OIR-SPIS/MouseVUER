import numpy as np
import pandas as pd
import json
import math
import os
import argparse
import glob


def oks(y_true: np.ndarray, y_pred: np.ndarray, area: np.ndarray):
    # You might want to set these global constant
    # outside the function scope
    # calculate std_dev for that keypoint
    
    # The object scale
    # You might need a dynamic value for the object scale

    # The standard deviation
    assert y_true.shape == y_pred.shape
    assert area.shape[0] == y_true.shape[0]
    std_dev = np.std(y_true - y_pred)
    visibility = np.ones((y_true.shape[0], 1))
    # Compute euclidean distances
    distances = np.linalg.norm(y_pred - y_true, axis=-1)
    
    # OKS = exp(-1 * (d^2) / (2 * (s^2) * (k^2))
    # s is merely the sqrt of the area so doing s^2 is the same as doing area
    # k is the standard deviation between the two points
    exp_vector = np.exp(-(distances**2) / (2 * (area) * (std_dev**2)))

    return np.dot(exp_vector, visibility) / np.sum(visibility)



# def find_std_dev(vect_1, vect_2):
#     # vect_of is numpy matrix with x coordinates and y coordinates
#     # 2 columns, 1 row per point

#     euclidean_dist = np.sqrt( (vect_1[:,0] - vect_2[:,0])**2 + (vect_1[:,1] - vect_2[:,1])**2 )
#     return np.std(euclidean_dist)

# def find_std_dev_all(dict_keypoints_to_coords, dict_keypoints_to_coords2):
#     # dict_keypoints_to_coords is a dictionary with keypoint names as keys
#     # and numpy matrices as values
#     # each matrix is a list of x,y coordinates
#     # 2 columns, 1 row per point
#     std_devs = {}
#     for key in dict_keypoints_to_coords:
#         std_devs[key] = find_std_dev(dict_keypoints_to_coords[key], dict_keypoints_to_coords2[key])
#     # std_devs is a dictionary with keypoint names as keys
#     # The values are tuples of the form (x_std_dev, y_std_dev)
#     return std_devs

# def calc_oks_step(area_bounding_box, xycoord_1, xycoord_2, std_dev_xy):
#     # xycoord_1 and xycoord_2 are tuples of the form (x,y)
#     # std_dev_xy is a tuple of the form (x_std_dev, y_std_dev)
#     # returns a float

#     # calculate euclidean distance between xycoord_1 and xycoord_2
#     euclidean_dist = math.sqrt( (xycoord_1[0] - xycoord_2[0])**2 + (xycoord_1[1] - xycoord_2[1])**2 )
#     s_i = math.sqrt(area_bounding_box)
#     k_i = std_dev_xy
#     return math.exp( -1 * (euclidean_dist**2) / (2 * (s_i * k_i)**2) )


# def calc_oks(dict_list, data_name, data_name2):

#     # dict_list is a list of dictionaries each dictionary is organized as follows
#     # dict_list[i] = {'image_path': <path to image str>, 'keypoints': <list of keypoints>, 'keypoints_coords': <list of keypoints coordinates first labeler> , 
#     # 'keypoints_coords2': <list of keypoints coordinates second labeler>, 'area': <area of bounding box float>}

#     # for i in range(len(dict_list)):
#     #     # find standard deviation for each keypoint
#     #     dict_list[i]['keypoints_std_dev'] = find_std_dev_all(dict_list[i])
    
#     # calculate oks

#     # using find_std_dev get the standard deviation for each keypoint
#     for key in data_name:
#         data_name[key] = np.array(data_name[key])
#         data_name2[key] = np.array(data_name2[key])
#     std_devs = list(find_std_dev_all(data_name, data_name2).values())
#     print(std_devs)
        

#     oks = 0
#     for i in range(len(dict_list)):
#         oks = 0
#         for j in range(len(dict_list[i]['keypoints'])):
#             oks += calc_oks_step(dict_list[i]['area'], dict_list[i]['keypoints_coords'][j], dict_list[i]['keypoints_coords2'][j], std_devs[j]) / 4

#     # oks /= len(dict_list)
    
#     return oks



def parse_data_json(json_dict):
    data_hold = {}

    data_hold['filename'] = json_dict['image']['filename']

    keys = json_dict['annotations'][0]['skeleton']['nodes']

    data_hold['keypoints_coords'] = {}
    for i in range(len(keys)):
        data_hold['keypoints_coords'][keys[i]['name']] = [keys[i]['x'], keys[i]['y']]


    keys = json_dict['annotations'][1]['skeleton']['nodes']
    data_hold['keypoints_coords2'] = {}
    for i in range(len(keys)):
        data_hold['keypoints_coords2'][keys[i]['name']] = [keys[i]['x'], keys[i]['y']]
    jh = json_dict['annotations'][2]['bounding_box']
    data_hold['area'] =jh['w'] * jh['h']

    return data_hold



def from_dict_to_np_dict(data_pts: dict):
    y_true = {}
    y_pred = {}
    # For each data point, get the keypoints and the area
    for i in range(len(data_pts)):
        for key, value in data_pts[i]['keypoints_coords'].items():
            if key not in y_true:
                y_true[key] = []
            y_true[key].append(value)
        for key, value in data_pts[i]['keypoints_coords2'].items():
            if key not in y_pred:
                y_pred[key] = []
            y_pred[key].append(value)

    # Convert the keypoints to numpy arrays
    for key in y_true.keys():
        y_true[key] = np.array(y_true[key])

    for key in y_pred.keys():
        y_pred[key] = np.array(y_pred[key])

    return y_true, y_pred


def load_from_json_files(folder_path):

    json_files_lst = glob.glob(folder_path + '/*.json')
    data_pts = []
    our_data = {}
    for json_file in json_files_lst:

        try:
            with open(json_file, 'r') as f:
                data = json.load(f)

            data_pts.append(parse_data_json(data))
        except Exception as e:
            print("ERROR reading in file {}: {}".format(json_file, e))
            continue
    
    # Sort the data_pts by filename
    data_pts.sort(key=lambda x: x['filename'])
    return data_pts




def parse_args():
    parser = argparse.ArgumentParser(description='Calculate Oks')
    parser.add_argument('--json_path', help='directory of json files', default='data', type=str)
    args = parser.parse_args()
    return args
    


if __name__ == "__main__":
    # load data from json files

    args = parse_args()
    dict_list = load_from_json_files(args.json_path)
    y_true, y_pred = from_dict_to_np_dict(dict_list)
    area_np = []
    for i in range(len(dict_list)):
        area_np.append(dict_list[i]['area'])
    area_np = np.array(area_np)
    oks_results = {}
    for key in y_true.keys():
        oks_results[key] = oks(y_true[key], y_pred[key], area_np)
    print(oks_results)