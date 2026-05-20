#!/bin/bash

if [ -d builddir ] ; then
	echo "build directory exists"
else
	meson setup builddir
fi
ninja -C builddir $1
