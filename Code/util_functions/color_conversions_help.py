import numpy as np
import math
import random
import multiprocessing as mp

# [R G B]' = [[1 1 1]' [0 -0.16455312684366 1.8814]' [1.4746 -0.57135312684366]]
# K_B = 0.0593, K_R = 0.2627

# def convert_pixel_yuv444_to_rgb(yuv_np_pixel: np.ndarray) -> np.ndarray:
    
#     Y = yuv_np_pixel[0]
#     U = yuv_np_pixel[1]
#     V = yuv_np_pixel[2]

#     R = Y + 1.4746 * (V - 128)
#     G = Y - 0.16455312684366 * (U - 128) - 0.57135312684366 * (V - 128)
#     B = Y + 1.8814 * (U - 128)

#     return np.array([R, G, B])

# def convert_pixel_rgb_to_yuv444(rgb_np_pixel: np.ndarray) -> np.ndarray:
    
#     R = rgb_np_pixel[0]
#     G = rgb_np_pixel[1]
#     B = rgb_np_pixel[2]

#     Y = 0.2627 * R + 0.6780 * G + 0.0593 * B
#     U = 128 - 0.1482 * R - 0.2910 * G + 0.4392 * B
#     V = 128 + 0.4392 * R - 0.3678 * G - 0.0714 * B

#     return np.array([Y, U, V])

tform = np.array([[1.164, 0, 1.596], [1.164, -0.392, -0.813], [1.164, 2.017, 0]], dtype=np.float64)

tform_helper =np.array([[0.257, 0.504,  0.098], [-0.148, -0.291, 0.439], [0.439, -0.368, -0.071]], dtype=np.float)

def convert_pixel_yuv444_to_rgb(yuv_np_pixel: np.ndarray) -> np.ndarray:
    
    
    # Y = yuv_np_pixel[0]
    # U = yuv_np_pixel[1]
    # V = yuv_np_pixel[2]
    # Y -= 16
    # U -= 128
    # V -= 128
    # R = 1.164 * Y             + 1.596 * V
    # G = 1.164 * Y - 0.392 * U - 0.813 * V
    # B = 1.164 * Y + 2.017 * U

    # # round to nearest integer
    # R = round(R)
    # G = round(G)
    # B = round(B)


    # return np.round(np.clip(np.array([R, G, B]), 0, 255))


    # Translate above into linear algebra
    yuv_np_pixel[0] -= 16
    yuv_np_pixel[1] -= 128
    yuv_np_pixel[2] -= 128


    rgb_arr = np.matmul(tform, yuv_np_pixel)
    rgb_arr = np.round(np.clip(rgb_arr, 0, 255))
    return rgb_arr





def convert_pixel_rgb_to_yuv444(rgb_np_pixel: np.ndarray) -> np.ndarray:
    # R = rgb_np_pixel[0]
    # G = rgb_np_pixel[1]
    # B = rgb_np_pixel[2]
    # Y =  0.257 * R + 0.504 * G + 0.098 * B +  16
    # U = -0.148 * R - 0.291 * G + 0.439 * B + 128
    # V =  0.439 * R - 0.368 * G - 0.071 * B + 128


    # # round to nearest integer

    
    # return np.round(np.clip(np.array([Y, U, V]), 0, 255))

    # The linear algebra way
    #tform = np.array([[0.299, 0.587, 0.114], [-0.14713, -0.28886, 0.436], [0.615, -0.51499, -0.10001]], dtype=np.float32)
    
    yuv_arr = np.matmul(tform_helper, rgb_np_pixel)
    yuv_arr[0] += 16
    yuv_arr[1] += 128
    yuv_arr[2] += 128
    return np.round(np.clip(yuv_arr, 0, 255))
    

def convert_rgb_to_YCuCv(arr):
    R = arr[0]
    G = arr[1]
    B = arr[2]
    Y = math.floor((R + 2* G + B) / 4)
    CU = R - G
    CV = B - G
    Y = round(Y)
    CU = round(CU)
    CV = round(CV)

    return [Y, CU, CV]

def convert_YCuCv_to_rgb(arr):
    Y = arr[0]
    CU = arr[1]
    CV = arr[2]

    G = Y - math.floor((CU + CV) / 4)
    R = CU + G
    B = CV + G
    G = round(G)
    R = round(R)
    B = round(B)

    return [R, G, B]


def test_colorconv2():
    error_count = 0
    count = 0
    print("number of possibilities: " + str(256 * 256 * 256))

    # place 
    # parrallelize this:






    min_val = 1000000
    max_val = -1000000
    for r in range(256):
        for g in range(256):
            for b in range(256):
                rgb = [r, g, b]
                yuv = convert_rgb_to_YCuCv(rgb)
                if yuv[1] < min_val:
                    min_val = yuv[1]
                if yuv[1] > max_val:
                    max_val = yuv[1]
                rgb2 = convert_YCuCv_to_rgb(yuv)
                # if not rgb == rgb2:
                #     print("Error converting pixel: " + str(rgb))
                #     print("Got: " + str(rgb2))
                #     print("Expected: " + str(rgb))
                #     error_count += 1
                
                count += 1
                # if count % 100000 == 0:
                #     print("count: " + str(count) + " error_count: " + str(error_count))
                #     print("Got correct conversion: " + str(rgb) + " -> " + str(yuv) + " -> " + str(rgb2) + " -> " + str(yuv))
    print("count: " + str(count) + " error_count: " + str(error_count))
    print("min_max_val: " + str(min_val) + " " + str(max_val))


test_colorconv2()



# Go through every possible pixel value and make sure the conversion is correct
# def test_color_conversions():
#     error_count = 0
#     count = 0
#     print("number of possibilities: " + str(256 * 256 * 256))

#     # place 
#     # parrallelize this:





#     for r in range(256):
#         for g in range(256):
#             for b in range(256):
#                 rgb_np_pixel = np.array([r, g, b])
#                 yuv_np_pixel = convert_pixel_rgb_to_yuv444(rgb_np_pixel)
#                 rgb_np_pixel2 = convert_pixel_yuv444_to_rgb(yuv_np_pixel)
#                 # r
#                 if not np.array_equal(rgb_np_pixel, rgb_np_pixel2):
#                     # print("Error converting pixel: " + str(rgb_np_pixel))
#                     # print("Got: " + str(rgb_np_pixel2))
#                     # print("Expected: " + str(rgb_np_pixel))

#                     # diff_n = rgb_np_pixel - rgb_np_pixel2
#                     # diff_p = rgb_np_pixel2 - rgb_np_pixel

#                     # print("Difference: " + str(diff_n) + " or " + str(diff_p))

#                     error_count += 1
#                 count += 1
#                 if count % 100000 == 0:
#                     print("Checked " + str(count) + " pixels" + " with " + str(error_count) + " errors")

#     if error_count == 0:
#         print("All conversions correct!")
#     else:
#         print("Found " + str(error_count) + " errors")

#     return error_count
#     # The above serial version takes too long below is the parallelized version using multiprocessing


# THIS APPARENTLY DOES NOT WORK AND WILL ACT AS MALICIOUS CODE
# def test_color_conversions_helper(r):


#     error_count = 0
#     count = 0
#     print("number of possibilities: " + str(256 * 256 * 256))

#     # place 
#     # parrallelize this: 

#     for g in range(256):
#         for b in range(256):
#             rgb_np_pixel = np.array([r, g, b])
#             yuv_np_pixel = convert_pixel_rgb_to_yuv444(rgb_np_pixel)
#             rgb_np_pixel2 = convert_pixel_yuv444_to_rgb(yuv_np_pixel)
#             if not np.array_equal(rgb_np_pixel, rgb_np_pixel2):
#                 # print("Error converting pixel: " + str(rgb_np_pixel))
#                 # print("Got: " + str(rgb_np_pixel2))
#                 # print("Expected: " + str(rgb_np_pixel))
#                 error_count += 1
#             count += 1
#             if count % 100000 == 0:
#                 print("Checked " + str(count) + " pixels")

#     if error_count == 0:
#         print("All conversions correct!")
#     else:
#         print("Found " + str(error_count) + " errors")

#     return error_count
# pool = mp.Pool(processes=4)
# results = pool.map(test_color_conversions_helper, range(256))



# test_color_conversions()
