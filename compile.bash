#!/usr/bin/env bash
export CC=clang
export CXX=clang++
export LINK=clang++

export CFLAGS="-Wno-unused-private-field -Wno-unused-variable -Wno-nested-anon-types -Wno-unused-function"
export LDFLAGS="-L/usr/local/lib -lexecinfo"
export CPPFLAGS="-I/usr/local/include"

gmake -j 4
