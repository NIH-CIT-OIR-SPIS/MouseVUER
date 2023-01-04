#!/usr/bin/python3
#from _thread import start_new_thread
import os
import socket    
import multiprocessing
import subprocess as sp
import json
import time
import threading
import argparse
import signal
import ipaddress
import platform
import logging
import selectors
import types
import sys
import ssl
import shlex
import datetime
import traceback
from client_side import PORT_CLIENT_LISTEN, COMMON_NAME, ORGANIZATION, COUNTRY_ORGIN, BYTES_SIZE


# For GUI


from server_gui import ServerGUI

"""
This is the server side of the application.
GET LIST OF ALL IP ADDRESSES
1) First the server will get a list of all ip addresses on the network, using the pinger function and the map_network function.
2) Then the server will check if the ip address is in the list of valid ip addresses

HANDSHAKES 
3) Then a handshake will be sent to each of the clients with the ip address of the client.
4) Then the server will wait for the handshake from all of the clients

SENDING INITIAL JSON DATA
5) Then the server will send a json file to each of the clients
6) Then the server will wait for an acknowledgement from all of the clients that they recieved the json file or a json file was already present or the json file was not present.
7) If the json file isn't present the server will again attempt to send the json file to the client and repeat this up to 3 times unil the client recieves the json file. If fails the server will throw an error.

STARTING UP FFMPEG LISTENER WITH THREADED COMMAND LINE CALL
8) Then this server will then call a subprocess command in a parallel process. This command will contain ffmpeg rtsp listener and the ip address 


9) Then it will send a message with the servers ip address, and with a command string attached to all of the clients.


10) Then clients will run this command string in a subprocess on their own machine

11) The server will wait for a end signal from the subprocess command ffmpeg rtsp listener, once seen it will close the subprocess and send a message to all of the clients to end the session.

"""
  
NO_FORCE_EXIT = True
# PORT_CLIENT_LISTEN = 1026
# COMMON_NAME = "SCHORE_SYSTEM"
# ORGANIZATION = "NIH"
# COUNTRY_ORGIN = "US"
# BYTES_SIZE = 4096
def run_processes_parallel(cmd_lst):
    """
    Start up two server processes in parallel. Send them both signals to terminate.
    """
    
    try:
        p_list = []
        for cmd in cmd_lst:
            
            p = sp.Popen(shlex.split(cmd),  shell=False)
            p_list.append(p)
            #p_list.append(sp.Popen(cmd, shell=False))
        
        # Wait for the processes to terminate
        i = True
        while i:
            for p in p_list:
                if p.poll() is None:
                    time.sleep(0.1)
                else:
                    i = False
            #p.wait()
            #p.wait()
    
    # Send the processes a signal to terminate
    except KeyboardInterrupt:
        for p in p_list:
            p.send_signal(signal.SIGINT)
            p.wait()
        sys.exit(0)
    finally:
        if p_list is not []:
            for p in p_list:
                if p is not None:
                    p.send_signal(signal.SIGQUIT)

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

def build_ffmpeg_cmd_pair(server_addr: str, loglevel: str, port: int, dir: str, ip_str: str):
    """
    Build the ffmpeg command string
    :param server_addr:
    :param loglevel:
    :param port:
    :return:
    """
    addr = validate_ip(server_addr)
    port = port_type(port)
    loglevel = validate_loglevel(loglevel)
    str_time = str(time.strftime("%Y-%m-%d_%H-%M-%S", time.localtime()))
    cmd_lsb = "ffmpeg -hide_banner -listen 1 -timeout 10000 -f flv -loglevel {} -an -i rtmp://{}:{}/ -vcodec copy -pix_fmt yuv420p -y -movflags +faststart {}/vid_{}_lsb_port_{}_{}_out.mp4".format(loglevel, addr, port, dir, ip_str, str_time, port )
    cmd_msb = "ffmpeg -hide_banner -listen 1 -timeout 10000 -f flv -loglevel {} -an -i rtmp://{}:{}/ -vcodec copy -pix_fmt yuv420p -y -movflags +faststart {}/vid_{}_msb_port_{}_{}_out.mp4".format(loglevel, addr, port + 1, dir,  ip_str, str_time, port + 1)
    return cmd_lsb, cmd_msb

def build_ffmpeg_cmd_pair_list(map_dict: dict, loglevel: str, server_addr: str, dir: str):
    """
    Build the ffmpeg command string
    :param map_dict:
    :return:
    """
    cmd_list = []
    for ip, port in map_dict.items():
        ip_str = str(ip).replace('.','_')
        cmd_lsb, cmd_msb = build_ffmpeg_cmd_pair(server_addr, loglevel, port, dir, ip_str)
        cmd_list.append(cmd_lsb)
        cmd_list.append(cmd_msb)
    return cmd_list
    

def pinger(job_q, results_q):
    """
    Do Ping
    :param job_q:
    :param results_q:
    :return:
    """
    DEVNULL = open(os.devnull, 'w')
    while True:

        ip = job_q.get()

        if ip is None:
            break

        try:
            sp.check_call(['ping', '-c1', ip],
                                  stdout=DEVNULL)
            results_q.put(ip)
        except:
            pass



def get_my_ip():
    """
    Find my IP address
    :return:
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    ip = s.getsockname()[0]
    s.close()
    return ip


def map_network(pool_size=255):
    """
    Maps the network
    :param pool_size: amount of parallel ping processes
    :return: list of valid ip addresses
    """
    
    ip_list = list()
    
    # get my IP and compose a base like 192.168.1.xxx
    ip_parts = get_my_ip().split('.')
    base_ip = ip_parts[0] + '.' + ip_parts[1] + '.' + ip_parts[2] + '.'
    
    # prepare the jobs queue
    jobs = multiprocessing.Queue()
    results = multiprocessing.Queue()
    
    pool = [multiprocessing.Process(target=pinger, args=(jobs, results)) for i in range(pool_size)]
    
    for p in pool:
        p.start()
    
    for i in range(1, 255):
        place_in = base_ip + '{0}'.format(i)
        if place_in != get_my_ip():
            jobs.put(place_in)
        
    for p in pool:
        jobs.put(None)
    
    for p in pool:
        p.join()
    
    while not results.empty():
        ip = results.get()
        ip_list.append(ip)

    return ip_list

def port_type(port: int):
    port = int(port)
    if port <= 1027 or port > 65000:
        raise argparse.ArgumentTypeError("Error: Port number must in the ranges [1025, 65000]. Port number given: {}".format(port))
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

class Server:
    """
    Server class
    
    """
    def __init__(self, ip_lst_in, **kwargs):
        
        
        self.server_sni_hostname = COMMON_NAME
        if not (os.path.exists("keys/") and os.path.isfile("keys/server.key") and os.path.isfile("keys/client.crt") and os.path.isfile("keys/server.crt")):
            raise Exception("Server keys not found. Please run the make_ssl_keys_cert.py script, and copy files to each client and server.")
        self.server_cert = 'keys/server.crt'
        self.server_key = 'keys/server.key'
        self.client_crt = 'keys/client.crt'
        
        self.context  = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        self.context.verify_mode = ssl.CERT_REQUIRED
        self.context.load_cert_chain(certfile=self.server_cert, keyfile=self.server_key)
        self.context.load_verify_locations(cafile=self.client_crt)
        

        self.client_ip_lst = ip_lst_in
        self.argdict = kwargs
        self.__validate_args()
        self.port = self.argdict['port']
        self.ffmpeg_port_map = {} # maps ip address to two distinct ports
        self.host_port = {}
        self.server_ip = get_my_ip()
        print("Mapping to all ports {}".format(self.client_ip_lst))
        if not self.client_ip_lst:
            raise Exception("No clients seen please try again.")
        self.__initialize_maps()
        print("Mapping : {}".format(self.ffmpeg_port_map))
        self.thread_list = []
        #self.__start_ffmpeg(cmd_list)
        self.print_lock = threading.Lock()
        
        if os.path.isdir(self.argdict['dir']):
            with open(os.path.join(self.argdict['dir'], "video_head_file.txt"), "w") as f:
                f.write(str(self.argdict['max_depth']) + " " + str(self.argdict['min_depth']) + " " + str(self.argdict['depth_unit']))
        else:
            raise Exception("Directory {} does not exist".format(self.argdict['dir']))
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(('', PORT_CLIENT_LISTEN))



    def __validate_args(self):
        if self.argdict['port'] < 1025 or self.argdict['port'] > 65000:
            raise Exception("Error: Port number must in the ranges [1025, 65535]. Port number given: {}".format(self.argdict['port']))
        if self.argdict['time_run'] < 1:
            raise Exception("Error: Time to run must be greater than 0. Time to run given: {}".format(self.argdict['time_run']))
        if self.argdict['max_depth'] < 1:
            raise Exception("Error: Max depth must be greater than 0. Max depth given: {}".format(self.argdict['max_depth']))
        if self.argdict['max_depth'] > 65535:
            raise Exception("Error: Max depth must be less than 65535. Max depth given: {}".format(self.argdict['max_depth']))
        if self.argdict['max_depth'] <= self.argdict['min_depth']:
            raise Exception("Error: Max depth must be greater than min depth. Max depth given: {}, Min depth given: {}".format(self.argdict['max_depth'], self.argdict['min_depth']))
        if self.argdict['min_depth'] < 0:
            raise Exception("Error: Min depth must be greater than 0. Min depth given: {}".format(self.argdict['min_depth']))
        if self.argdict['min_depth'] >= 65535:
            raise Exception("Error: Min depth must be less than 65535. Min depth given: {}".format(self.argdict['min_depth']))
        if self.argdict['crf'] < -1:
            raise Exception("Error: CRF must be greater than -1. CRF given: {}".format(self.argdict['crf']))
        if self.argdict['crf'] > 51:
            raise Exception("Error: CRF must be less than 51. CRF given: {}".format(self.argdict['crf']))
        if not self.argdict['fps'] in [30, 60, 120]:
            raise Exception("Error: FPS must be 30, 60, or 120. FPS given: {}".format(self.argdict['fps']))
        if self.argdict['dimensions'] not in ['640x480', '1280x720', '1920x1080']:
            raise Exception("Error: Dimensions must be 640x480, 1280x720, or 1920x1080. Dimensions given: {}".format(self.argdict['dimensions']))
        if self.argdict['fps'] > 30 and self.argdict['dimensions'] in ['1280x720', '1920x1080']:
            raise Exception("Error: FPS must be less than 30 if dimensions are 1280x720 or 1920x1080. FPS given: {}, Dimensions given: {}".format(self.argdict['fps'], self.argdict['dimensions']))
        if self.argdict['depth_unit'] < 40 or self.argdict['depth_unit'] >= 65535:
            raise Exception("Error: Depth unit must be greater than 40 and less than 65535. Depth unit given: {}".format(self.argdict['depth_unit']))
        
    def __initialize_maps(self):
        i = 0
        port_store = self.port
        for ip_addr in self.client_ip_lst:
            self.ffmpeg_port_map.update({ip_addr: port_store + i})
            i += 2

        for ip_addr in self.client_ip_lst:
            self.host_port.update({ip_addr: PORT_CLIENT_LISTEN})
            
    def listen(self):
        print("Listen")
        try:
            self.sock.listen(6)
            count = 0
            while count < self.argdict['num_clients']:
                clientsocket, addr = self.sock.accept()
                #self.print_lock.acquire()
                with self.print_lock:
                    print("Connected to {}".format(addr))
                #addr = (self.ffmpeg_port_map[str(addr[0])], addr)
                if addr[0] in self.ffmpeg_port_map:
                    ffmpeg_port = self.ffmpeg_port_map[str(addr[0])]
                else:
                    ffmpeg_port = addr[1]
                #ffmpeg_port = 
                # ## START UP FFMPEG SERVER HERE
                # make_commands(addr, "debug", addr[1])
                clientsocket.settimeout(60)
                th = threading.Thread(target=self.threaded_get_info, args=(clientsocket, addr, ffmpeg_port,))
                self.thread_list.append(th)
                th.start()
                count += 1


            for ty in self.thread_list:
                ty.join()
            self.sock.close()
                
        except KeyboardInterrupt:
            print("Keyboard Interrupt, here")
            
            self.sock.close()

    def threaded_get_info(self, clientsocket, addr, ffmpeg_port):
        conn = self.context.wrap_socket(clientsocket, server_side=True)
        try:
            while True:
                data = conn.recv(BYTES_SIZE)
                if data:
                    #buf += data
                    # with self.print_lock:
                    #     print("Received: {}".format(data.decode('ascii')))
                    #     print("Address: {}".format(addr))
                    #conn.send("ADDRESS: {}".format(self.server_ip))
                    message = "time_run:{:d},crf:{:d},server_ip:{},ffmpeg_port:{:d},max_depth:{:d},min_depth:{:d},depth_unit:{:d},to_run:{:d}".format(self.argdict['time_run'], self.argdict['crf'], self.server_ip, ffmpeg_port, self.argdict['max_depth'], self.argdict['min_depth'], self.argdict['depth_unit'], 1)#f'{make_commands(self.server_ip, "debug", addr[1])[0]} :: {make_commands(self.server_ip, "debug", addr[1])[1]}'
                    
                    #message = "FROM SERVER: ADDRESS: {}, SERVER IP: {}".format(addr, self.server_ip)
                    conn.send(message.encode('ascii'))
                    
                else:
                    #print("Recieved: {}".format(data))
                    
                    #self.print_lock.release()
                    break

        finally:
            # if self.print_lock.
            # self.print_lock.release()
            conn.shutdown(socket.SHUT_RDWR)
            conn.close()
            with self.print_lock:
                print("Connection closed\n")
                
            #print("Connection closed")


def server_side_command_line_parser():
    """
    Takes in the options of what ports will be used, what level of verbosity for the logging, 
    Options:
    -p, --port <int> must be between values [1025, 65535]
    -v, --verbose <string> (debug, info, error, warning, quiet)
    -h, --help
    -dim, --dimensions <string> (widthxheight) valid dimensions are: (640x480, 1280x720, 1920x1080). Default is 1280x720
    -f, --framerate <int> valid framerates are: (15, 30, 60, 90). Default is 30. 60 fps and 90 fps are only available for 640x480. 
                            30 fps and 15 fps are available for all dimensions.
    -time <time> amount of time to record in seconds
    -json <json file> json file to be used for the video recordings.
    -split_time <time> length in time of each video recording in seconds. Default is 120 seconds.
    Optional:
    -basename <string> the base name of the video files
    -max_depth <int> the max depth of for the depth camera to use. Must be greater than min_depth. Valid range is [1, 65535]. Default is 65535
    -min_depth <int> the min depth for the depth camera to use. Must be less than max_depth. Valid range is [0, 65534]. Default is 0
    -depth_unit <int> The units of the depth camera to use valid range, [40, 25000]. Default value is 1000
    """
    parser = argparse.ArgumentParser(description='Server for the video streaming application')
    parser.add_argument('--port', type=int, default=5000, help='Port to listen on.  Must be between values [1027, 65000]')
    parser.add_argument('--loglevel', type=str, default='info', help='Verbosity level of the logging')
    parser.add_argument('--dimensions', type=str, default='1280x720', help='Dimensions of the video stream, (widthxheight) valid dimensions are: (640x480, 1280x720, 1920x1080). Default is 1280x720')
    parser.add_argument('--fps', type=int, default=30, help='Framerate of recordings.Valid framerates are: (15, 30, 60, 90). Default is 30. 60 fps and 90 fps are only available for 640x480. 30 fps and 15 fps are available for all dimensions.')
    parser.add_argument('--time_run', type=int, default=30, help='Amount of time to record in seconds. Default 30 seconds')
    parser.add_argument('--json', type=str, default='', help='Json file to be sent to the client and used for the video recordings')
    parser.add_argument('--split_time', type=int, default=120, help='Length in to split each recording into. Default is 120 seconds')
    parser.add_argument('--crf', type=int, default=22, help='The quality of the video. Valid range is [0, 51]. Default is 22')
    parser.add_argument('--basename', '--basename', type=str, default='test_', help='The base name of the video files')
    parser.add_argument('--max_depth', '--max_depth', type=int, default=65535, help='The max depth of for the depth camera to use. Must be greater than min_depth. Valid range is [1, 65535]. Default is 65535')
    parser.add_argument('--min_depth', '--min_depth', type=int, default=0, help='The min depth for the depth camera to use. Must be less than max_depth. Valid range is [0, 65534]. Default is 0')
    parser.add_argument('--depth_unit', '--depth_unit', type=int, default=1000, help='The units of the depth camera to use valid range, [40, 25000]. Default value is 1000')
    parser.add_argument('--num_clients', '--num_clients', type=int, default=1, help='The number of clients to connect to the server. Default is 1')
    parser.add_argument('--dir', '--dir', type=str, default='Testing_DIR', help='The directory to save the video files to. Default is the current directory')
    return parser.parse_args()



def main():
    print("Getting IP addresses...")
    ip_lst = map_network()
    print("Done getting IP addresses")
    print("Starting Graphical User Interface...")
    # TODO: Start GUI
    
    args = server_side_command_line_parser()
    server = Server(ip_lst, **vars(args))
    cmd_list = build_ffmpeg_cmd_pair_list(server.ffmpeg_port_map, server.argdict['loglevel'], server.server_ip, server.argdict['dir'])
    p1 = multiprocessing.Process(target=run_processes_parallel, args=(cmd_list,))
    p1.start()
    server.listen()
    # if p1.is_alive():
    #     p1.terminate()
    #     p1.join()
    try:
        p1.join()
    except Exception as e:
        p1.terminate()
        p1.join()
        print("Error here: {}".format(e))
        #pass
    finally:
        os.system("stty echo")


if __name__ == '__main__':
    main()
    #server.start_conn_send_data(host="127.0.0.1", port=12345)

    #lst.remove()
    #print(lst)

