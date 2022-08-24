# Directions for Configuring Server Client System

# Hardware Requirements.
Server computer should have more CPU cores for every client (Recommend for 3 or more systems should be at least 6th gen i5 desktop CPU or equivilant)

Client computers can be found for cheap if used (around $150 or less) for example (a used HP EliteDesk 800 G2 Mini Business Desktop with an i5-6500T is sufficiently powerful enough.)

Our configuration:
    Server (1):
        HP EliteDesk 800 G2 Mini Business Desktop with 16GB RAM and 5TB external drive
    
    Clients (multiple):
        HP EliteDesk 800 G2 Mini Business Desktop with 8GB RAM, and 128GB SSD     

Depth Cameras:
    Intel&reg; RealSense&trade; D435 Stereo Depth Cameras

Ubuntu 20.04 LTS is installed on each computer. See more configuration settings in the README.md document


## First please configure for static IP addresses:

### On server
Go to Settings -> Network -> Wired -> [Click on Settings Symbol] -> IPv4 -> Manual:

    Then enter the following into their respective boxes
    In Addresses section
        Address: 192.168.1.234
        Netmask: 255.255.255.0
        Gateway: 192.168.1.1
    In DNS section:
        8.8.4.4,8.8.8.8
    Then hit apply

**NOTE IF YOU HAVE ANY FIREWALL SETTINGS **
1) Open a terminal
2) Type in 
```
sudo ufw status verbose
```
3) You will see either an active status or inactive status
4) If you have an active status add rules so you can accept your client computer cameras

5) Type the following into a terminal 
```
sudo ufw allow proto tcp 192.168.1.201 to any port 5000 && sudo ufw allow proto tcp 192.168.1.201 to any port 5001 && \
sudo ufw allow proto tcp 192.168.1.202 to any port 5002 && sudo ufw allow proto tcp 192.168.1.202 to any port 5003 && \
sudo ufw allow proto tcp 192.168.1.203 to any port 5004 && sudo ufw allow proto tcp 192.168.1.202 to any port 5004 && \
.
.
.
...

```
If need be you can remove all the firewall rules by doing
```
sudo ufw status verbose
```
Looking at the number in the list at which the rule occurs at

```
sudo ufw delete [x] # Where x is the number in the list at which the rule occurs at which you want to delete
```
Key generation
Type the following in to a terminal on your Server
```
cd ~/DepthCameraRecordingTool/Code_8_bit/ && \
make clean && make -j4 && \
python3 make_ssl_keys_cert.py
```

### On Client computers
Go to Settings -> Network -> Wired -> [Click on Settings Symbol] -> IPv4 -> Manual:
    Then enter the following into their respective boxes
    In Addresses section
        Address: 192.168.1.2xx    (Here xx means any two digit number as long as it is unique and not 34)
        Netmask: 255.255.255.0
        Gateway: 192.168.1.1
    In DNS section:
        8.8.4.4,8.8.8.8
    Then hit apply
    

### Copy Entire directory from Server to each client
Copy entire ~/DepthCameraRecordingTool/  directory to storage medium of your choice.
Then For ***each*** client computer:
    Navigate on the client computer to your home directory using the Files application on Ubuntu
    Paste a copy of the DepthCameraRecordingTool in this folder.


### For each Client 
```
cd ~/DepthCameraRecordingTool/Code_8_bit/ && \
make clean && \
make -j4
```

```
crontab -e
```
Then if prompted type in 1 and hit ENTER

Using the arrow keys go down to the very bottom of the file
Then add the following text replacing USER with your client username found if you go to settings and users:
```
@reboot sleep 10; export XAUTHORITY=/home/<USER>/.Xauthority; cd /home/<USER>/DepthCameraRecordingTool/Code_8_bit/ && python3 client_side.py &
```
Then hit the keys CTRL+X then Y then ENTER

Then reboot the system
```
sudo reboot
```

Repeat for each client

### Running Server

Running a recording for 30 seconds at a crf of 22
```
python3 server_side.py --dir Testing_DIR/ --num_clients 4 --json Default.json --time_run 30 --crf 22
```








