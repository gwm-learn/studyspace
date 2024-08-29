#!/bin/sh

CONFIGFILE="/var/net-snmp/conf/snmpd.conf"

killall -SIGUSR1 snmpd

killall snmpd

echo "#####################SNMPD STOP#########################" > $CONFIGFILE

echo "[SNMPD] snmp service stopped"
