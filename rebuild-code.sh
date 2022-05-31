#!/bin/sh

set -ex

make clean
./autogen.sh
./configure
make && echo "OK!"
