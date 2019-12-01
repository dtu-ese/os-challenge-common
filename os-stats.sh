
#!/bin/sh
#

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

echo "Group Number: $GROUP_NO"
echo "Repo Name: $REPO_NAME"

if [ -d "$REPO_NAME" ]; then
  echo "Repo already exists. Pulling the latest version..."
  cd $REPO_NAME
  git checkout master
  for remote in `git branch -r`; do git branch --track ${remote#origin/} $remote; done
  git pull --all
else
  echo "Repo does not exist"
  exit 1
fi

git shortlog -s -n --all --no-merges | cat

echo " "

git log --all --shortstat --no-merges --pretty="%an" | sed 's/\(.*\)@.*/\1/' | grep -v "^$" | awk 'BEGIN { line=""; } !/^ / { if (line=="" || !match(line, $0)) {line = $0 "," line }} /^ / { print line " # " $0; line=""}' | sort | sed -E 's/# //;s/ files? changed,//;s/([0-9]+) ([0-9]+ deletion)/\1 0 insertions\(+\), \2/;s/\(\+\)$/\(\+\), 0 deletions\(-\)/;s/insertions?\(\+\), //;s/ deletions?\(-\)//' | awk -F "," '{ gsub(" ", "-", $1); }1' | tr "," " " | awk 'BEGIN {name=""; files=0; insertions=0; deletions=0;} {if ($1 != name && name != "") { print name ": " files " files changed, " insertions " insertions(+), " deletions " deletions(-), " insertions-deletions " net"; files=0; insertions=0; deletions=0; name=$1; } name=$1; files+=$2; insertions+=$3; deletions+=$4} END {print name ": " files " files changed, " insertions " insertions(+), " deletions " deletions(-), " insertions-deletions " net";}'
