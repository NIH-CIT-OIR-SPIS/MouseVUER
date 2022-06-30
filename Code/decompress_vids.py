import cv2
import numpy as np
#import glob
import av
import math
import argparse, os
#from scipy.misc import imread
from scipy.linalg import norm
from scipy import sum, average, ndimage
from skimage.metrics import structural_similarity
from sklearn.metrics import mean_squared_error
from scipy import signal
import stat
#import ffmpeg
import traceback
from statistics import median
import ffmpeg
import imageio
import glob
import re

RESOLUTION_WIDTH = 1280
RESOLUTION_HEIGHT = 720
FRAME_RATE = 30

def apply_custom_colormap(img):
    # img_old = img
    # b, g, r = cv2.split(img)
    
    # _, max_b, _, _ = cv2.minMaxLoc(b)
    # _, max_g, _, _ = cv2.minMaxLoc(g)
    # _, max_r, _, _ = cv2.minMaxLoc(r)
    #stretch =  (img / ratio).astype('uint8')
    #min_i, max_i, _, _ = cv2.minMaxLoc(img)
    #stretch = img*255.0/img.max()
    #stretch = np.uint8(stretch)
    #normal_array = img/np.linalg.norm(img)
    #stretch = cv2.normalize(img, stretch, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_16U) #skimage.exposure.rescale_intensity(img, in_range='image', out_range=(0,255)).astype(np.uint8)
    stretch = cv2.convertScaleAbs(img, alpha=0.55, beta=0.11)
    #stretch = cv2.merge([stretch,stretch,stretch])
    # color1 = (0, 0, 0)       #black
    # color2 = (0, 165, 255)   #orange
    # color3 = (0, 255, 255)   #yellow
    # color4 = (255, 255, 0)   #cyan
    # color5 = (255, 0, 0)     #blue
    # color6 = (128, 64, 64)   #violet    
    # colorArr = np.array([[color1, color2, color3, color4, color5, color6]], dtype=np.uint8)
    # lut = cv2.resize(colorArr, (256,1), interpolation = cv2.INTER_LINEAR)
    # lut = np.zeros((256, 1, 3), dtype=np.uint8)
    
    # #Red
    # lut[:, 0, 0] = [255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,253,251,249,247,245,242,241,238,237,235,233,231,229,227,225,223,221,219,217,215,213,211,209,207,205,203,201,199,197,195,193,191,189,187,185,183,181,179,177,175,173,171,169,167,165,163,161,159,157,155,153,151,149,147,145,143,141,138,136,134,132,131,129,126,125,122,121,118,116,115,113,111,109,107,105,102,100,98,97,94,93,91,89,87,84,83,81,79,77,75,73,70,68,66,64,63,61,59,57,54,52,51,49,47,44,42,40,39,37,34,33,31,29,27,25,22,20,18,17,14,13,11,9,6,4,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]

    # #Green
    # lut[:, 0, 1] = [ 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,252,250,248,246,244,242,240,238,236,234,232,230,228,226,224,222,220,218,216,214,212,210,208,206,204,202,200,198,196,194,192,190,188,186,184,182,180,178,176,174,171,169,167,165,163,161,159,157,155,153,151,149,147,145,143,141,139,137,135,133,131,129,127,125,123,121,119,117,115,113,111,109,107,105,103,101,99,97,95,93,91,89,87,85,83,82,80,78,76,74,72,70,68,66,64,62,60,58,56,54,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0 ]

    # #Blue
    # lut[:, 0, 2] = [195,194,193,191,190,189,188,187,186,185,184,183,182,181,179,178,177,176,175,174,173,172,171,170,169,167,166,165,164,163,162,161,160,159,158,157,155,154,153,152,151,150,149,148,147,146,145,143,142,141,140,139,138,137,136,135,134,133,131,130,129,128,127,126,125,125,125,125,125,125,125,125,125,125,125,125,125,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126]
    # #print(lut.size)
    return cv2.applyColorMap(stretch, cv2.COLORMAP_JET)

def check_imgs(dir):
    i = 0
    for image_path in glob.glob(dir + "*.png"):
        image = imageio.imread(image_path)
        if i > 46:
            break
        print(np.max(image))
        print(np.min(image))
        i += 1


def folder_maker(path: str) -> None:
    if not os.path.isdir(path):
        os.makedirs(path)
        os.chmod(path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)



def recombine_frames(img_msb: np.uint8, img_lsb: np.uint16) -> np.uint16:
    depth_right_shifted = img_msb[:,:,0]
    arr_left_shift = np.left_shift(np.uint16(depth_right_shifted), 10, dtype=np.uint16)
    return np.bitwise_or(arr_left_shift, img_lsb, dtype=np.uint16)

def decode_8bitrgb_to_16bit_vsplit2_arr(img_msb: np.uint8, img_lsb: np.uint8) -> np.uint16:
    #img_msb, img_lsb = np.vsplit(depth_frm_res, 2)
    depth_right_shifted = img_msb[:,:,0]
    depth_and = img_lsb[:,:,0]
    arr_left_shift = np.left_shift(np.uint16(depth_right_shifted), np.uint16(8), dtype=np.uint16)
    return np.bitwise_or(arr_left_shift, np.uint16(depth_and), dtype=np.uint16)


def find_correlation_coef(raw, compressed):
    prod = np.mean((raw - raw.mean()) * (compressed - compressed.mean()))
    std_mul = raw.std() * compressed.std()
    if std_mul == 0:
        return 0
    else:
        prod /= std_mul
        return prod

def normalize(arr):
    rng = arr.max()-arr.min()
    amin = arr.min()
    return (arr-amin)*((2**16)-1)/rng

def compare_images(img1, img2) -> tuple:
    # normalize to compensate for exposure difference, this may be unnecessary
    # consider disabling it
    
    img1 = normalize(img1)
    img2 = normalize(img2)
    # calculate the difference and its norms
    diff = img1 - img2  # elementwise for scipy arrays
    m_norm = np.sum(abs(diff))  # Manhattan norm
    z_norm = norm(diff.ravel(), 0)  # Zero norm
    return (m_norm, z_norm)

def find_PSNR_btwn_imgs(raw: np.uint16, compressed: np.uint16):
    """
    Returns Peak Signal-to-Noise Ratio (PSNR) in dB
    """
    
    MSE = np.mean((raw - compressed)**2)
    if MSE == 0:
        return 100.0
    max_pixel_value = ((2**16) - 1)**2
    rmse = math.sqrt(MSE)
    psnr = 10.0 * math.log10(max_pixel_value / rmse)
    return (psnr, rmse)


def find_correlation_coef(raw, compressed):
    prod = np.mean((raw - raw.mean()) * (compressed - compressed.mean()))
    std_mul = raw.std() * compressed.std()
    if std_mul == 0:
        return 0
    else:
        prod /= std_mul
        return prod


def tryint(s):
    try:
        return int(s)
    except:
        return s

def alphanum_key(s):
    """ Turn a string into a list of string and number chunks.
        "z23a" -> ["z", 23, "a"]
    """
    return [ tryint(c) for c in re.split('([0-9]+)', s) ]

def sort_nicely(l):
    """ Sort the given list in the way that humans expect.
    """
    l.sort(key=alphanum_key)

def read_video_in(vid):
    str_out = ""
    if  not os.path.exists(vid):
        print("Path: {} doesn't exist".format(vid))
        return np.empty((1, 1))
    probe = ffmpeg.probe(vid)
    video_info = next(x for x in probe['streams'] if x['codec_type'] == 'video')
    width = int(video_info['width'])
    height = int(video_info['height'])
    #num_frames = int(video_info['nb_frames'])
    pix_fmt_in = str(video_info['pix_fmt'])
    #pix_fmt_out = 'rgb24'
    if pix_fmt_in == "yuvj420p" or pix_fmt_in ==  "yuv420p" or pix_fmt_in == "rgb24" or pix_fmt_in == "gbrp":
        pix_fmt_in = "rgb24"
    elif pix_fmt_in == "yuv420p10le" or pix_fmt_in == "yuv444p10le" or "gray10le":
        pix_fmt_in = "gray10le"
    else:
        print("Error: pix_fmt: {}, is not a pixel format supported".format(pix_fmt_in))
        return np.empty((1, 1))
    out, err = (
        ffmpeg.input(vid)
        .output('pipe:', format='rawvideo', pix_fmt=pix_fmt_in)
        .run(capture_stdout=True)
    )
    video = np.empty((720, 1280))
    if pix_fmt_in == "rgb24":
        video = np.frombuffer(out, np.uint8).reshape([-1, height, width, 3])
        return video
    else:
        video = np.frombuffer(out, np.uint16).reshape([-1, height, width])
        return video

def read_raw_video(vid):
    if not os.path.exists(vid):
        print("Path: {} doesn't exist".format(vid))
        return np.empty((1, 1))
    
def decompress_imgs(vidmsb, vidlsb, path_img, vidraw):
    parent_dir = os.path.abspath(os.path.join(vidlsb, os.pardir))
    path_write = os.path.join(parent_dir, 'CompressedVideoColormap.mp4')
    path_write_raw = os.path.join(parent_dir, 'RawVideoColormap.mp4')
    if not os.path.exists(vidmsb) or not os.path.exists(vidlsb) or not os.path.isdir(path_img):
        print("Path: {} doesn't exist or Path: {} doesn't exist".format(vidmsb, vidlsb))
        return
    lsb_vid = read_video_in(vidlsb)
    # msb_vid = read_video_in(vidmsb)
    frm_nm = 0
    path_read = [pth for pth in glob.glob(vidraw + "*.bin")]
    sort_nicely(path_read)
    psnr_lst = []
    ssim_lst = []
    out = cv2.VideoWriter(path_write, cv2.VideoWriter_fourcc('m', 'p', '4', 'v'), 30, (RESOLUTION_WIDTH, RESOLUTION_HEIGHT))
    out_raw =  cv2.VideoWriter(path_write_raw, cv2.VideoWriter_fourcc('m', 'p', '4', 'v'), 30, (RESOLUTION_WIDTH, RESOLUTION_HEIGHT))
    with av.open(vidmsb) as container_msb:
        for cont_frame_msb, frame_lsb in zip(container_msb.decode(video=0), lsb_vid):
            frm_nm += 1
            frame_msb = cont_frame_msb.to_ndarray(format='rgb24')
            frame_lossy = recombine_frames(frame_msb, frame_lsb)
            with open(path_read.pop(0), mode='rb') as fp:
                bter = fp.read()
                raw_img = np.frombuffer(bter, np.uint16).reshape(720, 1280)
            psnr, _ = find_PSNR_btwn_imgs(raw_img, frame_lossy)
            psnr_lst.append(psnr)
            #print(psnr)
            dfeg = apply_custom_colormap(frame_lossy)
            dfeg_raw = apply_custom_colormap(raw_img)
            #cv2.putText(dfeg, "Compressed Video Colormap", (50,50), cv2.FONT_HERSHEY_PLAIN, 2, (0,0,255), 2 )
            out.write(dfeg)
            out_raw.write(dfeg_raw)
            #ssim, _  = structural_similarity(raw_img, frame_lossy, full=True)
            #ssim_lst.append(ssim)
            #print(ssim)
            #cv2.imwrite(os.path.join(path_img, "decomp" + str(frm_nm) + ".png"), frame_lossy)
            #cv2.imwrite(os.path.join(path_img, "decomp" + str(frm_nm) + "_raw.png"), raw_img)
            # if frm_nm > 10000:
            #     print("Too many frames more than 10000")
            #     break
    out.release()
    out_raw.release()
    print(np.average(psnr_lst))
    #print(np.average(ssim_lst))
    #check_imgs(path_img)

    print("Success...")

def get_info_vid(vid):
    probe = ffmpeg.probe(vid)
    video_info = next(x for x in probe['streams'] if x['codec_type'] == 'video')
    return video_info

def process_vid(vidmsb, vidlsb, path_img, vidraw):
    num_frames_read_at_once = 1
    if not os.path.exists(vidmsb) or not os.path.exists(vidlsb) or not os.path.isdir(path_img):
        print("Path: {} doesn't exist or Path: {} doesn't exist or Directory: {} doesn't exist".format(vidmsb, vidlsb, path_img))
        return
    msb_info = get_info_vid(vidmsb)
    lsb_info = get_info_vid(vidlsb)

    if msb_info['height'] != lsb_info['height'] or msb_info['width'] != lsb_info['width']:
        print("Wrong shapes of videos")
        return
    # if msb_info['pix_fmt'] != str('yuvj420p') or lsb_info['pix_fmt'] != str('yuv420p10le'):
    #     print("Wrong formats")
    #     return
    msb_nb_frms = int(msb_info['nb_frames'])
    lsb_nb_frms = int(lsb_info['nb_frames'])
    width = int(msb_info['width'])
    height = int(msb_info['height'])
    if msb_nb_frms != lsb_nb_frms:
        print("Careful, reading one at a time becuase num_frames lsb != num frames msb ie {} != {}".format(lsb_nb_frms, msb_nb_frms))
        num_frames_read_at_once = 1
    else:
        k = 1
        while k <= 50:
            if (width*height) % k == 0:
                num_frames_read_at_once = k
            k += 1
        
    procmsb = (
        ffmpeg
        .input(vidmsb)
        .output('pipe:', format='rawvideo', pix_fmt='rgb24')
        .run_async(pipe_stdout=True)
    )
    


    proclsb = (
        ffmpeg
        .input(vidlsb)
        .output('pipe:', format='rawvideo', pix_fmt='gray10le')
        .run_async(pipe_stdout=True)
    )
    avg_psnr = 0
    frm_nm = 0
    num_frames_read_at_once = 1
    path_read = [pth for pth in glob.glob(vidraw + "*.bin")]
    sort_nicely(path_read)
    print(path_read)
    while True:
        frm_nm += 1
        msb_bytes = procmsb.stdout.read(width * height * 3 * num_frames_read_at_once)
        lsb_bytes = proclsb.stdout.read(width * height * 2 * num_frames_read_at_once)
        
        if not lsb_bytes or not msb_bytes:
            break
        msbvid = np.frombuffer(msb_bytes, np.uint8).reshape([-1, height, width, 3])
        lsbvid = np.frombuffer(lsb_bytes, np.uint16).reshape([-1, height, width])
        if path_read == []:
            break
        for frame_msb, frame_lsb in zip(msbvid, lsbvid): 
            with open(path_read.pop(0), mode='rb') as fp:
                bter = fp.read()
                raw_img = np.frombuffer(bter, np.uint16).reshape(height, width)
                
            frame_lossy = recombine_frames(frame_msb, frame_lsb)
            frame_lossy = (frame_lossy << 6) >> 6
            raw_img = (raw_img << 6) >> 6
            print("lossy: {0:b} raw: {0:b}".format(int(np.average(frame_lossy)), int(np.average(raw_img))))
            psnr, _ = find_PSNR_btwn_imgs(raw_img, frame_lossy)
            print(psnr)
            # holder_raw_lsb = (raw_img << 6) >> 6
            # holder_raw_msb = (raw_img >> 8)

            # #print(np.max(frame_lossy))
            # #print(np.min(frame_lossy))
            # print("raw max msb: {0:b}".format(np.max(holder_raw_msb)))
            # print("raw max lsb: {0:b}".format(np.max(holder_raw_lsb)))
            #print(np.min(raw_img))
            avg_psnr += psnr
            #cv2.imwrite(os.path.join(path_img, "decomp" + str(frm_nm) + ".png"), frame_lossy)
    procmsb.wait()
    proclsb.wait()
    proclsb.stdout.close()
    procmsb.stdout.close()
    print("Avg psnr: {}".format( (avg_psnr /frm_nm) ))
    print(num_frames_read_at_once)


def process_vid_other(vidmsb, vidlsb, path_img, vidraw):
    num_frames_read_at_once = 1
    if not os.path.exists(vidmsb) or not os.path.exists(vidlsb) or not os.path.isdir(path_img):
        print("Path: {} doesn't exist or Path: {} doesn't exist or Directory: {} doesn't exist".format(vidmsb, vidlsb, path_img))
        return
    msb_info = get_info_vid(vidmsb)
    lsb_info = get_info_vid(vidlsb)

    if msb_info['height'] != lsb_info['height'] or msb_info['width'] != lsb_info['width']:
        print("Wrong shapes of videos")
        return
    # if msb_info['pix_fmt'] != str('yuvj420p') or lsb_info['pix_fmt'] != str('yuv420p10le'):
    #     print("Wrong formats")
    #     return
    msb_nb_frms = int(msb_info['nb_frames'])
    lsb_nb_frms = int(lsb_info['nb_frames'])
    width = int(msb_info['width'])
    height = int(msb_info['height'])
    if msb_nb_frms != lsb_nb_frms:
        print("Careful, reading one at a time becuase num_frames lsb != num frames msb ie {} != {}".format(lsb_nb_frms, msb_nb_frms))
        num_frames_read_at_once = 1
    else:
        k = 1
        while k <= 50:
            if (width*height) % k == 0:
                num_frames_read_at_once = k
            k += 1
        
    procmsb = (
        ffmpeg
        .input(vidmsb)
        .output('pipe:', format='rawvideo', pix_fmt='rgb24')
        .run_async(pipe_stdout=True)
    )
    


    proclsb = (
        ffmpeg
        .input(vidlsb)
        .output('pipe:', format='rawvideo', pix_fmt='rgb24')
        .run_async(pipe_stdout=True)
    )
    avg_psnr = 0
    frm_nm = 0
    num_frames_read_at_once = 1
    path_read = [pth for pth in glob.glob(vidraw + "*.bin")]
    sort_nicely(path_read)
    print(path_read)
    while True:
        frm_nm += 1
        msb_bytes = procmsb.stdout.read(width * height * 3 * num_frames_read_at_once)
        lsb_bytes = proclsb.stdout.read(width * height * 3 * num_frames_read_at_once)
        
        if not lsb_bytes or not msb_bytes:
            break
        msbvid = np.frombuffer(msb_bytes, np.uint8).reshape([-1, height, width, 3])
        lsbvid = np.frombuffer(lsb_bytes, np.uint8).reshape([-1, height, width, 3])
        if path_read == []:
            break
        for frame_msb, frame_lsb in zip(msbvid, lsbvid): 
            with open(path_read.pop(0), mode='rb') as fp:
                bter = fp.read()
                raw_img = np.frombuffer(bter, np.uint16).reshape(height, width)
            
            frame_lossy = decode_8bitrgb_to_16bit_vsplit2_arr(frame_msb, frame_lsb)
            #frame_lossy = (frame_lossy << 6) >> 6
            #raw_img = (raw_img << 6) >> 6
            print("lossy: {0:b} raw: {0:b}".format(int(np.average(frame_lossy)), int(np.average(raw_img))))
            psnr, _ = find_PSNR_btwn_imgs(raw_img, frame_lossy)
            print(psnr)
            # holder_raw_lsb = (raw_img << 6) >> 6
            # holder_raw_msb = (raw_img >> 8)

            # #print(np.max(frame_lossy))
            # #print(np.min(frame_lossy))
            # print("raw max msb: {0:b}".format(np.max(holder_raw_msb)))
            # print("raw max lsb: {0:b}".format(np.max(holder_raw_lsb)))
            #print(np.min(raw_img))
            avg_psnr += psnr
            #cv2.imwrite(os.path.join(path_img, "decomp" + str(frm_nm) + ".png"), frame_lossy)
    procmsb.wait()
    proclsb.wait()
    proclsb.stdout.close()
    procmsb.stdout.close()
    print("Avg psnr: {}".format( (avg_psnr /frm_nm) ))
    print(num_frames_read_at_once)


def parse_options(dir=""):
    parser = argparse.ArgumentParser(description = 'Pass commands to read files from')
    parser.add_argument('--vidmsb', type=str, default='', help='MSB video')
    parser.add_argument('--vidlsb', type=str, default='', help='LSB video')  
    parser.add_argument('--path_img', type=str, default='', help='image write out directory')
    parser.add_argument('--vidraw', type=str, default='', help='RAW uncompressed video files path')
    return parser.parse_args()

if __name__ == "__main__":
    opt = parse_options()
    #process_vid_other(**vars(opt))
     #decompress_imgs
    #load_imgs(**vars(opt))
    decompress_imgs(**vars(opt))