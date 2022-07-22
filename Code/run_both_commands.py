import platform
import subprocess as sp
import os
import signal
import argparse
import ipaddress
import time
import datetime
def run_processes_parallel(cmd_lst):
    """
    Start up two server processes in parallel. Send them both signals to terminate.
    """
    try:
        p_list = []
        for cmd in cmd_lst:
            p_list.append(sp.Popen(cmd, shell=True))
        
        # Wait for the processes to terminate
        for p in p_list:
            p.wait()
    
    # Send the processes a signal to terminate
    finally:
        if p_list is not []:
            for p in p_list:
                p.send_signal(signal.SIGQUIT)

def port_type(port: int):
    port = int(port)
    if port <= 1024 or port > 65535:
        raise argparse.ArgumentTypeError("Error: Port number must in the ranges [1025, 65535]. Port number given: {}".format(port))
    return port

def validate_ip(addr):
    try:
        ip = ipaddress.ip_address(addr)
        print("Valid IP: {}".format(ip))
        return addr
    except ValueError:
        #print("IP address {} is not valid".format(addr))
        raise argparse.ArgumentTypeError("IP address {} is not valid".format(addr))
        return None

def validate_loglevel(loglevel):
    if loglevel not in ['error', 'verbose', 'debug', 'quiet', 'info', 'warning']:
        raise argparse.ArgumentTypeError("Error: Log level must be one of the following: error, verbose, debug")
    return loglevel

def make_commands(addr: str, loglevel: str, port: int, split_size: int):
    addr1 = validate_ip(addr)
    port = port_type(port)
    loglevel = validate_loglevel(loglevel)
    split_size = int(split_size)
    if split_size < 1:
        raise argparse.ArgumentTypeError("Error: Split size must be greater than 0")
    #print("Address: {}, loglevel {}, port {}".format(addr, loglevel, port))
    #loglevel = "error"
    #addr1 = "169.254.255.255"
    port_num_lsb = port
    cmd_lst = []
    cmd1 = "ffmpeg -listen 1 -timeout 10000 -f flv -loglevel {} -an -i rtmp://{}:{}/ -c:v copy -pix_fmt yuv420p10le -map 0 -segment_time {} -y Testing_DIR/test_lsb_{}.mp4".format(loglevel, addr1, port_num_lsb, str(datetime.timedelta(seconds=split_size)), port)
    cmd2 = "ffmpeg -listen 1 -timeout 10000 -f flv -loglevel {} -an -i rtmp://{}:{}/ -c:v copy -pix_fmt rgb24 -map 0 -segment_time {} -y Testing_DIR/test_msb_{}.mp4".format(loglevel, addr1, port_num_lsb, str(datetime.timedelta(seconds=split_size)), port)
    # shlex.quote(cmd1)
    # shlex.quote(cmd2)
    cmd_lst.append(cmd1)
    cmd_lst.append(cmd2)
    run_processes_parallel(cmd_lst)
    if platform.system() == "Linux":
        os.system("stty echo")


def parse_options():
    parser = argparse.ArgumentParser(description = 'Pass commands to read files from')
    parser.add_argument('--addr', type=str, help='IP address to use')
    parser.add_argument('--loglevel', type=str, default='error', help='logging level: ie error, verbose, debug, [error default]')  
    parser.add_argument('--port', type=int, default=5000, help="Port number must in the ranges [1025, 65535], default is 5000")
    return parser.parse_args()

if __name__ == "__main__":
    make_commands(**vars(parse_options()))