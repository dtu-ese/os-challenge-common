#!/bin/sh

apt update
apt upgrade -y

apt-get install -y git
apt-get install -y build-essential

echo "export PATH=/home/vagrant/os-challenge-common/:$PATH" >> /home/vagrant/.bashrc

echo "bootstrap.sh done"

