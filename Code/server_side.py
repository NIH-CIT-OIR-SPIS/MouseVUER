import os
import socket    
import multiprocessing
import subprocess as sp
import json
import time


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


9) Then it will send a message with a command string attached to all of the clients.


10) Then clients will run this command string in a subprocess on their own machine

11) The server will wait for a end signal from the subprocess command ffmpeg rtsp listener, once seen it will close the subprocess and send a message to all of the clients to end the session.


"""




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


if __name__ == '__main__':
    lst = map_network()
    #lst.remove()
    print(lst)

