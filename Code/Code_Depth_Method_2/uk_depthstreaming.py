import numpy
from math import floor
import random
import time
import json
w = 2**16
n_p = 512
p = n_p / w
print("p: ", 512 / 65336)


def equat_L(dist):
    return (dist + 0.5) / w

def equat_H_a(d):
    L_d = equat_L(d)
    ph = p / 2

    #rint() 
    # print("L_d: ", L_d)
    # print("L_d / ph: ", L_d / ph)
    # print("L_d / ph % 2: ", (L_d / ph) % 2)
    if (L_d / ph) % 2 <= 1:

        return (L_d / ph) % 2
    else:
        return 2 - ((L_d / ph) % 2)

def equat_H_b(d):
    p4 = p / 4
    L_d = equat_L(d)
    p2 = p / 2
    if ((L_d - p4) / p2) % 2 <= 1:
        return ((L_d - p4) / p2) % 2
    else:
        return 2 - ( ((L_d - p4) / p2) % 2)


def equat_mL(L):
    return floor(4*L/p - .5) % 4


def equat_L0(L_):
    # print("L_: ", L_)
    # a = (L_ - (p/8)) % p
    # print("L_ - (p/8): ", (L_ - (p/8)))
    # print("a: ", a)
    # print("(p/4)*equat_mL(L_) ", (p/4)*equat_mL(L_))
    # ret_val = L_ - ((L_ - (p/8)) % p ) + (p/4)*equat_mL(L_) - (p/8)
    # print("ret_val: ", ret_val)
    return L_ - ((L_ - (p/8)) % p ) + (p/4)*equat_mL(L_) - (p/8)

def delta_f(L_, Ha_, Hb_):
    ag = equat_mL(L_)
    if ag == 0:
        return Ha_*(p/2)
    elif ag == 1:
        return Hb_*(p/2)
    elif ag == 2:
        return (1-Ha_)*(p/2)
    elif ag == 3:
        return (1-Hb_)*(p/2)
    else:
        print("ahwwww")
        return -5
    
def back_orig(L_, Ha_, Hb_):
    return int(w*(equat_L0(L_) + delta_f(L_, Ha_, Hb_)))

def tuple_to_string(tup):
    return str(tup[0]) + "," + str(tup[1]) + "," + str(tup[2])
st_time = time.time()
depth_val = random.randint(0, 2**16-1)
print("depth_val: ", depth_val)
count = 0
build_map = {}
map_depth_to_tuple = {}
map_sum = {}
list_all_vals = []
for depth_val in range(0, 2**16):
    Le = equat_L(depth_val)
    Hea = equat_H_a(depth_val)
    Heb = equat_H_b(depth_val)
    # print("depth_val: ", depth_val)
    # print("equat_H_a: ", equat_H_a(depth_val))
    # print("equat_H_b: ", equat_H_b(depth_val))
    # print("equat_L: ", equat_L(depth_val))

    Le1 = 255 * Le
    Hea1 = 255 * Hea
    Heb1 = 255 * Heb

    # print("Le1: ", Le1)
    # print("Hea1:", Hea1)
    # print("Heb1:", Heb1)

    # print("rounded: ")
    # print(" Le1 rounded ", int(Le1))
    # print(" Hea1 rounded ", int(Hea1))
    # print(" Heb1 rounded ", int(Heb1))

    # map a unique tuple to each depth value
    # check if collision occurs
    if int(round(Le1, 0)) not in list_all_vals:
        list_all_vals.append(int(round(Le1, 0)))
    if (int(round(Le1, 0)), int(round(Hea1, 0)), int(round(Heb1, 0))) in build_map.keys():
        print("collision")
        print("depth_val: ", depth_val)
        print((int(round(Le1, 0)), int(round(Hea1, 0)), int(round(Heb1, 0))))
    else:
        tup = (int(round(Le1, 0)), int(round(Hea1, 0)), int(round(Heb1, 0)))
        build_map[tuple_to_string(tup)] = depth_val
    map_sum[int(round(Le1, 0)) + int(round(Hea1, 0)) + int(round(Heb1, 0))] = depth_val
    map_depth_to_tuple[depth_val] = (int(round(Le1, 0)), int(round(Hea1, 0)), int(round(Heb1, 0)))
    Le = int(round(Le1, 0)) / 255

    Hea = int(round(Hea1, 0)) / 255

    Heb = int(round(Heb1, 0)) / 255

    # print("Le: ", Le)
    # print("Hea: ", Hea)
    # print("Heb: ", Heb)

    orig = int(back_orig(Le, Hea, Heb))
    # print("orig: ", orig)

    # length of build_map should be 2**16
    
    
    if abs(orig - depth_val) > 120:
        print("orig: ", orig)
        print("depth_val: ", depth_val)
        print("diff: ", abs(orig - depth_val))
        count += 1
    #break
# for tup in list_all_vals:
#     if list_all_vals.count(tup) > 1:
#         print("tup: ", tup)
# look for duplicates in 
# Check if there are any duplicates


# print("back_to original: ", back_orig(Le, Hea, Heb))with open('map_depth_to_tuple.json', 'w') as fp:

print(list_all_vals)
print("time elapsed: ", (time.time() - st_time))
print("len of build_map: ", len(build_map))
print("len of map_depth_to_tuple: ", len(map_depth_to_tuple))
print("len of map_sum: ", len(map_sum))

# Save build map to file json 
with open('map_tuple_to_depth.json', 'w') as fp:
    json.dump(build_map, fp)

with open('map_depth_to_tuple.json', 'w') as fep:
    json.dump(map_depth_to_tuple, fep)

print("count: ", count)