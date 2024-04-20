#!/bin/bash

make
./driver.py
make clean
rm *.tmp
