#!/bin/sh

apt-get install -y git
apt-get install -y libssl-dev
apt-get install -y build-essential

echo "export PATH=/home/vagrant/os-challenge-common/:$PATH" >> /home/vagrant/.bashrc

echo "bootstrap.sh done"

