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
import ipaddress
import platform
import logging
import selectors
import types
import sys
# Secure Sockets Layer (SSL)
import hashlib
from Crypto import Random
import Crypto.Cipher.AES as AES
from Crypto.PublicKey import RSA
import signal
import ssl
COMMON_NAME = "."
ORGANIZATION = "NIH"
COUNTRY_ORGIN = "US"

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

class Client:
    def __init__(self, server_sni_hostname: str, host: str, port: int, server_cert_file: str, client_cert_file: str, client_key_file: str):
        self.server_addr = host
        self.server_port = port
        self.server_sni_hostname = server_sni_hostname
        self.context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=server_cert_file)
        self.context.load_cert_chain(certfile=client_cert_file, keyfile=client_key_file)
        self.connect_to_server('127.0.0.1', 12345)

    def connect_to_server(self, host, port):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        conn = self.context.wrap_socket(s, server_side=False, server_hostname=self.server_sni_hostname)
        conn.connect((host, port))
        conn.send(b"Hello from the client")
        data = conn.recv(1024)
        print(f"Received: {data.decode('utf-8')}")
        conn.close()



def main():

    
    server_sni_hostname = "SCHORE_SERVER"
    if not (os.path.exists("keys/") and os.path.isfile("keys/client.key") and os.path.isfile("keys/client.crt") and os.path.isfile("keys/server.crt")):
        raise Exception("Error finding ssl files")
    server_cert = "keys/server.crt"
    client_cert = "keys/client.crt"
    client_key = "keys/client.key"
    client = Client(server_sni_hostname, '127.0.0.1', 12345, server_cert, client_cert, client_key)
    client.connect_to_server('127.0.0.1', 12345)
    return 0
    # context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=server_cert)
    # context.load_cert_chain(certfile=client_cert, keyfile=client_key)
    # s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # conn = context.wrap_socket(s, server_side=False, server_hostname=server_sni_hostname)
    # c
    
if __name__ == "__main__":
    main()
    
    sys.exit(0)
    