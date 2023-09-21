#!/bin/sh

PATHTOCOMMON=/home/vagrant/os-challenge-common
PORT=5003

$PATHTOCOMMON/$(./get-bin-path.sh)/server $PORT
