#!/usr/bin/env bash
export CC=clang35
export CXX=clang++35
export LINK=clang++35

export CFLAGS="-Wno-unused-variable -Wno-unused-function"
export LDFLAGS="-L/usr/local/lib -lexecinfo"
export CPPFLAGS="-I/usr/local/include"
./configure --prefix=/usr/chroot/arango 
#		--disable-all-in-one-v8

gmake -j 3 clean

