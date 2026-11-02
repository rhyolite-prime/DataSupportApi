# MagmaNet — sector AP in bridge mode.
#
# Hardware note: the mANTBox ax 15s (L22UGS-5HaxD2HaxD-15S) is a COMPLETE
# dual-band sector base station (IPQ-5010 + QCN-6102, RouterOS v7, 2.4 GHz 12 dBi
# / 5 GHz 15 dBi, passive PoE 18-28 V — never 48 V). The bare mANT 15s
# (MTAS-5G-15D120) is a passive antenna and needs a separate BaseBox/NetBox/
# NetMetal radio; in that case apply only the wireless section below.
#
# This AP does no hotspot/DHCP/RADIUS work: it bridges the wireless clients to
# the gateway. That keeps the data path simple and lets the gateway enforce
# rates.

/interface bridge
add name=br-lan protocol-mode=none comment="MagmaNet L2 to gateway"

/interface bridge port
add bridge=br-lan interface=ether1 comment="Trunk to gateway"
add bridge=br-lan interface=wifi1 comment="5 GHz sector"
add bridge=br-lan interface=wifi2 comment="2.4 GHz sector"

# RouterOS v7 /interface wifi (wifi-qcom driver on mANTBox ax).
/interface wifi
set wifi1 disabled=no mode=ap ssid="MagmaNet-Kaneshie-5G" \
    channel.frequency=5180-5825 channel.width=20mhz \
    security.authentication-types=wpa2-psk,wpa3-psk security.passphrase="CHANGE_ME_WIFI_PASSWORD" \
    configuration.country="Ghana" configuration.installation=outdoor
set wifi2 disabled=no mode=ap ssid="MagmaNet-Kaneshie" \
    channel.frequency=2412-2472 channel.width=20mhz \
    security.authentication-types=wpa2-psk security.passphrase="CHANGE_ME_WIFI_PASSWORD" \
    configuration.country="Ghana" configuration.installation=outdoor

# Legacy /interface wireless variant (mANTBox 15s, RB921GS — EOL but common):
# /interface wireless set wlan1 mode=ap-bridge ssid="MagmaNet-Kaneshie-5G" \
#     band=5ghz-a/n/ac frequency=auto channel-width=20mhz country=ghana

/ip dhcp-client
add interface=br-lan disabled=yes comment="AP is L2-only; gateway runs DHCP"

/ip service
set telnet disabled=yes
set ftp disabled=yes
set www disabled=yes
set api disabled=no
set api-ssl disabled=yes

/ip firewall filter
add chain=input connection-state=established,related action=accept
add chain=input in-interface=wg-magma action=accept
add chain=input protocol=icmp action=accept
add chain=input action=drop

# Optional: reuse the gateway's tunnel for REST management of the AP itself.
# Import deploy/magma/wireguard/gateway-wg.rsc with a unique 10.200.1.x address.
