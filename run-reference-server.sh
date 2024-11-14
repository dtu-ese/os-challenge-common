#!/bin/sh

PATHTOCOMMON=~/operating/byteforce/os-challenge-ByteForce
PORT=5003

$PATHTOCOMMON/$(./get-bin-path.sh)/server $PORT
