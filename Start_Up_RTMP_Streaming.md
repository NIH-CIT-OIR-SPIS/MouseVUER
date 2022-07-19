## First please configure for static IP addresses:

# On server
Go to Settings -> Network -> Wired -> [Click on Settings Symbol] -> IPv4 -> Manual:
    Then enter the following into their respective boxes
    In Addresses section
        Address: 192.168.1.299 
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


# On each client camera
Go to Settings -> Network -> Wired -> [Click on Settings Symbol] -> IPv4 -> Manual:
    Then enter the following into their respective boxes
    In Addresses section
        Address: 192.168.1.2xx    (Here xx means any two digit number as long as it is unique)
        Netmask: 255.255.255.0
        Gateway: 192.168.1.1
    In DNS section:
        8.8.4.4,8.8.8.8
    Then hit apply



