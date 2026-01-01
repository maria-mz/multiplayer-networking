#!/bin/bash

set -x

# Reset everything
sudo dnctl -q flush
sudo pfctl -F all -f /etc/pf.conf
sudo pfctl -d

# Enable pf
sudo pfctl -E

# Create pipes
sudo dnctl pipe 1 config delay 20 plr 0   # TCP
sudo dnctl pipe 2 config delay 50 plr 0   # UDP

# Apply rules
sudo pfctl -f - <<EOF
dummynet in proto tcp from 127.0.0.1 to 127.0.0.1 pipe 1
dummynet out proto tcp from 127.0.0.1 to 127.0.0.1 pipe 1
dummynet in proto udp from 127.0.0.1 to 127.0.0.1 pipe 2
dummynet out proto udp from 127.0.0.1 to 127.0.0.1 pipe 2
EOF

# Verify
sudo dnctl list
sudo pfctl -sr
