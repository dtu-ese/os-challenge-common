#!/bin/sh
#
# Usage: ./run-server.sh <group-number>

TOT_GROUPS=14
PORT=5003

if ! [ $1 -eq $1 ] 2> /dev/null
then
  echo "Error. Usage: $0 <group-number>"
  exit 1
fi

if [ $1 -lt 0 ]; then
  echo "Error. Usage: $0 <group-number>"
  exit 1
fi

if [ $1 -gt $TOT_GROUPS ]; then
  echo "Error. Usage: $0 <group-number>"
  exit 1
fi

GROUP_NO=$1
REPO_NAME=`cat repositories.csv | head -n $GROUP_NO | tail -n 1 | cut -d, -f1`
REPO_URL=`cat repositories.csv | head -n $GROUP_NO | tail -n 1 | cut -d, -f2`
MILESTONE_COMMIT=`cat repositories.csv | head -n $GROUP_NO | tail -n 1 | cut -d, -f3`
FINAL_COMMIT=`cat repositories.csv | head -n $GROUP_NO | tail -n 1 | cut -d, -f4`

echo "Group Number: $GROUP_NO"
echo "Repo Name: $REPO_NAME"
echo "Repo URL: $REPO_URL"
echo "Milestone Commit: $MILESTONE_COMMIT"
echo "Final Commit:" $FINAL_COMMIT

if [ -d "$REPO_NAME" ]; then
  echo "Repo already exists. Pulling the latest version..."
  cd $REPO_NAME
  git checkout master
  git pull
else
  echo "Cloning the repo..."
  git clone $REPO_URL
  cd $REPO_NAME
fi

if [ $2 = "-m" ] 2> /dev/null
then
  echo "Checking out milestone commit: $MILESTONE_COMMIT."
  COMMIT=$MILESTONE_COMMIT
else
  echo "Checking out final commit: $FINAL_COMMIT."
  COMMIT=$FINAL_COMMIT
fi

if [ $COMMIT = "0" ]; then
  echo "Commit not specified. Exiting..."
  exit 1
else
  git checkout $COMMIT
fi

echo "Compiling..."
make clean 2> /dev/null
make all

echo "Starting server..."
cd ..
./$REPO_NAME/server $PORT
