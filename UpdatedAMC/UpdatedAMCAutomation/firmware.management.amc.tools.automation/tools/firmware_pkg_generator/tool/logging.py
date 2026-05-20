#!/usr/bin/env python3

#This tool is inherited from freertos source code as in the below link:
#https://github.com/intel-innersource/firmware.management.amc.amc-gfx-ti

RESET_COLOR = '\033[0m'
RED = '\033[1;31m'
GREEN ='\033[1;32m'
YELLOW ='\033[1;33m'
PURPLE = "\033[1;35m"
CYAN = "\033[1;36m"
WHITE = "\033[1;37m"

def red(str):
    print(RED+str+RESET_COLOR,end="")

def green(str):
    print(GREEN+str+RESET_COLOR,end="")

def yellow(str):
    print(YELLOW+str+RESET_COLOR,end="")

def purple(str):
    print(PURPLE+str+RESET_COLOR,end="")

def cyan(str):
    print(CYAN+str+RESET_COLOR,end="")

def white(str):
    print(WHITE+str+RESET_COLOR,end="")
