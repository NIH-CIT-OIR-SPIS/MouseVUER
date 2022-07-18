import platform
import subprocess as sp
import os
import signal

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


if __name__ == "__main__":
    loglevel = " verbose "
    addr1 = "127.0.0.1"
    port_num_lsb = 5000
    cmd_lst = []
    cmd1 = "ffmpeg -listen 1 -timeout 10000 -f flv -loglevel " + loglevel + " -an -i rtmp://" + addr1 + ":" + str(port_num_lsb) + "/ -c:v copy -pix_fmt yuv420p10le -y Testing_DIR/test_lsb.mp4"
    cmd2 = "ffmpeg -listen 1 -timeout 10000 -f flv -loglevel " + loglevel + " -an -i rtmp://" + addr1 + ":" + str(port_num_lsb + 1) + "/ -c:v copy -pix_fmt rgb24 -y Testing_DIR/test_msb.mp4"
    cmd_lst.append(cmd1)
    cmd_lst.append(cmd2)
    run_processes_parallel(cmd_lst)
    if platform.system() == "Linux":
        os.system("stty echo")
    
    #
    #p1.s')
    #print()
