# MagmaNet — zone gateway bootstrap (RB5009 / CCR2004 / hEX S), RouterOS v7.
# The API can push most of this via /rest (MagmaDeviceService::pushDeviceConfig);
# this script is for first install and disaster recovery.
#
# Replace CHANGE_ME values. Never expose the REST API to the WAN: it is only
# reachable on the WireGuard interface.

# ---------------------------------------------------------------- identity ---
/system identity set name="mg-KANESHIE-gw"
/user add name=magma-api group=full password="CHANGE_ME_API_PASSWORD" \
    comment="MagmaNet REST user" address=10.200.0.0/24
/ip service
set telnet disabled=yes
set ftp disabled=yes
set www disabled=yes
set api disabled=yes
set api-ssl disabled=no

# ------------------------------------------------------------------ radius ---
/radius
add service=hotspot,login address=10.200.0.1 secret="CHANGE_ME_RADIUS_SECRET" \
    authentication-port=1812 accounting-port=1813 timeout=1s comment="MagmaNet VPS"
/radius incoming set accept=yes port=3799

# ---------------------------------------------------------------- hotspot ----
# Bridge each sector AP into the LAN; the gateway owns DHCP/hotspot/queues.
/interface bridge add name=br-hotspot comment="MagmaNet hotspot LAN"
/ip address add address=10.10.0.1/22 interface=br-hotspot
/ip pool add name=hs-pool ranges=10.10.0.10-10.10.3.254
/ip dhcp-server add name=hs-dhcp interface=br-hotspot address-pool=hs-pool lease-time=1h disabled=no
/ip dhcp-server network add address=10.10.0.0/22 gateway=10.10.0.1 dns-server=1.1.1.1,8.8.8.8

/ip hotspot profile
add name=magma-hs hotspot-address=10.10.0.1 dns-name="login.magmanet.gh" \
    html-directory=hotspot login-by=http-chap,http-pap,mac-cookie \
    use-radius=yes radius-accounting=yes radius-interim-update=1m \
    mac-auth-mode=mac-as-username-and-password nas-port-type=wireless-802.11

/ip hotspot
add name=hs-kaneshie interface=br-hotspot address-pool=hs-pool profile=magma-hs \
    disabled=no comment="Kaneshie market sector"

# Pre-auth walled garden: portal + payment providers.
/ip hotspot walled-garden
add dst-host="portal.magmanet.gh" action=accept comment="MagmaNet portal"
add dst-host="*.hubtel.com" action=accept
add dst-host="*.paystack.com" action=accept
add dst-host="*.mtn.com.gh" action=accept comment="MTN MoMo prompt"

# Local user cache survives a tunnel outage: the hotspot keeps honouring
# existing sessions, but new logins require FreeRADIUS.
/ip hotspot user profile
add name=magma-fallback shared-users=1 rate-limit="2M/2M" session-timeout=1h \
    comment="Offline fallback only; MagmaNet overwrites via RADIUS"

# ---------------------------------------------------------------- security ---
/ip firewall filter
add chain=input connection-state=established,related action=accept
add chain=input in-interface=wg-magma action=accept comment="MagmaNet control plane"
add chain=input protocol=icmp action=accept
add chain=input action=drop comment="default deny"
add chain=forward connection-state=established,related action=accept
add chain=forward in-interface=br-hotspot out-interface=ether1 action=accept
add chain=forward action=drop

/ip firewall nat
add chain=srcnat out-interface=ether1 action=masquerade

# ---------------------------------------------------------------- backup -----
/system scheduler
add name=magma-nightly-backup interval=1d start-time=02:30:00 \
    on-event="/system backup save name=(\"mg-\" . [/system identity get name] . \"-\" . [/system clock get date])"
