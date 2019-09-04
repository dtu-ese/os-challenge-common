#!/bin/sh

PORT=5003

REPO_NAME=os-challenge               # Change this to your repo name
REPO_ROOT=/home/vagrant/$REPO_NAME   # Change this to your repo path

$REPO_ROOT/server $PORT
