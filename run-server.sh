#!/bin/sh
#
# Usage: ./run-server.sh <group-number> <optional-mode>
#
# If executed with no optional mode: it pulls, compiles and runs the latest commit.
# If executed with optional mode -m: it pulls, compiles and runs the milestone
# commit as specified in the 3rd argument in repositories.csv.
# If executed with optional mode -f: it pulls, compiles and runs the final
# commit as specified in the 4th arguement in repositories.csv.

PORT=5003
TOT_GROUPS=`cat repositories.csv | wc -l`

if ! [ $1 -eq $1 ] 2> /dev/null
then
  echo "Error. Usage: $0 <group-number>"
  exit 1
fi

if [ $1 -lt 0 ]; then
  echo "Error. Usage: $0 <group-number>"
  exit 1
fi

if [ $1 -ge $TOT_GROUPS ]; then
  echo "Error. Usage: $0 <group-number>"
  exit 1
fi

GROUP_NO=$1
LINE=`expr $GROUP_NO + 1`
REPO_NAME=`cat repositories.csv | head -n $LINE | tail -n 1 | cut -d, -f1`
REPO_URL=`cat repositories.csv | head -n $LINE | tail -n 1 | cut -d, -f2`
MILESTONE_COMMIT=`cat repositories.csv | head -n $LINE | tail -n 1 | cut -d, -f3`
FINAL_COMMIT=`cat repositories.csv | head -n $LINE | tail -n 1 | cut -d, -f4`

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

COMMIT="HEAD"
if [ $2 = "-m" ] 2> /dev/null
then
  echo "Checking out milestone commit: $MILESTONE_COMMIT."
  COMMIT=$MILESTONE_COMMIT
fi

if [ $2 = "-f" ] 2> /dev/null
then
  echo "Checking out final commit: $FINAL_COMMIT."
  COMMIT=$FINAL_COMMIT
fi

if [ $COMMIT = "0" ]; then
  echo "Commit not specified. Exiting..."
  exit 1
elif [ $COMMIT = "HEAD" ]; then
  echo "Proceeding with HEAD..."
else
  git checkout $COMMIT
fi

echo "Compiling..."
find . -exec touch {} \;
make clean 2> /dev/null
find . -exec touch {} \;
make

echo "Starting server..."
cd ..
./$REPO_NAME/server $PORT
