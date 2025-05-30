#!/bin/sh
make clean

make SERVER=1 -j32 && make -j32 2>&1 | tee output/build.log