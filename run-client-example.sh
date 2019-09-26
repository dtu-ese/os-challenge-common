#!/bin/sh
#
# This is an indicative configuration of the final run. My not-so-clever server is 
# able to process it in around 18 minutes on my laptop. The final configuration
# will be released after the milestone deadline. The DIFFICULTY and DELAY_US
# might be modified to match the processing power of the hardware to be used
# for the final test.
#

SERVER=192.168.101.10
PORT=5003
SEED=0
TOTAL=1000
START=0
DIFFICULTY=30000000
REP_PROB_PERCENT=20
DELAY_US=750000
PRIO_LAMBDA=1.5

/home/vagrant/os-challenge-common/client $SERVER $PORT $SEED $TOTAL $START $DIFFICULTY $REP_PROB_PERCENT $DELAY_US $PRIO_LAMBDA
