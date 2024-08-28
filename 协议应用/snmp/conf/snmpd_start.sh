#!/bin/sh

# config setting info load in /etc/config/snmpd
# remap /etc/config/snmpd to /var/net-snmp/conf/snmpd.conf

CONFIGFILE="/var/net-snmp/conf/snmpd.conf"

echo "#####################SNMPD START#########################" > $CONFIGFILE
echo "# config setting info load in /etc/config/snmpd" >> $CONFIGFILE
echo "# remap /etc/config/snmpd to /var/net-snmp/conf/snmpd.conf" >> $CONFIGFILE

echo "[SNMPD] agent_add"
cfg="@agent[0]"
agent_agentaddress=`uci -q get "snmpd.$cfg.agentaddress"`
[ -n "$agent_agentaddress" ] && echo "agentaddress $agent_agentaddress" >> $CONFIGFILE

echo "[SNMPD] system_add"
cfg="@system[0]"
system_syslocation=`uci -q get "snmpd.$cfg.sysLocation"`
system_syscontact=`uci -q get "snmpd.$cfg.sysContact"`
system_sysname=`uci -q get "snmpd.$cfg.sysName"`
[ -n "$system_syslocation" ] && echo "sysLocation $system_syslocation" >> $CONFIGFILE
[ -n "$system_syscontact" ] && echo "sysContact $system_syscontact" >> $CONFIGFILE
[ -n "$system_sysname" ] && echo "sysName $system_sysname" >> $CONFIGFILE

echo "[SNMPD] com2sec_add public"
cfg="public"
com2sec_secname=`uci -q get "snmpd.$cfg.secname"`
com2sec_source=`uci -q get "snmpd.$cfg.source"`
com2sec_community=`uci -q get "snmpd.$cfg.community"`
[ -n "$com2sec_secname" ] && [ -n "$com2sec_source" ] && [ -n "$com2sec_community" ] && echo "com2sec $com2sec_secname $com2sec_source $com2sec_community" >> $CONFIGFILE

echo "[SNMPD] com2sec_add private"
cfg="private"
com2sec_secname=`uci -q get "snmpd.$cfg.secname"`
com2sec_source=`uci -q get "snmpd.$cfg.source"`
com2sec_community=`uci -q get "snmpd.$cfg.community"`
[ -n "$com2sec_secname" ] && [ -n "$com2sec_source" ] && [ -n "$com2sec_community" ] && echo "com2sec $com2sec_secname $com2sec_source $com2sec_community" >> $CONFIGFILE

echo "[SNMPD] group_add public_v2c"
cfg="public_v2c"
group_group=`uci -q get "snmpd.$cfg.group"`
group_version=`uci -q get "snmpd.$cfg.version"`
group_secname=`uci -q get "snmpd.$cfg.secname"`
[ -n "$group_group" ] && [ -n "$group_version" ] && [ -n "$group_secname" ] && echo "group $group_group $group_version $group_secname" >> $CONFIGFILE

echo "[SNMPD] group_add private_v2c"
cfg="private_v2c"
group_group=`uci -q get "snmpd.$cfg.group"`
group_version=`uci -q get "snmpd.$cfg.version"`
group_secname=`uci -q get "snmpd.$cfg.secname"`
[ -n "$group_group" ] && [ -n "$group_version" ] && [ -n "$group_secname" ] && echo "group $group_group $group_version $group_secname" >> $CONFIGFILE

echo "[SNMPD] view_add"
cfg="all"
view_viewname=`uci -q get "snmpd.$cfg.viewname"`
view_type=`uci -q get "snmpd.$cfg.type"`
view_oid=`uci -q get "snmpd.$cfg.oid"`
view_mask=`uci -q get "snmpd.$cfg.mask"`
[ -n "$view_viewname" ] && [ -n "$view_type" ] && [ -n "$view_oid" ] && echo "view $view_viewname $view_type $view_oid $view_mask" >> $CONFIGFILE

echo "[SNMPD] access_add public_access"
cfg="public_access"
access_group=`uci -q get "snmpd.$cfg.group"`
access_context=`uci -q get "snmpd.$cfg.context"`
access_version=`uci -q get "snmpd.$cfg.version"`
access_level=`uci -q get "snmpd.$cfg.level"`
access_prefix=`uci -q get "snmpd.$cfg.prefix"`
access_read=`uci -q get "snmpd.$cfg.read"`
access_write=`uci -q get "snmpd.$cfg.write"`
access_notify=`uci -q get "snmpd.$cfg.notify"`
[ -n "$access_group" ] && [ -n "$access_context" ] && [ -n "$access_version" ] && [ -n "$access_level" ] && [ -n "$access_prefix" ] && [ -n "$access_read" ] && [ -n "$access_write" ] && [ -n "$access_notify" ] && echo "access $access_group $access_context $access_version $access_level $access_prefix $access_read $access_write $access_notify" >> $CONFIGFILE

echo "[SNMPD] access_add private_access"
cfg="private_access"
access_group=`uci -q get "snmpd.$cfg.group"`
access_context=`uci -q get "snmpd.$cfg.context"`
access_version=`uci -q get "snmpd.$cfg.version"`
access_level=`uci -q get "snmpd.$cfg.level"`
access_prefix=`uci -q get "snmpd.$cfg.prefix"`
access_read=`uci -q get "snmpd.$cfg.read"`
access_write=`uci -q get "snmpd.$cfg.write"`
access_notify=`uci -q get "snmpd.$cfg.notify"`
[ -n "$access_group" ] && [ -n "$access_context" ] && [ -n "$access_version" ] && [ -n "$access_level" ] && [ -n "$access_prefix" ] && [ -n "$access_read" ] && [ -n "$access_write" ] && [ -n "$access_notify" ] && echo "access $access_group $access_context $access_version $access_level $access_prefix $access_read $access_write $access_notify" >> $CONFIGFILE

echo "[SNMPD] sink_add"
cfg="@trapsink[0]"
sink_section="trapsink"
sink_host=`uci -q get "snmpd.$cfg.host"`
sink_community=`uci -q get "snmpd.$cfg.community"`
sink_port=`uci -q get "snmpd.$cfg.port"`
[ -n "$sink_section" ] && [ -n "$sink_host" ] && echo "$sink_section $sink_host:$sink_port $sink_community" >> $CONFIGFILE

enabled=`uci -q get snmpd.general.enabled`
if [ "$enabled" != "1" ]; then
    echo "[SNMPD] snmp is disabled. Do not start the service."
    exit 0
fi

snmpd -c /var/net-snmp/conf/snmpd.conf &
echo "[SNMPD] snmp service started"
