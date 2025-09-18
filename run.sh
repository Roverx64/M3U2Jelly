#!/bin/bash
if [ ! -d ./convert ]; then
    mkdir ./convert
fi
if [ ! -d ./output ]; then
    mkdir ./output
fi
gcc ./src/main.c -o ./main.o -D_GNU_SOURCE
read -p "Add your M3U files to the 'convert' folder, then press enter to continue"
./main.o $1 $2
for file in ./output/*
do
    filename=$(basename $file)
    fname=${filename%.*}
    mkdir ./output/$fname
    mv $file "./output/$fname/playlist.xml"
done