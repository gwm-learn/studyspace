#!/bin/sh
# dhcp.@dnsmasq[0].dhcpscript='/etc/dnsmasq_script.sh' debug dnsmasq detial

echo "######################## dnsmasq action:[$1] mac:[$2] ip:[$3] hostname:[$4] #########################" > /dev/console
echo "######################## dnsmasq action:[$1] mac:[$2] ip:[$3] hostname:[$4] #########################" > /dev/kmsg
