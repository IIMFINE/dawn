#!/bin/bash
path=$(cd `dirname $0`;pwd)
echo $path
cd $path

docker build --no-cache -f Dockerfile --target build_base -t dawn_base .
docker build --no-cache -f Dockerfile --target build_dependence -t simplepan/dawn_dev:latest .
