#!/bin/sh

uci set network.wg_client=interface
uci set network.wg_client.disabled='0'
uci set network.wg_client.proto='wireguard'
uci set network.wg_client.peerdns='0'
uci set network.wg_client.dns='8.8.8.8'
uci set network.wg_client.mtu='1200'
uci set network.wg_client.private_key='gMoncXZrhbrvW32Q4HRN7Ab//7C5+I1Gm6cyiomHYVY='
uci set network.wg_client.addresses='10.0.0.3/32'
uci set network.wireguard_wg_client=wireguard_wg_client
uci set network.wireguard_wg_client.allowed_ips='0.0.0.0/0'
uci set network.wireguard_wg_client.route_allowed_ips='1'
uci set network.wireguard_wg_client.public_key='2sr4T2ngRAuYq8Ky2Z7KDmhS3a1d+85aTVly/7bue2Q='
uci set network.wireguard_wg_client.description='wg_server'
uci set network.wireguard_wg_client.persistent_keepalive='25'
uci set network.wireguard_wg_client.endpoint_host='127.0.0.1'
uci set network.wireguard_wg_client.endpoint_port='50001'
uci set network.wireguard_wg_client.preshared_key='fcDI+yM7UqTwDLuoUyvfvakW4u+hxgQl2Ym9DqGfkIk='

uci commit