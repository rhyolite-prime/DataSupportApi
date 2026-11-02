# MagmaNet — RouterOS v7 gateway WireGuard client (paste with /import).
# Replace all CHANGE_ME values. The gateway initiates the tunnel, so it works
# behind CGNAT with no port forwarding.

/interface wireguard
add name=wg-magma mtu=1420 listen-port=51820 private-key="CHANGE_ME_GATEWAY_PRIVATE_KEY" \
    comment="MagmaNet control-plane tunnel"

/interface wireguard peers
add interface=wg-magma public-key="CHANGE_ME_SERVER_PUBLIC_KEY" \
    preshared-key="CHANGE_ME_PSK" endpoint-address="vps.magmanet.gh" endpoint-port=51820 \
    allowed-address=10.200.0.0/24 persistent-keepalive=25s comment="MagmaNet VPS"

/ip address
add address=10.200.1.10/24 interface=wg-magma comment="MagmaNet tunnel IP"

# Management traffic (REST :80) must only be reachable through the tunnel.
/ip firewall filter
add chain=input in-interface=wg-magma action=accept comment="MagmaNet control plane"
add chain=input in-interface=ether1 protocol=tcp dst-port=8291,80,443 action=drop \
    comment="Block management from the internet"

# Health check: the API marks a device OFFLINE when /rest/system/resource fails.
/tool netwatch
add host=10.200.0.1 interval=1m comment="MagmaNet RADIUS/API reachability" \
    down-script="/log warning \"MagmaNet control plane unreachable\"" 
