#!/usr/bin/env bash
export CC=clang35
export CXX=clang++35
export LINK=clang++35

export CFLAGS="-Wno-unused-private-field -Wno-unused-variable -Wno-nested-anon-types -Wno-unused-function"
export LDFLAGS="-L/usr/local/lib -lexecinfo"
export CPPFLAGS="-I/usr/local/include"

gmake -j 4
