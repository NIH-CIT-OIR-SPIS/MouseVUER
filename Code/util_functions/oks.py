import numpy as np
import pandas as pd
import json
import math
import os
import argparse
import glob

def find_std_dev(vect_1, vect_2):
    # vect_of is numpy matrix with x coordinates and y coordinates
    # 2 columns, 1 row per point

    euclidean_dist = np.sqrt( (vect_1[:,0] - vect_2[:,0])**2 + (vect_1[:,1] - vect_2[:,1])**2 )
    return np.std(euclidean_dist)

def find_std_dev_all(dict_keypoints_to_coords, dict_keypoints_to_coords2):
    # dict_keypoints_to_coords is a dictionary with keypoint names as keys
    # and numpy matrices as values
    # each matrix is a list of x,y coordinates
    # 2 columns, 1 row per point
    std_devs = {}
    for key in dict_keypoints_to_coords:
        std_devs[key] = find_std_dev(dict_keypoints_to_coords[key], dict_keypoints_to_coords2[key])
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


def calc_oks(dict_list, data_name, data_name2):

    # dict_list is a list of dictionaries each dictionary is organized as follows
    # dict_list[i] = {'image_path': <path to image str>, 'keypoints': <list of keypoints>, 'keypoints_coords': <list of keypoints coordinates first labeler> , 
    # 'keypoints_coords2': <list of keypoints coordinates second labeler>, 'area': <area of bounding box float>}

    # for i in range(len(dict_list)):
    #     # find standard deviation for each keypoint
    #     dict_list[i]['keypoints_std_dev'] = find_std_dev_all(dict_list[i])
    
    # calculate oks

    # using find_std_dev get the standard deviation for each keypoint
    for key in data_name:
        data_name[key] = np.array(data_name[key])
        data_name2[key] = np.array(data_name2[key])
    std_devs = list(find_std_dev_all(data_name, data_name2).values())
    print(std_devs)
        

    oks = 0
    for i in range(len(dict_list)):
        oks = 0
        for j in range(len(dict_list[i]['keypoints'])):
            oks += calc_oks_step(dict_list[i]['area'], dict_list[i]['keypoints_coords'][j], dict_list[i]['keypoints_coords2'][j], std_devs[j])
        print(oks / 4)
    # oks /= len(dict_list)
    
    return oks




def load_from_json_files(folder_path):
    # json_path is a string
    # returns a list of dictionaries
    # 
    dict_list = []
    data_name = {}
    data_name2 = {}
    # get list of json files 
    json_files = glob.glob(folder_path + '/*.json')
    for json_file in json_files:
        data_hold = {}
        try:
            with open(json_file, 'r') as f:
                data = json.load(f)
            print(json_file)
            data_hold['image_path'] = os.path.join('images', json_file.replace('.json', '.jpg'))
            data_hold['keypoints'] = data['annotations'][0]['skeleton']['nodes']
            data_hold['keypoints_2'] = data['annotations'][1]['skeleton']['nodes']
            data_hold['keypoints_coords'] = []
            
            for i in range(len(data_hold['keypoints'])):
                if data_hold['keypoints'][i]['name'] not in data_name:
                    data_name[data_hold['keypoints'][i]['name']] = []
                else:
                    data_name[data_hold['keypoints'][i]['name']].append( [data_hold['keypoints'][i]['x'], data_hold['keypoints'][i]['y']] )

                data_hold['keypoints_coords'].append((data_hold['keypoints'][i]['x'], data_hold['keypoints'][i]['y']))
            data_hold['keypoints_coords2'] = []
            for i in range(len(data_hold['keypoints_2'])):
                if data_hold['keypoints_2'][i]['name'] not in data_name2:
                    data_name2[data_hold['keypoints_2'][i]['name']] = []
                else:
                    data_name2[data_hold['keypoints_2'][i]['name']].append( [data_hold['keypoints_2'][i]['x'], data_hold['keypoints_2'][i]['y']] )
                data_hold['keypoints_coords2'].append((data_hold['keypoints_2'][i]['x'], data_hold['keypoints_2'][i]['y']))
            data_hold['area'] = data['annotations'][2]['bounding_box']["w"] * data['annotations'][2]['bounding_box']["h"]
        except Exception as e:
            print(e)
            continue
        dict_list.append(data_hold)

    return dict_list, data_name, data_name2

def parse_args():
    parser = argparse.ArgumentParser(description='Calculate Oks')
    parser.add_argument('--json_path', help='directory of json files', default='data', type=str)
    args = parser.parse_args()
    return args
    


if __name__ == "__main__":
    # load data from json files

    args = parse_args()

    dict_list, data_name, data_name2 = load_from_json_files(args.json_path)
    print(data_name)
    print(data_name2)
    oks = calc_oks(dict_list, data_name, data_name2)
    print(oks)