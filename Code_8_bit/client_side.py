#!/usr/bin/python3
# from _thread import start_new_thread

import os
import socket    
import multiprocessing
import subprocess as sp
import json
import time
import threading
import argparse
import ipaddress
import platform
import logging
import selectors
import types
import sys
# Secure Sockets Layer (SSL)
import hashlib
# from Crypto import Random
# import Crypto.Cipher.AES as AES
# from Crypto.PublicKey import RSA
import signal
import ssl

COMMON_NAME = "."
ORGANIZATION = "NIH"
COUNTRY_ORGIN = "US"
PORT_CLIENT_LISTEN = 1026
HOST_ADDR = "192.168.1.234"
BYTES_SIZE = 4096
WAIT_SEC = 0.

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

def run_rtmp_command(recieved: str):
    arr = recieved.split(",")
    for i in range(len(arr)):
        arr[i] = arr[i].strip()
        arr[i] = arr[i].split(":")[1]
    
    time_run = int(arr[0])
    crf = int(arr[1])
    server_ip = str(arr[2])
    port = int(arr[3])
    max_depth = int(arr[4])
    min_depth = int(arr[5])
    depth_unit = int(arr[6])
    run_it = int(arr[7])
    cmd = "./bin/multicam -dir Testing_DIR/ -sec {:d} -crf {:d} -sv_addr {} -port {:d} -max_depth {:d} -min_depth {:d} -depth_unit {:d}".format(time_run, crf, server_ip, port, max_depth, min_depth, depth_unit)
    #print(cmd)
    os.system(cmd)

def trying_connect(s: socket, host, port):
    print("Trying to connect to {}:{}".format(host, port))
    while s.connect_ex((host, port)) != 0:
        continue
        # if count > 300000:
        #     print("Connection failed. Script Waiting {} seconds...".format(WAIT_SEC))
        #     count = 0
        # count += 1
        #time.sleep(WAIT_SEC)

class Client:
    def __init__(self, server_sni_hostname: str, host: str, port: int, server_cert_file: str, client_cert_file: str, client_key_file: str):
        self.server_addr = host
        self.server_port = port
        self.server_sni_hostname = server_sni_hostname
        self.context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=server_cert_file)
        self.context.load_cert_chain(certfile=client_cert_file, keyfile=client_key_file)
        #self.connect_to_server('127.0.0.1', 12345)


        
    def connect_to_server(self, host, port):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #s.settimeout(WAIT_SEC)
        frt = os.path.basename(__file__)
        print("strftime: {}".format(time.strftime("%Y-%m-%d_%H-%M-%S", time.localtime())))
        # Keep polling
        # while True:
        #     try:
        #         if not trying_connect(s, host, port):
        #             continue
        #         break
        #     except socket.timeout as e:
        #         #print("Error: timeout {}".format(e))
        #         continue
        #     except socket.error as e:
        #         print("Fatal Error: {}".format(e))
        #         sys.exit(1)
        #     # except Exception as e:
        #     #     print("Fatal Error: {}".format(e))
        #     #     sys.exit(1)
        while s.connect_ex((host, port)) != 0:
            print("Waiting for connection to host: {}, port {}...".format(host, port))
            continue
        conn = self.context.wrap_socket(s, server_side=False, server_hostname=self.server_sni_hostname)
        message = "Host: {}, Port: {}, My_ip: {}, ".format(host, port, get_my_ip())
        conn.send(message.encode('ascii'))
        data = conn.recv(BYTES_SIZE)
        recieved = "{}".format(data.decode("ascii")) # decode the data
        #print("{},{}".format(os.path.basename(__file__),recieved))
        #print("Closing connection")
        #print("Recieved data: {}".format(data))
        conn.close()
        s.close()
        run_rtmp_command(recieved)
        





def main():

    
    server_sni_hostname = "SCHORE_SERVER"
    if not (os.path.exists("keys/") and os.path.isfile("keys/client.key") and os.path.isfile("keys/client.crt") and os.path.isfile("keys/server.crt")):
        raise Exception("Error finding ssl files")
    server_cert = "keys/server.crt"
    client_cert = "keys/client.crt"
    client_key = "keys/client.key"
    client = Client(server_sni_hostname, HOST_ADDR, PORT_CLIENT_LISTEN, server_cert, client_cert, client_key)
    # Keep connecting until keyboard interrupt or shutdown signal
    while True:
        try:
            client.connect_to_server(HOST_ADDR, PORT_CLIENT_LISTEN)
        except KeyboardInterrupt:
            print("Keyboard interrupt")
            break
        except Exception as e:
            print("Error: {}".format(e))
            break
    # while True:
    #     client.connect_to_server(HOST_ADDR, PORT_CLIENT_LISTEN)
    #return 0
    # context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=server_cert)
    # context.load_cert_chain(certfile=client_cert, keyfile=client_key)
    # s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # conn = context.wrap_socket(s, server_side=False, server_hostname=server_sni_hostname)
    # c
    
if __name__ == "__main__":
    main()
    