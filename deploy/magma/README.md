# MagmaNet deployment runbook

> Planning context — phases, test gates, KPIs, maintenance cadence and the
> go-live checklist — lives in [`MagmaNet_Roadmap.md`](../../MagmaNet_Roadmap.md).

MagmaNet is the community-WiFi control plane implemented inside DataSupportApi:
captive-portal checkout (MTN/Telecel MoMo, card), vouchers and agents, RADIUS
projection, MikroTik provisioning, monitoring, invoicing and reporting.

The **control plane is never in the data path**. If the API or FreeRADIUS is
unreachable, existing hotspot sessions keep working (RouterOS caches the last
`Access-Accept`); new purchases simply wait until the tunnel recovers.

```
               MTN / Telecel fibre (CGNAT is fine)
 hotspot clients ──► mANTBox ax 15s (bridge) ──► gateway (RB5009/CCR2004/hEX S)
                                                      │  WireGuard, outbound only
                                                      ▼
                                         VPS 10.200.0.1: FreeRADIUS + PostgreSQL
                                              + DataSupportApi (:5702)
```

## 1. Database

```bash
psql "$DATABASE_URL" -f migrations/0001_magma_schema.sql
psql "$DATABASE_URL" -f migrations/0002_radius_schema.sql
psql "$DATABASE_URL" -f migrations/0003_magma_seed.sql
# optional — only once session volume justifies it (rewrites magma_sessions):
psql "$DATABASE_URL" -f migrations/0004_partitioning.sql
```

`0004` is optional and nothing in the C++ depends on it. `magma_sessions` gets one
row per RADIUS session (not one per interim update), so a plain table is fine for the
first few thousand subscribers. Without `0004`, prune from cron instead of dropping
partitions:

```bash
psql "$DATABASE_URL" -c "DELETE FROM magma_sessions WHERE started_at < now() - interval '13 months';"
```

**ORM note:** the MagmaNet tables are accessed with hand-written SQL
(`execSqlCoro` + `utils/DbUtils.h`), not Drogon models. Running
`drogon_ctl create model` against this database will generate ~23 `Magma*` model
files that nothing uses — and a partitioned `magma_sessions` has no single-column
primary key, so its generated model would be unusable. If they appear, delete
`models/Magma*.h` and `models/Magma*.cc`; `CMakeLists.txt` only globs that
directory, so they disappear from the build automatically.

Provision a tenant and its default catalogue:

```sql
SELECT * FROM magma_provision_tenant('<business uuid>', 'KANESHIE', 'Kaneshie Market', 'Okaikoi South');
```

### Demo data (staging only)

`seeds/magma_demo_data.sql` fills every MagmaNet table so the admin, portal and
reporting screens have something to show: 3 zones, 6 devices, 12 subscribers with
subscriptions and usage, 39 accounting sessions, 30 payments, 4 invoices, 70
vouchers, 3 agents with wallets, 5 staff, tickets, work orders, alerts, audit rows,
outbox events and zone costs.

```bash
psql "$DATABASE_URL" \
     -v business_id="<business uuid>" \
     -v secrets_key="$MAGMA_SECRETS_KEY" \
     -v hmac_key="$MAGMA_HMAC_KEY" \
     -v pepper="$MAGMA_VOUCHER_PEPPER" \
     -f seeds/magma_demo_data.sql
```

Pass the **running** keys, otherwise the seeded voucher codes and device secrets
cannot be decrypted by the API.

`-v` only defines a psql variable; the script reads it as `:'secrets_key'`. If you
leave the flags out, the script falls back to these literal defaults — which are
exactly the values to hand the API, so the two agree:

```bash
export MAGMA_SECRETS_KEY='magmanet-demo-secrets-key-32-chars!'
export MAGMA_HMAC_KEY='magmanet-demo-hmac-key'
export MAGMA_VOUCHER_PEPPER='magmanet-demo-voucher-pepper'
psql "$DATABASE_URL" -f seeds/magma_demo_data.sql
```

`"$DATABASE_URL"` is any libpq connection string, e.g.
`postgresql://postgres:secret@localhost:5432/datasupport`; if you don't have that
variable, use the equivalent flags:
`psql -h localhost -U postgres -d datasupport -f seeds/magma_demo_data.sql`.
Find the tenant id with `SELECT id, business_name FROM business_accounts;` and pass it
as `-v business_id=...` when the database has more than one tenant. The script needs `pgcrypto` (contrib), is
idempotent, and never deletes anything. It prints the fixed test credentials:

| What | Value |
| --- | --- |
| Agent login | `0244555666` / PIN `1234` |
| Staff login | `0244000001` / PIN `2468` (ADMIN) |
| Voucher | `TEST-<tag>-001` where `<tag>` is printed by the script (plan DAY-1) |
| Hotspot login | `mg:<subscription uuid>` / `demo-radius-1` |
| RADIUS client | gateways `10.200.1.10`, `10.200.1.20`, `10.200.1.30` |

## 2. Secrets and configuration

`config.json → custom_config.magma_net` holds non-secret defaults. Production
must set:

| Variable | Purpose |
| --- | --- |
| `MAGMA_SECRETS_KEY` | AES-256 key for encrypted columns (RADIUS secrets, API passwords, Ghana Card, voucher codes) |
| `MAGMA_HMAC_KEY` | voucher-code hashes + portal/agent/staff tokens |
| `MAGMA_VOUCHER_PEPPER` | additional pepper mixed into voucher hashes |
| `MAGMA_INTERNAL_SECRET` | `X-Magma-Internal-Secret` for job + device endpoints |
| `MAGMA_SANDBOX_PAYMENTS` | `true` shorts out Hubtel/Paystack and activates plans immediately |

Losing `MAGMA_SECRETS_KEY` makes existing encrypted values unreadable — back it
up with the database.

Per-tenant payment credentials live in `business_accounts.configuration_data`:

```json
{
  "HubtelPaymentApi": {"ApiIdOrUsername": "...", "ApiKeyOrPassword": "...", "PosSalesAccountId": "..."},
  "HubtelSmsApi":     {"ApiIdOrUsername": "...", "ApiKeyOrPassword": "...", "SenderId": "MagmaNet"},
  "PaystackApi":      {"ApiKey": "sk_live_..."}
}
```

## 3. FreeRADIUS

* `freeradius/mods-available-sql.conf` → `/etc/freeradius/3.0/mods-available/sql`
* `freeradius/clients.conf.snippet` → append to `clients.conf`
* `freeradius/default-site-snippet.conf` → merge into `sites-available/default`

```bash
ln -s ../mods-available/sql /etc/freeradius/3.0/mods-enabled/sql
systemctl restart freeradius
freeradius -X    # confirm rlm_sql_postgresql loads and reads radius.nas
```

Gateways are inserted into `radius.nas` by `MagmaDeviceService` when a device is
registered with `role = GATEWAY`; no manual client entry is needed.

### FreeRADIUS on a Raspberry Pi instead of the VPS

For a single-site test the RADIUS server can sit next to the access point. Keep
the **API and PostgreSQL in the cloud** (payment webhooks and the portal need a
public HTTPS origin) and run only the daemon on the Pi — do not move the database
to the SD card.

| Host | WireGuard | LAN |
| --- | --- | --- |
| VPS (API + PostgreSQL + wg0) | `10.200.1.1` | public |
| Raspberry Pi (FreeRADIUS) | `10.200.1.2` | `192.168.88.2` |
| mANTBox ax 15s (hotspot/NAS) | `10.200.1.10` | `192.168.88.1` |

All peers are `/32` in the same `10.200.1.0/24` subnet (`wg0-server.conf` ships
`10.200.0.1/24` — change it to `10.200.1.1/24`), with `PersistentKeepalive = 25`
on both site peers.

**On the Pi**, in `mods-available/sql` set `server = "10.200.1.1"` (the rest of
the shipped file is unchanged), give FreeRADIUS its own role with
`SELECT/INSERT/UPDATE/DELETE` on `radius` and `USAGE` on `radius` sequences only —
it has no business reading `magma_*` — and allow it in `pg_hba.conf` as
`10.200.1.2/32 scram-sha-256`. Then:

```bash
radtest mg:<subscription uuid> <radius secret> 127.0.0.1 0 <shared secret>
```

**On the gateway**, point RADIUS at the Pi over the LAN (low latency) and pin the
source address so accounting carries the tunnel IP the API uses for CoA:

```
/radius add address=192.168.88.2 src-address=10.200.1.10 secret="<device secret>" \
    service=hotspot,login authentication-port=1812 accounting-port=1813
/radius incoming set accept=yes port=3799
```

RouterOS only processes CoA/disconnect from an address configured in `/radius`,
and the API sends them from the VPS, so add one more entry — same secret, source
address `10.200.1.1`, `service=login` (inert unless `/user aaa` uses RADIUS):

```
/radius add address=10.200.1.1 secret="<same device secret>" service=login
```


## 4. Gateways and APs

1. Import `wireguard/gateway-wg.rsc` on the gateway (unique `10.200.1.x`).
2. Import `mikrotik/gateway-bootstrap.rsc` (hotspot, RADIUS client, CoA, firewall).
3. Bridge each sector AP per `mikrotik/ap-bridge.rsc`.
4. Register the gateway through the API and push config:

```bash
curl -X POST "$API/api/v1/magma/admin/devices" \
  -H "ClientId: $CLIENT_ID" -H "ClientSecret: $CLIENT_SECRET" \
  -H 'Content-Type: application/json' \
  -d '{"name":"Kaneshie GW","role":"GATEWAY","zoneId":"<zone uuid>",
       "serialNumber":"HG60ABCD1234","tunnelIp":"10.200.1.10",
       "restPort":80,"apiUsername":"magma-api","apiPassword":"..."}'
# response contains radiusSecret once — paste it into the gateway
curl -X POST "$API/api/v1/magma/admin/devices/<device id>/config" \
  -H "ClientId: $CLIENT_ID" -H "ClientSecret: $CLIENT_SECRET"
```

**PoE warning:** mANTBox ax 15s takes **passive 18–28 V** PoE. A standard
802.3af/at injector (48 V) destroys the unit. mANT 15s is a passive antenna and
needs a separate radio (BaseBox/NetBox/NetMetal).

### mANTBox ax 15s as the gateway (no separate router)

The mANTBox ax 15s (L22UGS-5HaxD2HaxD-15S) is a complete RouterOS device — dual-core
IPQ-5010 at 800 MHz, 256 MB RAM, licence **level 4**, one GbE port with 18–28 V
passive PoE-in, one 2.5G SFP cage — so it can be the zone gateway itself. Three
numbers decide the design:

| Limit | Value | Consequence |
| --- | --- | --- |
| Licence | **L4 = 200 concurrent hotspot users** | 400 registered users at 25 % concurrency is 100 — fits, but the ceiling is real. An L5 key (software ID from `/system license print`) raises it to 500. |
| CPU with per-user queues | ~100–250 Mbps of mixed traffic | Keep the fibre plan ≤100 Mbps per gateway, or shape with fewer queues |
| Radio | ~60–80 associated clients per band | 400 users need 3–5 sectors, not one |

There is **no PoE-out**, so each AP is powered by its own 24 V injector (included) or
a 24 V passive switch. Never 802.3af/at 48 V.

**Scale-out layout for 400 users — one zone per sector AP:**

```
            Telecel ONT ── SFP (2.5G)
   mANTBox #0  WAN router + management only (hotspot disabled)
        │ ether1 (VLAN trunk)
    switch ──── mANTBox #1  hotspot + NAT, zone KANESHIE-N   wg 10.200.1.11
        ├────── mANTBox #2  hotspot + NAT, zone KANESHIE-S   wg 10.200.1.12
        └────── mANTBox #3  hotspot + NAT, zone KANESHIE-E   wg 10.200.1.13
                              │  RADIUS
                        Raspberry Pi (FreeRADIUS)
```

Each sector AP is registered as its **own device** with `role = GATEWAY`, its own
`tunnelIp`, its own zone and therefore its own `radius.nas` secret. MagmaNet keys
CoA on `host(radacct.nasipaddress) = host(magma_devices.tunnel_ip)`, so multiple
NASes work without code changes; `loginDevice` resolves the gateway per zone.

Customers who move between sectors re-authenticate against the new hotspot
(different NAT domain). MAC-cookie (`login-by=http-chap,mac-cookie`) makes that
invisible to them.

**Single Ethernet port, WAN and LAN on one cable.** With the ONT on copper you need
VLAN separation on `ether1`:

```rsc
/interface bridge add name=br-wan
/interface bridge add name=br-lan vlan-filtering=yes
/interface vlan add name=vlan-wan interface=br-lan vlan-id=100
/ip dhcp-client add interface=vlan-wan disabled=no
/interface bridge port add bridge=br-lan interface=ether1 pvid=10
/interface bridge vlan add bridge=br-lan vlan-ids=100 tagged=ether1
```

Put the ONT on an access port of VLAN 100 in the switch; everything else stays on
VLAN 10. If the ONT offers fibre, plug it straight into the SFP cage and keep
`ether1` as a plain LAN trunk.

## 5. Scheduled jobs

Register the Quartz callbacks once after deployment:

```bash
curl -X POST "$API/api/v1/magma/internal/jobs/register-schedules" \
  -H "X-Magma-Internal-Secret: $MAGMA_INTERNAL_SECRET"
```

| Callback | Suggested schedule | Work |
| --- | --- | --- |
| `jobs/ten-minute` | every 10 min | accounting sync, expiry/depletion, fair-use throttle, outbox drain |
| `jobs/hourly` | hourly | device health sweep, pending-payment reconciliation |
| `jobs/daily` | 02:30 | invoice run, overdue suspension |

If no scheduler is available, put the same calls in cron:

```cron
*/10 * * * * curl -fsS -X POST -H "X-Magma-Internal-Secret: $S" https://api.magmanet.gh/api/v1/magma/internal/jobs/ten-minute
0 * * * *    curl -fsS -X POST -H "X-Magma-Internal-Secret: $S" https://api.magmanet.gh/api/v1/magma/internal/jobs/hourly
30 2 * * *   curl -fsS -X POST -H "X-Magma-Internal-Secret: $S" https://api.magmanet.gh/api/v1/magma/internal/jobs/daily
```

## 6. Captive portal

`portal/index.html` is a dependency-free captive-portal SPA (plan selection, MoMo
prompt, polling, voucher redemption, agent login). Serve it from the API's
`document_root` (`./portal/`), the VPS reverse proxy, or a CDN. It calls the
public `/api/v1/magma/portal/*` endpoints with relative URLs, so it works on the
same origin as the API and needs no CORS setup.

Point the hotspot `login.html` at `/portal/index.html?zone=KANESHIE&mac=$(mac)&ip=$(ip)`;
RouterOS substitutes `$(mac)`/`$(ip)` for the client.

## 7. Endpoint map

Public (captive portal, no API key):

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/api/v1/magma/portal/bootstrap?zone=CODE` | branding, zone status, plans |
| GET | `/api/v1/magma/portal/plans?zone=CODE` | plan catalogue |
| POST | `/api/v1/magma/portal/checkout` | start MoMo/card checkout |
| GET | `/api/v1/magma/portal/checkout/{id}` | poll payment/provisioning |
| POST | `/api/v1/magma/portal/checkout/{id}/login` | server-side hotspot login |
| POST | `/api/v1/magma/portal/vouchers/redeem` | redeem voucher |
| POST | `/api/v1/magma/portal/tickets` | report a problem |
| POST | `/api/v1/magma/portal/agents/login` | agent app login |
| POST | `/api/v1/magma/portal/agents/pin/reset/request` | agent OTP |
| POST | `/api/v1/magma/portal/agents/pin/reset` | complete reset |

Webhooks (called by providers/devices):

| Method | Path |
| --- | --- |
| POST | `/api/v1/magma/webhooks/hubtel/checkout` |
| POST | `/api/v1/magma/webhooks/paystack/checkout` |
| POST | `/api/v1/magma/webhooks/hubtel/settlement` |
| POST | `/api/v1/magma/webhooks/device/hello` |
| POST | `/api/v1/magma/webhooks/device/{id}/heartbeat` |

Admin (`ClientId` / `ClientSecret` headers, same keys as the rest of the API):
`/api/v1/magma/admin/…` — zones, plans, subscribers, subscriptions, money,
devices, vouchers, agents, alerts, monitoring, reports, tickets, work orders,
staff, audit, outbox.

Internal: `/api/v1/magma/internal/jobs/{ten-minute,hourly,daily,register-schedules}`
with `X-Magma-Internal-Secret`.

## 8. Money and tax

Prices are tax-inclusive by default. At checkout the API splits each amount into
net / VAT 15% / NHIL 2.5% / GETFund 2.5% and stores `tax_breakdown` on
`magma_transactions`. Reports group by day/plan/zone/method/agent and export
CSV.

Provider callbacks are **verification-first**: the API re-queries Hubtel or
Paystack before provisioning, then activates exactly once. Sandbox mode
(`MAGMA_SANDBOX_PAYMENTS=true`) skips the provider round-trip.

## 9. Operations

* Alerts surface in `/admin/alerts`; device offline/online transitions raise
  `DEVICE_OFFLINE` / `DEVICE_ONLINE`.
* `/admin/monitoring/health` returns device counts, open alerts, outbox backlog,
  DB latency and Redis state.
* Outbox events (`RADIUS_PROVISION`, `SUBSCRIPTION_ENDED`, `SMS_SEND`,
  `SMS_RAW`) retry with exponential backoff; dead events can be retried from
  `/admin/outbox/{id}/retry`.
* `magma_prune_old_rows()` and `magma_close_stale_sessions()` should run nightly
  (they are created by `0004_partitioning.sql`; with a plain `magma_sessions`
  table use the cron `DELETE` from section 1 instead).

## 10. Compliance checklist

* NCA **Internet Hotspot** authorisation (unlicensed spectrum, max three
  administrative districts, five years) or an ISP/Regional ISP/Internet Reseller
  class above that footprint.
* Data Protection Commission registration under Act 843; capture consent
  (`magma_subscribers.consent_at/consent_version`) and store Ghana Card numbers
  encrypted only (`ghana_card_ciphertext`), with `ghana_card_masked` for display.
* Keep the `magma_audit_log` table — it records every admin action.
