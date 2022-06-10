#!/bin/bash
# @file make.sh
# @brief use doker for mac

# docker build -f . -t compilerbook Dockerfile
docker run --rm -v ~/compilerbook:/9cc -w /9cc compilerbook make
docker run --rm -v ~/compilerbook:/9cc -w /9cc compilerbook make test
docker run --rm -v ~/compilerbook:/9cc -w /9cc compilerbook make clean
