#!/bin/bash
# Simple update
sudo systemctl stop rpi-intercom.service
git stash
git pull
git stash apply
cmake .
make
sudo setcap 'cap_net_bind_service=+ep' rpi-intercom
sudo systemctl start rpi-intercom.service
