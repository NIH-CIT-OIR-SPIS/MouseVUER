import numpy as np
import json
import argparse
import glob
import os


STORE_FILES = {}

def calc_oks(y_true: np.ndarray, y_pred: np.ndarray, area: np.ndarray, k_arr_np: np.ndarray):
    # The standard deviation
    global STORE_FILES
    assert y_true.shape == y_pred.shape
    print("area shape: {}".format(area.shape))

    print("y_true : {}".format(y_true))
    visibility = np.ones((y_true.shape[0], 1))
    # Compute euclidean distances
    distances = np.linalg.norm(y_pred - y_true)
    exp_vector = np.exp(-(distances**2) / (2 * (area) * (k_arr_np**2)))
    return np.dot(exp_vector, visibility) / np.sum(visibility)


def calc_std_dev(coords1: np.ndarray, coords2: np.ndarray):
    # The standard deviation
    # print("coords1", coords1)
    # print("coords2 ", coords2)
    std_dev = np.std(coords1 - coords2, axis=1)
    print("std dev2: {}".format(std_dev))
    return std_dev


def parse_data_json(json_dict):
    data_hold = {}
    data_hold['filename'] = json_dict['image']['filename']
    keys = json_dict['annotations'][0]['skeleton']['nodes']
    data_hold['keypoints_coords'] = {}
    for i in range(len(keys)):
        data_hold['keypoints_coords'][keys[i]['name']] = [
            keys[i]['x'], keys[i]['y']]
    keys = json_dict['annotations'][1]['skeleton']['nodes']
    data_hold['keypoints_coords2'] = {}

    for i in range(len(keys)):
        data_hold['keypoints_coords2'][keys[i]['name']] = [
            keys[i]['x'], keys[i]['y']]
    jh = json_dict['annotations'][2]['bounding_box']
    data_hold['area'] = jh['w'] * jh['h']
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
    parser.add_argument('-j','--json_path', help='directory of json files', default='data', type=str)
    parser.add_argument('-k','--kappa_path_json', help='kappa value path to either write to or to read from', default="data", type=str)
    args = parser.parse_args()
    return args


if __name__ == "__main__":
    # load data from json files

    args = parse_args()
    
    dict_list = load_from_json_files(args.json_path)
    # y_true, y_pred = from_dict_to_np_dict(dict_list)
    area_np = []
    for i in range(len(dict_list)):
        area_np.append(dict_list[i]['area'])
    area_np = np.array(area_np)
    oks_results = {}

    kappa_vals = []
    
    optional_kappas_dict = None
    if os.path.exists(args.kappa_path_json):
        print("hi")
        with open(args.kappa_path_json, 'r') as f:
            optional_kappas_dict = json.load(f)
        for key in optional_kappas_dict.keys():
            kappa_vals.append(optional_kappas_dict[key]['kappa'])
    else:
        optional_kappas_dict = {}
        # Calculate variances for each keypoint over all images
        y_true, y_pred = from_dict_to_np_dict(dict_list)

        for key in y_true.keys():
            distances = np.linalg.norm(y_pred[key] - y_true[key])
            variances = np.mean(distances ** 2 / area_np)
            k_i = 2*np.sqrt(variances)
            kappa_vals.append(k_i)
            optional_kappas_dict[key] = {}
            optional_kappas_dict[key]['kappa'] = k_i
            optional_kappas_dict[key]['variance'] = variances




    # for each image in the json files
    kappa_vals = np.array(kappa_vals)
    for i in range(len(dict_list)):
        # for each keypoint in the image
        y_true = {}
        y_pred = {}
        y_true_arr = []
        y_pred_arr = []
        for key, value in dict_list[i]['keypoints_coords'].items():
            if key not in y_true:
                y_true[key] = []
            y_true[key].append(value)
            y_true_arr.append(value)
        for key, value in dict_list[i]['keypoints_coords2'].items():
            if key not in y_pred:
                y_pred[key] = []
            y_pred[key].append(value)
            y_pred_arr.append(value)
        print("y_true", y_true)
        print("y_pred", y_pred)
        # stack all keys
        print("y_true_arr", y_true_arr)
        print("y_pred_arr", y_pred_arr)
        y_true_arr = np.array(y_true_arr)
        y_pred_arr = np.array(y_pred_arr)

        oks_results[dict_list[i]['filename']] = calc_oks(y_true_arr, y_pred_arr, area_np[i],  kappa_vals)[0]

    with open('oks_results_human.json', 'w') as f:
        json.dump(oks_results, f)

    
    # print("oks_results", oks_results)



    # for key in y_true.keys():
    #     oks_results[key] = calc_oks(y_true[key], y_pred[key], area_np, key, optional_kappas_dict)
    
    # if STORE_FILES is empty dict
    if not os.path.exists(args.kappa_path_json) and optional_kappas_dict is not None:
        with open(args.kappa_path_json, 'w') as f:
            json.dump(optional_kappas_dict, f)

    # # map_img_to_std_dev = {}
    # # for key in y_true.keys():
    # #     map_img_to_std_dev[key] = calc_std_dev(y_true[key], y_pred[key])
    # # # for img in dict_list:
    # # #     map_img_to_std_dev[img['filename']] = calc_std_dev(coords2=np.array(list(img['keypoints_coords2'].values())), coords1=np.array(list(img['keypoints_coords'].values())))
    # # print("map img to std dev: {}".format(map_img_to_std_dev))

    # print(oks_results)


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
