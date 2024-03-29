#!/bin/sh
#
# This is the configuration of the milestone test run.
#

PATHTOCOMMON=/home/vagrant/os-challenge-common
SERVER=192.168.101.10
PORT=5003
SEED=3435245
TOTAL=100
START=0
DIFFICULTY=30000000
REP_PROB_PERCENT=20
DELAY_US=600000
PRIO_LAMBDA=1.5

$PATHTOCOMMON/$(./get-bin-path.sh)/client $SERVER $PORT $SEED $TOTAL $START $DIFFICULTY $REP_PROB_PERCENT $DELAY_US $PRIO_LAMBDA
