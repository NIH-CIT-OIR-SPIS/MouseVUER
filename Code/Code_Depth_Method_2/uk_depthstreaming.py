import numpy
from math import floor
import random
import time
w = 2**16
n_p = 512
p = n_p / w



def equat_L(dist):
    return (dist + 0.5) / w

def equat_H_a(d):
    L_d = equat_L(d)
    ph = p / 2

    #rint() 
    print("L_d: ", L_d)
    print("L_d / ph: ", L_d / ph)
    print("L_d / ph % 2: ", (L_d / ph) % 2)
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
    a = (L_ - (p/8)) % p
    print("a: ", a)
    print((L_ - (p/8)) )
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

st_time = time.time()
depth_val = random.randint(0, 2**16-1)
print("depth_val: ", depth_val)
count = 0
for depth_val in range(0, 2**16-1):
    Le = equat_L(depth_val)
    Hea = equat_H_a(depth_val)
    Heb = equat_H_b(depth_val)
    # print("equat_H_a: ", equat_H_a(depth_val))
    # print("equat_H_b: ", equat_H_b(depth_val))
    #print("equat_L: ", equat_L(depth_val))

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

    Le = int(round(Le1, 0)) / 255

    Hea = int(round(Hea1, 0)) / 255

    Heb = int(round(Heb1, 0)) / 255
    orig = int(back_orig(Le, Hea, Heb))
    if abs(orig - depth_val) > 120:
        print("orig: ", orig)
        print("depth_val: ", depth_val)
        print("diff: ", abs(orig - depth_val))
        count += 1


        

# print("back_to original: ", back_orig(Le, Hea, Heb))
print("time elapsed: ", (time.time() - st_time))
print("count: ", count)