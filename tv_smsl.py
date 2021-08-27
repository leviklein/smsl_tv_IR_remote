#!/usr/bin/env python3

from time import sleep

import logging

import socket
import subprocess
import json

HOST = '192.168.2.31'  # The server's hostname or IP address
PORT = 20060        # The port used by the server

FILENAME = '/etc/tv_smsl/smsl_status.json'
BASE_COMMAND = ["/usr/local/bin/irrp.py", "-p", "-g", "17", "-f", "/etc/tv_smsl/smsl_ir_codes"]

PING_MSG = bytes('*SEPOWR################\n', 'utf-8')
PING_TIMEOUT = 5
DEFAULT_TIMEOUT = 60 * 15 # in seconds

status = {}

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def get_status():
    with open(FILENAME, "r") as read_file:
        status = json.load(read_file) 
        return status


def write_status():
    with open(FILENAME, "w") as write_file:
        json.dump(status, write_file, indent=4) 


def run_command(command_list):
    logger.debug(command_list)
    subprocess.run(command_list)


def process_data(data):
    data = data.decode('utf-8')
    data = data.split("\n")
    data.pop()
    
    volume_commands = []
    mute_commands = []
    power_commands = []

    for i in data:

        if "SNVOLU" in i:
            volume_commands.append(i)
        elif "SNAMUT" in i:
            mute_commands.append(i)
        elif "SNPOWR" in i:
            power_commands.append(i)

    if volume_commands:
        volume_change(volume_commands[-1])

    if mute_commands:
        mute(mute_commands[-1])

    if power_commands:
        power_change(power_commands[-1])

def mute(data):
    logger.debug(data)
    mute = int(data[-1])

    if mute:
        logger.info("Mute")
        command_list = BASE_COMMAND + ["key_down"] * 5 # volume down to reset to 0
        run_command(command_list)
    else:
        logger.info("Unmute")


def volume_change(data):
    logger.info("Volume change!")
    tv_volume = int(data[-3:])
    new_volume = tv_volume // 2 # max 50 only
    delta_volume = new_volume - status["volume"]

    logger.info("TV volume: {}, amp_volume: {}".format(tv_volume, new_volume))

    command_list = []
    if delta_volume > 0:
        command_list = BASE_COMMAND + ["key_up"] * delta_volume

    elif delta_volume < 0: 
        command_list = BASE_COMMAND + ["key_down"] * abs(delta_volume)

    if command_list:
        run_command(command_list)
        status["volume"] = new_volume
        write_status()


def power_change(data):
    command_list = BASE_COMMAND + ["key_power"] 
    run_command(command_list)

    power_on = int(data[-1])

    if power_on:
        logger.info("Power on")
        sleep(3)
    else:
        logger.info("Power off")
        

### main
logger.info("Program started!")

try:
    status = get_status()
    logger.info("Read current status from file: {}".format(status))
except:
    status = { "volume" : 20 }
    logger.warning("No file found. Writing status to {}:".format(FILENAME))
    logger.info(status)
    write_status()
    logger.info("Write succesful!")

while True:    
    logger.info("Creating socket..")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        logger.info("Connecting to TV..")
        s.connect((HOST, PORT))
        logger.info("Connected.")

        s.settimeout(DEFAULT_TIMEOUT)
        sent_ping = False

        while True:
            try:
                data = s.recv(1024)
                s.settimeout(DEFAULT_TIMEOUT)
                sent_ping = False
                logger.debug(data)
                if data:
                    process_data(data)
                else:
                    logger.error("Connection to TV was closed. Reconnecting..")
                    break
            except:
                if not sent_ping:
                    logger.debug("Timeout! Pinging TV..")
                    s.settimeout(PING_TIMEOUT)
                    logger.debug(PING_MSG)
                    s.send(PING_MSG)
                    sent_ping = True
                else:
                    logger.error("Ping failed, reconnecting to TV..")
                    break
        
        s.close()