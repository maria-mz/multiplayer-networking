#!/bin/bash

set -x

sudo dnctl -q flush
sudo pfctl -F all -f /etc/pf.conf
sudo pfctl -d
