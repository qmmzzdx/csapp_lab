#!/bin/bash

./hex2raw < clevel1.txt | ./ctarget -q
./hex2raw < clevel2.txt | ./ctarget -q
./hex2raw < clevel3.txt | ./ctarget -q
./hex2raw < rlevel2.txt | ./rtarget -q
./hex2raw < rlevel3.txt | ./rtarget -q
