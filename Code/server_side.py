#!/usr/bin/python3
from _thread import start_new_thread
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
PORT_CLIENT_LISTEN = 1026
COMMON_NAME = "SCHORE_SYSTEM"
ORGANIZATION = "NIH"
COUNTRY_ORGIN = "US"



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
    
    # cue hte ping processes
    for i in range(1, 255):
        place_in = base_ip + '{0}'.format(i)
        if place_in != get_my_ip():
            jobs.put(place_in)
        
    for p in pool:
        jobs.put(None)
    
    for p in pool:
        p.join()
    
    # collect he results
    while not results.empty():
        ip = results.get()
        ip_list.append(ip)

    return ip_list

def port_type(port: int):
    port = int(port)
    if port <= 1027 or port > 65535:
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

# class ServerWorker:
#     def __init__(self, clientinfo):
#         self.clientinfo = clientinfo

#     def run(self):
#         threading.Thread(target=self.requesting).start()


#     def requesting(self):
#         """ Recive Request from client """
#         connsocket = self.clientinfo['connsocket'][0]
#         while True:
#             data = connsocket.recv(1024)
#             if data:
#                 print("Received: {}".format(data))
#                 self.clientinfo['request'] = data.decode('utf-8')
#                 break


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
        self.port = self.argdict['port']
        self.ffmpeg_port_map = {} # maps ip address to two distinct ports
        self.host_port = {}
        self.server_ip = get_my_ip()

        self.__initialize_maps()

        self.print_lock = threading.Lock()

        
        
        # for key, value in kwargs.items():
        #     self.argdict[key] = value
        # self.logger = logging.getLogger(__name__)
        # self.logger.setLevel(logging.DEBUG)
        # self.logger.addHandler(logging.StreamHandler())
        # self.logger.info("Server started with ip address {} ".format(self.server_ip))
        # self.logger.info("Client IP List: {}".format(self.client_ip_lst))
        # self.logger.info("Arguments: {}".format(self.argdict))
        # self.logger.info("Platform: {}".format(platform.platform()))
        # self.logger.info("OS: {}".format(platform.system()))
        # self.logger.info("OS Version: {}".format(platform.version()))




    def __initialize_maps(self):
        i = 0
        port_store = self.port
        for ip_addr in self.client_ip_lst:
            self.ffmpeg_port_map.update({ip_addr: (port_store + i, port_store + i + 1)})
            i += 2

        for ip_addr in self.client_ip_lst:
            self.host_port.update({ip_addr: PORT_CLIENT_LISTEN})


    def start_conn_send_data(self, host: str, port: int):
        """
        Start a connection to a host and send data
        :param host:
        :param port:
        :return:
        """
        ssocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        ssocket.bind((host, port))
        ssocket.listen(5)
        while NO_FORCE_EXIT:
            c, addr = ssocket.accept()
            self.print_lock.acquire()
            print("Connected to {}:{}".format(addr[0], addr[1]))
            start_new_thread(self.threaded_get_info, (c, addr,))
        ssocket.close()


    def threaded_get_info(self, connsocket, addr):
        while True:
            data = connsocket.recv(1024)
            if not data:
                print("Bye")
                self.print_lock.release()
                break
            boh = data.decode('utf-8')
            print("Received: {}".format(boh))
            connsocket.send(b'Address: {}'.format(addr[0]).encode('utf-8'))
        connsocket.close()

            









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
    parser.add_argument('--port', type=port_type, default=port_type(5000), help='Port to listen on.  Must be between values [1027, 65000]')
    parser.add_argument('--verbose', type=str, default='info', help='Verbosity level of the logging')
    parser.add_argument('--dimensions', type=str, default='1280x720', help='Dimensions of the video stream, (widthxheight) valid dimensions are: (640x480, 1280x720, 1920x1080). Default is 1280x720')
    parser.add_argument('--fps', type=int, default=30, help='Framerate of recordings.Valid framerates are: (15, 30, 60, 90). Default is 30. 60 fps and 90 fps are only available for 640x480. 30 fps and 15 fps are available for all dimensions.')
    parser.add_argument('-time', type=int, default=30, help='Amount of time to record in seconds. Default 30 seconds')
    parser.add_argument('--json', type=str, default='', help='Json file to be sent to the client and used for the video recordings')
    parser.add_argument('--split_time', type=int, default=120, help='Length in to split each recording into. Default is 120 seconds')

    parser.add_argument('-basename', '--basename', type=str, default='test_', help='The base name of the video files')
    parser.add_argument('-max_depth', '--max_depth', type=int, default=65535, help='The max depth of for the depth camera to use. Must be greater than min_depth. Valid range is [1, 65535]. Default is 65535')
    parser.add_argument('-min_depth', '--min_depth', type=int, default=0, help='The min depth for the depth camera to use. Must be less than max_depth. Valid range is [0, 65534]. Default is 0')
    parser.add_argument('-depth_unit', '--depth_unit', type=int, default=1000, help='The units of the depth camera to use valid range, [40, 25000]. Default value is 1000')

    return parser.parse_args()

if __name__ == '__main__':
    print("Getting IP addresses...")
    ip_lst = map_network()
    print("Done getting IP addresses")
    args = server_side_command_line_parser()
    server = Server(ip_lst, **vars(args))
    server.start_conn_send_data(host="", port=12345)
    
    #lst.remove()
    #print(lst)

