#!/bin/sh


PATHTOCOMMON=~/os/challenge/
# Get the first argument passed to the script, or default to 5002 if no argument is given
PORT=${1:-5002}

# Run the server with the given or default port
$PATHTOCOMMON/$(./get-bin-path.sh)/server $PORT
