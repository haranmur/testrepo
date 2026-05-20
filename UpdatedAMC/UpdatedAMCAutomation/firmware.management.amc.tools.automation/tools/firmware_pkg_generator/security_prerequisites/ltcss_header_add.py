#!/usr/bin/env python

# INTEL CONFIDENTIAL
# Copyright 2023 Intel Corporation
#
# This software and the related documents are Intel copyrighted materials,
# and your use of them is governed by the express license under which they
# were provided to you (License). Unless the License provides otherwise,
# you may not use, modify, copy, publish, distribute, disclose or
# transmit this software or the related documents without
# Intel's prior written permission.
#
# This software and the related documents are provided as is,
# with no express or implied warranties, other than those
# that are expressly stated in the License.

import sys
import os

ERROR = 'Usage error: '+sys.argv[0]+' filename (0 - add header, 1 - append header) (file size)'

MODULE_TYPE =          b'\x14\x00\x00\x00'
HEADER_LEN =           b'\x33\x00\x00\x00'
HEADER_VERSION =       b'\x00\x00\x02\x00'
filesize = 0

if len(sys.argv) < 3:
    print(ERROR)
    exit(1)

if os.path.isfile(sys.argv[1]):
    filename = sys.argv[1]
else:
    print(ERROR)
    exit(1)

try:
    filesize = int(sys.argv[3])
except:
    print(ERROR)
    exit(1)


try:
    if sys.argv[2] == '0':
        with open(filename+'_tmp', 'wb') as f:
            f.write(b'\0'*204)
            fin = open(filename, 'rb')
            f.write(fin.read())
            padding_size = (filesize-fin.tell())
            fin.close()
            f.close()
    elif sys.argv[2] == '1':
        with open(filename+'_tmp', 'wb') as f:
            fin = open(filename, 'rb')
            f.write(fin.read())
            padding_size = (filesize-fin.tell())
            fin.close()
            f.write(b'\0'*padding_size)
            f.close()
    os.remove(filename)
    os.rename(filename+"_tmp",filename)
except:
    print(ERROR)
    exit(2)

try:
    with open(filename, 'r+b') as f:
        f.seek(0)
        f.write(MODULE_TYPE)
        f.write(HEADER_LEN)
        f.write(HEADER_VERSION)
        f.close()
except:
    print(ERROR)
    exit(1)
exit(0)