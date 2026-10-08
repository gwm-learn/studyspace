# Makefile for OpenWRT system

INCLUDE_DIR += ../inc
CFLAGS += -I$(INCLUDE_DIR) \
	  -Wundef \
	  -Wstrict-prototypes \
	  -Wno-trigraphs \
	  -g -O0 \
	  -fno-inline \
	  -fno-short-enums \


RM := rm -rf

LIBQCMAP_LAN_CLIENT := libqcmap_lan_client.so
LIBQCMAP_CLIENT := libqcmap_client.so
QCMAP_CLIENT := QCMAP_CLI
QCMAP_MNGR := qcmap_mngr

all: $(LIBQCMAP_LAN_CLIENT) $(LIBQCMAP_CLIENT) $(QCMAP_CLIENT) $(QCMAP_MNGR)

#libqcmap_lan_client
LIBQCMAP_LAN_CLIENT_SRCS = QCMAP_LAN_Client.cpp QCMAP_LAN_Multimedia.cpp QCMAP_LAN_Util.cpp QCMAP_EZMesh_Client.cpp QCMAP_LAN_Firewall_Client.cpp QCMAP_LAN_Natalg_Client.cpp QCMAP_LAN_Vlan_Client.cpp QCMAP_LAN_Wlan_Client.cpp QCMAP_LAN_Ippt_Client.cpp QCMAP_LAN_GSB_Client.cpp  QCMAP_WLAN_Common.cpp QCMAP_WLAN_HMT.cpp QCMAP_WLAN_WKK.cpp QCMAP_LAN_IPsec_Client.cpp

LIBQCMAP_LAN_CLIENT_OBJS := $(LIBQCMAP_LAN_CLIENT_SRCS:.cpp=.o)

$(LIBQCMAP_LAN_CLIENT): $(LIBQCMAP_LAN_CLIENT_OBJS)
			$(CC) $(LIBQCMAP_LAN_CLIENT_OBJS) $(CFLAGS) $(LDFLAGS) -shared -o $@
			mkdir -p lib
			mv libqcmap_lan_client.so lib/

#libqcmap_client
LIBQCMAP_CLIENT_SRCS = QCMAP_Client.cpp QCMAP_Client_OWRT.cpp

LIBQCMAP_CLIENT_OBJS := $(LIBQCMAP_CLIENT_SRCS:.cpp=.o) $(LIBQCMAP_LAN_CLIENT_OBJS)

$(LIBQCMAP_CLIENT): $(LIBQCMAP_CLIENT_OBJS)
			$(CC) $(LIBQCMAP_CLIENT_OBJS) $(CFLAGS) $(LDFLAGS) -lqcmap_lan_client -lqmi_cci -lqmi_encdec -lqmiservices -lqcmaputils -lqmi_ip -shared -o $@
			mkdir -p lib
			mv libqcmap_client.so lib/

#QCMAP_CLIENT
LIBPATH := ./lib
LDFLAGS += -L$(LIBPATH)

QCMAP_CLIENT_SRCS = qcmap_cli_main.cpp

QCMAP_CLIENT_OBJS := $(QCMAP_CLIENT_SRCS:.cpp=.o)

$(QCMAP_CLIENT): $(QCMAP_CLIENT_OBJS)
		$(CC) $(QCMAP_CLIENT_OBJS) $(CFLAGS) -fpie -pie  $(LDFLAGS) -lqcmap_client -lqmi_cci -lqmi_encdec -lqmiservices -lqcmaputils -lqmi_ip -o $@
		mkdir -p bin
		mv QCMAP_CLI bin/

#qcmap_mngr
QCMAP_MNGR_SRCS = qcmap_mngr.cpp  qcmap_mngr_util.cpp qcmap_mngr_config.cpp qcmap_mngr_modem.cpp qcmap_mngr_info.cpp qcmap_mngr_apn.cpp qcmap_mngr_led.cpp qcmap_mngr_lock.cpp qcmap_mngr_wan.cpp
QCMAP_MNGR_OBJS := $(QCMAP_MNGR_SRCS:.cpp=.o)
$(QCMAP_MNGR): $(QCMAP_MNGR_OBJS)
		$(CC) $(QCMAP_MNGR_OBJS) $(CFLAGS) -fpie -pie  $(LDFLAGS) -lqcmap_client -lqmi_cci -lqmi_encdec -lqmiservices -lqcmaputils -lqmi_ip -luci -o $@
		mkdir -p bin
		mv qcmap_mngr bin/

clean:
	$(RM) lib/
	$(RM) bin/
	$(RM) *.o
