#!/bin/sh
#
# This is the configuration of the final run.
# The seed value will be a non-zero value that will not be disclosed.
#

PATHTOCOMMON=~/os/challenge/
SERVER=localhost
PORT=${1:-5002}
SEED=3435245
TOTAL=100
START=0
DIFFICULTY=30000000
REP_PROB_PERCENT=20
DELAY_US=600000
PRIO_LAMBDA=1.5


$PATHTOCOMMON/$(./get-bin-path.sh)/client $SERVER $PORT $SEED $TOTAL $START $DIFFICULTY $REP_PROB_PERCENT $DELAY_US $PRIO_LAMBDA
