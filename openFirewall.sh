#! /bin/sh

## Open firewall for COWE socket connection
#iptables -I INPUT 3 -p tcp -m tcp -j ACCEPT -s 0.0.0.0/0 -d 192.168.0.19
iptables -I INPUT 3 -p tcp -m tcp -j ACCEPT -s 0.0.0.0/0 -d 10.10.10.46
