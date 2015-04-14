#!/usr/bin/env bash
export CC=clang
export CXX=clang++
export LINK=clang++

export CFLAGS="-Wno-unused-variable -Wno-unused-function"
export LDFLAGS="-L/usr/local/lib -lexecinfo"
export CPPFLAGS="-I/usr/local/include"
./configure --prefix=/usr/chroot/arango 
#		--disable-all-in-one-v8

gmake -j 4 clean

