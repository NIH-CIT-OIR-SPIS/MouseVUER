import ssl
import subprocess as sp
import os
COMMON_NAME = "SCHORE_SERVER"
ORGANIZATION = "NIH"
COUNTRY_ORGIN = "US"
if os.path.isdir("keys/"):
    os.system("rm -rf keys/")
    print("Making new keys and certificates...")
    os.system("mkdir keys/")

sp.run("openssl req -new -newkey rsa:2048 -days 5000 -nodes -x509 -subj '/CN=" + COMMON_NAME + "/O=" + ORGANIZATION + " LTD./C=" + COUNTRY_ORGIN +  "' -keyout  keys/server.key -out keys/server.crt ", shell=True) # Generate a self-signed certificate
sp.run("openssl req -new -newkey rsa:2048 -days 5000 -nodes -x509 -subj '/CN=" + COMMON_NAME + "/O=" + ORGANIZATION + " LTD./C=" + COUNTRY_ORGIN +  "' -keyout  keys/client.key -out keys/client.crt ", shell=True) # Generate a self-signed certificate