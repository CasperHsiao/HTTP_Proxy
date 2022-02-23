#!/bin/bash
make clean

make

./proxyd


while true
do
    sleep 1
done
