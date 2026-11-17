# MagmaNet — Development, Testing & Operations Roadmap

| | |
| --- | --- |
| **Version** | 1.0 |
| **Date** | 2 November 2026 |
| **Scope** | DataSupportApi / MagmaNet control plane, FreeRADIUS, MikroTik access layer, cloud hosting, first 400 subscribers |
| **Companion documents** | [`implementation_plan_MagmaNet Service.md`](implementation_plan_MagmaNet%20Service.md) (design), [`deploy/magma/README.md`](deploy/magma/README.md) (runbook), [`postman/README.md`](postman/README.md) (API surface), [`seeds/magma_demo_data.sql`](seeds/magma_demo_data.sql) (test data) |
| **Owner** | Platform lead (single accountable person) |

This document is the plan of record from "code exists" to "400 paying subscribers, maintained". Weeks are relative (`W1`, `W2`, …) with W1 assumed to be the week of 9 November 2026; shift the whole calendar if the fibre install or hardware delivery slips.

---

## 1. Where the system is today

### 1.1 Built and pushed

| Area | State | Evidence |
| --- | --- | --- |
| Control-plane API | Complete: 114 endpoints across admin, portal, webhook and internal controllers | `postman/MagmaNet.postman_collection.json` |
| Domain services | 12 services + facade (`MagmaNetService`) | `services/magma_net/` |
| Database | 23 Magma tables + FreeRADIUS `radius` schema | `migrations/0001`–`0004` |
| AAA integration | RADIUS projection, accounting sync, expiry/depletion enforcement, CoA disconnect + rate change | `MagmaRadiusService.cc` |
| Payments | Hubtel/Paystack adapters, verification-first callbacks, sandbox mode | `MagmaPayService.cc`, `MagmaBillingService.cc` |
| Vouchers & agents | Batches, HMAC code hashes, redemption, agent ledger/settlement | `MagmaVoucherService.cc`, `MagmaAgentService.cc` |
| Captive portal | Dependency-free SPA | `portal/index.html` |
| Demo dataset | 25-table seeded fixture, verified idempotent against PostgreSQL 16 | `seeds/magma_demo_data.sql` |
| Deployment assets | FreeRADIUS, RouterOS, WireGuard configs + runbook | `deploy/magma/` |

### 1.2 Verified

- Schema applies cleanly to an empty PostgreSQL 16 database; demo seed runs twice with identical row counts, and a second tenant seeds alongside the first without unique-constraint collisions.
- Voucher hash and AES helpers match `utils/CryptoUtils.cc` / `utils/HmacUtils.cc` algorithms (pgcrypto round-trip is asserted in-script).
- RouterOS property names and CoA source-address pinning corrected (commits `879bbd5`, `9a7478d`).

### 1.3 Not yet verified — these are the real risks

| Gap | Why it matters | Closed in |
| --- | --- | --- |
| No hardware run | Nothing has authenticated against a real mANTBox | Phase 2.6 |
| CI builds but never runs tests, migrations or the seed | A regression can merge silently | Phase 1 / E1 |
| Payments only in sandbox | Live money path never exercised | Phase 1 / E3 |
| SMS only in sandbox | No receipts/OTPs delivered | Phase 1 / E4 |
| No admin UI | Staff need API/Postman access for every task | Phase 1 / E7 |
| No agent-facing app | Agents cannot sell vouchers | Phase 1 / E8 |
| Key rotation unhandled | Losing or rotating `MAGMA_SECRETS_KEY` is manual and undocumented | Phase 1 / E5 |
| Single VPS, no replica | VPS loss = full outage until restore | Phase 3.5 |
| No metrics/alerting | Failures surface only when customers complain | Phase 1 / E2 |
| Never load-tested at 400 users | Concurrency and queue limits unknown in practice | Phase 2.4 |

---

## 2. Target architecture (one page)

```
                  Telecel business fibre (≥100 Mbps, ideally 200)
                                 │
                        Telecel ONT ──► mANTBox #0 (WAN router, hotspot off)   wg 10.200.1.10
                                 │
                    switch ──────┼──► mANTBox #1  hotspot+NAT  zone KANESHIE-N   wg 10.200.1.11
                                 ├──► mANTBox #2  hotspot+NAT  zone KANESHIE-S   wg 10.200.1.12
                                 ├──► mANTBox #3  hotspot+NAT  zone KANESHIE-E   wg 10.200.1.13
                                 │
                        Raspberry Pi ×2 (FreeRADIUS, primary + standby)  wg 10.200.1.2 / .3
                                 │  SQL over WireGuard
        Cloud VPS ── DataSupportApi (:5702, behind Caddy TLS) + PostgreSQL 16 + Redis + wg hub 10.200.1.1
                                 │  HTTPS
        Customers (portal) · Agents (app) · Payment providers (webhooks) · Staff (admin)
```

Design rules that the roadmap protects:

1. **API and database live in the cloud** (payment webhooks and the portal need a public TLS origin).
2. **The RADIUS daemon may live on site** (Pi) but never the database — measured against the RTT rule in §7.4.
3. **One zone per sector AP**, each registered as its own device with its own tunnel IP and RADIUS secret. This is what keeps the 200-concurrent hotspot licence cap and the 800 MHz CPU per device instead of per site.
4. **24 V passive PoE only** on mANTBox. 802.3af/at 48 V destroys the unit.

---

## 3. Phase overview

| Phase | Weeks | Outcome | Exit gate |
| --- | --- | --- | --- |
| **0. Baseline** | W1 | Repeatable environments, CI that actually tests, secrets in one place | Fresh clone → green CI → seeded DB in <30 min |
| **1. Development** | W1–W8 | P0 epics complete (CI, observability, live payments/SMS, key rotation, RADIUS resilience, thin admin UI) | All P0 acceptance criteria met; no Sev-1 defect open |
| **2. Testing** | W2–W10 | Hardware, load, chaos, security and UAT evidence for 400 users | Test report signed off; thresholds in §6.4 met |
| **3. Deployment** | W6–W12 | Staging → pilot site → 400 users in 3 waves | Go-live checklist (§13.3) complete per site |
| **4. Administration** | W8 → | SOPs, roles, training, daily routines | Ops staff can run a day without the platform lead |
| **5. Management** | W10 → | KPIs, cadence, compliance, incident & change management | First monthly business review held |
| **6. Maintenance** | W12 → | Scheduled upkeep, patching, DR drills, spares | Monthly restore drill passes unattended |

---

## 4. Phase 0 — Baseline (W1)

| # | Task | Deliverable | Done when |
| --- | --- | --- | --- |
| 0.1 | Tag the current tree | `v0.9.0-magmanet` | Tag pushed; `CMakeLists.txt` version matches |
| 0.2 | CI on the working branch | `.github/workflows/` extended | Builds on PRs to `main` and pushes to `dev`/feature branches, runs `ctest`, applies `0001`–`0003`, runs the seed twice, fails on diff |
| 0.3 | Reproducible local stack | `docker-compose.yml` (postgres:16, freeradius, redis) + `.env.example` | `docker compose up` then `psql … -f seeds/magma_demo_data.sql` yields the summary table |
| 0.4 | Secrets inventory | Appendix 13.2 filled, values in one password manager | Two people can find every secret; none in git |
| 0.5 | Backup baseline | Nightly `pg_dump` to object storage + 30-day retention | One restore into a scratch DB verified |
| 0.6 | Issue tracking | Epics from §5 as tickets with estimates | Board reflects W1–W12 plan |

**Environment matrix**

| Environment | API | Database | RADIUS | Hardware | Payments |
| --- | --- | --- | --- | --- | --- |
| Local | dev build | docker postgres 16 | optional container | none | sandbox |
| Staging | VPS, TLS | staging DB | Pi #2 | one AP on the bench | sandbox |
| Production | VPS, TLS | prod DB (PITR) | Pi #1 + Pi #2 standby | 4 sectors | live |

---

## 5. Phase 1 — Development

Estimates are developer-days, assuming one full-time developer plus review.

### P0 — required before the pilot takes money

| Epic | Outcome | Acceptance criteria | Est. |
| --- | --- | --- | --- |
| **E1 CI that tests** | Regressions caught before merge | Build + `ctest` + migrations + double seed + `newman run postman/…` against a seeded DB, all in one job; red build blocks merge | 3 |
| **E2 Observability** | Failures visible before customers notice | Structured JSON logs with request id; `/healthz` covering DB, Redis, RADIUS socket; metrics endpoint (auth success/fail, CoA sent/acked/failed, outbox backlog, pending payments, sessions live, p95 latency); alert rules → SMS/WhatsApp to on-call | 5 |
| **E3 Live payments** | Real money in, reconciled | Provider credentials in `business_configuration_data`; webhook signature/secret validation; verification-first unchanged; duplicate-callback test; refund and payout SOPs rehearsed with a GHS 1 live transaction; daily reconciliation report | 5 |
| **E4 Live SMS** | Receipts and OTPs delivered | `HubtelSmsApi` configured per tenant; templated messages; send-failure does not block provisioning (outbox retry); spend guard (max sends/day) | 3 |
| **E5 Key rotation** | `MAGMA_SECRETS_KEY` / `MAGMA_HMAC_KEY` rotatable without data loss | CLI/endpoint that decrypts with old key and re-encrypts with new (device secrets, RADIUS secrets, API passwords, Ghana Card, voucher ciphertext); voucher rehash procedure documented (drain-then-rotate, because plaintext codes are never stored); rehearsal on staging | 4 |
| **E6 RADIUS resilience** | One RADIUS host failing does not stop logins | Second Pi (or second FreeRADIUS instance) with the same DB, gateway `/radius` entries ordered primary/secondary, `read_clients=yes` from `radius.nas`, `radclient` burst test proving failover < 5 s | 3 |
| **E7 Thin admin UI** | Staff work without Postman | Login (ClientId/Secret or staff PIN), dashboard (overview endpoint), subscriber lookup, suspend/resume, voucher batch issue + assign, ticket queue, alert ack, zone open/close. Reuse `/api/v1/magma/admin/*` unchanged | 10 |

**P0 total ≈ 33 dev-days** (≈ 7 weeks with review and QA).

### P1 — required for a real crew to run it

| Epic | Outcome | Acceptance criteria | Est. |
| --- | --- | --- | --- |
| **E8 Agent app** | Vouchers sold in the field | Phone+PIN login, float balance, sell voucher (offline-tolerant queue), statement, settlement request; works on a GHS 300 Android phone over 3G | 10 |
| **E9 PPPoE / CPE onboarding** | Fixed-wireless product line | CPE device records, `pppoe_username` provisioning into `radcheck`, per-CPE rate profile, install work order → activation | 8 |
| **E10 Reports & exports** | Finance and ops self-serve | Sales/revenue/tax/agent/voucher reports export CSV; zone P&L from `magma_zone_costs`; monthly tax pack | 5 |
| **E11 Notifications** | Fewer "is it down?" calls | Outage/restoration notices to affected subscribers, expiry warnings 24 h ahead, agent low-float alert | 4 |
| **E12 Subscriber self-service** | Less counter load | Check balance/expiry and top up by MSISDN without re-entering everything | 5 |

### P2 — growth and polish

| Epic | Outcome | Est. |
| --- | --- | --- |
| E13 Multi-site dashboard + capacity forecasting | Plan sectors before they saturate | 8 |
| E14 Multi-device/family plans, device-limit UI | Upsell, fewer "why did my second phone fail" tickets | 4 |
| E15 Promotions/coupons, referral credit | Demand generation | 5 |
| E16 Accounting/WhatsApp integrations | Less manual bookkeeping | 8 |

**Definition of Done (all epics):** code reviewed; unit/integration tests updated; Postman collection updated; migration added if schema changes (never edit an applied migration); `deploy/magma/README.md` updated if operations change; changelog entry; deployed to staging and smoke-tested.

---

## 6. Phase 2 — Testing

### 6.1 Levels and ownership

| Level | What | Tooling | Owner | When |
| --- | --- | --- | --- | --- |
| Unit | Voucher normalisation, HMAC, token expiry, tax split | `ctest` (`test/magma_net_test.cc`) | Dev | Every commit |
| Integration | Service ↔ PostgreSQL, migrations, seed idempotency, tenant isolation | CI job + `psql` | Dev | Every PR |
| Contract/API | All 114 requests, auth failures, rate limits | `newman run postman/MagmaNet.postman_collection.json` | Dev | Every PR + pre-release |
| AAA | Auth accept/reject, accounting, CoA disconnect, CoA rate change | `radclient`, `freeradius -X` | Platform | Weekly + pre-release |
| Load | 100 concurrent sessions, 10 auths/s, portal traffic | `radclient -c 400 -p 40`, `k6` | Platform | Before each wave |
| Soak | 24 h continuous traffic, memory/queue drift | same + `vmstat`, RouterOS `/system resource` | Platform | Before wave 3 |
| Chaos | Kill RADIUS, kill DB, cut WireGuard, duplicate/replay webhooks, expire voucher mid-redemption | manual drills | Platform | Before wave 2 |
| Field/RF | Coverage, throughput, roaming, interference | `iperf3`, RouterOS registration table, site survey app | Field | Each site |
| Security | Cross-tenant access attempts, secret-at-rest inspection, TLS config, rate-limit bypass | manual + `nmap`/`testssl.sh` | Platform | Pre-pilot + quarterly |
| UAT | Real agents and customers complete the journeys | scripted scenarios | Ops | Before each wave |
| DR | Restore DB, rebuild API, re-provision one AP from `deploy/magma` | runbook | Platform | Monthly |

### 6.2 Test matrix — critical journeys

| Journey | Steps under test | Must pass |
| --- | --- | --- |
| MoMo purchase → online | `POST /portal/checkout` → webhook → `completeCheckout` → `projectSubscription` → `POST /checkout/{id}/login` | Subscription ACTIVE, `radcheck` row, client online within 10 s |
| Voucher redemption | `POST /portal/vouchers/redeem` with a seeded code | Voucher REDEEMED, subscription ACTIVE, agent ledger EARNING row |
| Accounting sync | traffic → 60 s interim → `syncUsage` | `magma_sessions` rows, `used_mb` increases, `radacct.nasipaddress` = tunnel IP |
| Expiry / depletion | time or volume exhausted → `enforceLimits` | Status EXPIRED/DEPLETED, `radcheck` deleted, CoA ACK, client disconnected |
| Fair-use throttle | unlimited plan crosses cap | `radreply` rate changed, CoA ACK, session survives |
| Suspension | admin suspend | Next login rejected; live session disconnected |
| Refund | admin refund | Transaction REVERSED, subscription terminated, alert/report consistent |
| Agent settlement | batch assign → sell → settle | Ledger balances, `balance_after` chain unbroken |
| Tenant isolation | tenant A's ClientId with tenant B's UUIDs | 404/empty everywhere, no data leak, audit rows present |
| Webhook replay | same callback 5× | Exactly one provisioning, idempotent responses |

### 6.3 Environments for testing

- **Bench**: 1 mANTBox + Pi + staging API, one laptop as client. Proves the whole chain before anything goes up a mast.
- **Pilot site**: 1 sector, ≤30 invited users, live payments with small amounts.
- **Staging**: same software version as production, sandbox payments, isolated database.

### 6.4 Acceptance thresholds (gate for each rollout wave)

| Metric | Threshold |
| --- | --- |
| Portal API p95 (cloud) | < 300 ms |
| Checkout → PROVISIONED | < 5 s (sandbox), < 30 s (live provider) |
| RADIUS auth p95 | < 500 ms with DB in cloud; < 150 ms with RADIUS in cloud |
| RADIUS auth success rate (valid credentials) | > 99.5 % |
| CoA delivery (ACK) | > 99 % |
| Accounting lag (`radacct` → `magma_sessions`) | < 2 min p95 |
| Cross-tenant access attempts that succeed | 0 |
| Concurrent sessions sustained | 100 without error, CPU < 70 % per AP |
| DB restore | RTO < 4 h, RPO < 15 min (after WAL archiving) |

---

## 7. Phase 3 — Deployment

### 7.1 Hosting layout

| Component | Host | Sizing | Notes |
| --- | --- | --- | --- |
| DataSupportApi | VPS (Ubuntu 22.04/24.04) | 2–4 vCPU, 4–8 GB RAM, 40 GB SSD | Build needs ≥4 GB; run needs ~1 GB |
| PostgreSQL 16 | same VPS | 2 vCPU, 4 GB, 40 GB + WAL | `listen_addresses = 'localhost,10.200.1.1'` |
| Redis | same VPS | 256 MB | Rate limits; API fails open without it |
| Caddy/nginx | same VPS | — | TLS termination, 443 only |
| WireGuard hub | same VPS | — | UDP 51820; the only other open port |
| FreeRADIUS | Raspberry Pi #1 (+ #2 standby) | Pi 4/5, 2 GB, USB SSD | No database on the Pi |
| Access layer | mANTBox ax 15s ×4 | 24 V injectors, passive switch | 24 V only |

**Open ports:** 443/tcp (public), 51820/udp (WireGuard). Everything else closed — no 5432, no 1812/1813, no RouterOS services from the WAN.

### 7.2 Release procedure

```
1. Tag candidate            git tag v1.x.y && push
2. CI green                 build, tests, migrations, newman
3. Staging deploy           migrations → binary → restart → smoke
4. Smoke set                /healthz · /portal/bootstrap · one sandbox purchase
                            · one radtest · one CoA disconnect
5. Change window            low-traffic (before 08:00), announce to agents
6. Production deploy        pg_dump first → migrations → binary → restart
7. Live smoke               one real voucher redemption + one live GHS 1 purchase
8. Watch 30 min             error rate, auth failures, outbox backlog
```

**Rollback:** keep the previous binary (`/opt/datasupport/releases/<tag>`, symlink `current`) — rollback is a symlink switch plus service restart. Migrations are **forward-only**; a rollback that requires schema reversal is done by restoring the pre-deploy dump, which is why step 6 dumps first.

### 7.3 Site build checklist (per zone/sector)

Physical: mast 8–12 m, sector aimed at the target area, downtilt set; grounded mast; surge arrestor per Ethernet run; injector/UPS in a weatherproof box; ONT on VLAN 100.

Logical: VLAN 10 management / VLAN 20 guest; AP channel and 20/40 MHz width per plan (§ site survey); power 17–20 dBm; WireGuard peer up (`persistent-keepalive=25s`); `/radius` primary = Pi with `src-address` = tunnel IP, secondary `/radius` = hub for CoA; `/radius incoming accept=yes port=3799`; walled garden for the portal host; `www` enabled only for the REST user restricted to `10.200.1.0/24`; client isolation on; guest→RFC1918 blocked.

MagmaNet: zone created and LIVE, plans attached via `magma_plan_zones`, device registered (`role=GATEWAY`, `tunnelIp`, REST creds), `POST /admin/devices/{id}/config` returns all steps `ok`, `radius.nas` row present, `SELECT host(nasipaddress)` proves the tunnel IP.

### 7.4 RADIUS placement decision rule

Measure `ping` from the Pi to the VPS:

| RTT | Decision |
| --- | --- |
| < 40 ms | RADIUS on the Pi (auth ≈ 5 × RTT) |
| 40–80 ms | Pi acceptable; set `/radius timeout=3s` and `pool { start=5 max=16 }` |
| > 80 ms | Move FreeRADIUS onto the VPS next to PostgreSQL; keep the Pi as standby |

### 7.5 Rollout waves

| Wave | Scope | Users | Exit criteria |
| --- | --- | --- | --- |
| **W6–W7 Pilot** | 1 sector, staff + 30 invited customers, live money | 30 | §6.4 thresholds; zero Sev-1 for 7 days; support load < 5 tickets/day |
| **W8–W9 Wave 2** | 2 sectors, agents selling vouchers | 100 | Voucher sell-through > 60 %; agent float settles daily; concurrency 25 at peak with CPU < 70 % |
| **W10–W12 Wave 3** | 4 sectors, full 400 | 400 | Load test evidence at 100 concurrent; peak utilization < 70 % of line; churn and ARPU baselines set |

### 7.6 Disaster recovery

| Scenario | Response | Target |
| --- | --- | --- |
| API/DB VPS lost | Rebuild from provisioned image, restore latest dump + WAL, repoint DNS | RTO 4 h, RPO 15 min |
| Pi (RADIUS) fails | Standby Pi takes over (gateway secondary `/radius` entry) | < 5 min |
| Fibre outage | Existing sessions survive until interim timeout; portal unreachable → status page + SMS | Comms within 15 min |
| AP failure | Swap from spares (1 spare AP per 4 deployed) | < 4 h |
| Payment provider outage | Checkout degrades; vouchers keep selling | No revenue stop |

---

## 8. Phase 4 — Administration

### 8.1 Roles and access

| Role | MagmaNet staff role | Can do | Cannot do |
| --- | --- | --- | --- |
| Platform lead | ADMIN | Everything, including secrets and migrations | — |
| Zone supervisor | OPS | Open/close zones, plans, alerts, reports for their zones | Secrets, refunds, staff creation |
| Support agent | SUPPORT | Lookup subscriber, create ticket, suspend on instruction | Refunds, plan pricing |
| Field technician | TECHNICIAN | Work orders, device registration, diagnostics | Money operations |
| Finance | FINANCE | Refunds, settlements, invoices, tax reports | Zone/network changes |
| Agent manager | AGENT_MANAGER | Create agents, assign batches, settle float | Refunds |

Rules: named accounts only (no shared logins), PIN/secret rotation every 90 days, leavers disabled same day, every money action carries an audit row (`magma_audit_log`).

### 8.2 Standard operating procedures (to be written as one-page runbooks)

| SOP | Trigger | Steps (summary) |
| --- | --- | --- |
| Onboard a zone | New site | Create zone → attach plans → register gateway + sectors → push config → publish |
| Open/close selling | Demand, outage, event | Set zone status LIVE/PAUSED; confirm portal banner |
| Issue voucher batch | Stock request | Create batch → download → assign to agent → record physical handover |
| Agent settlement | Weekly / on request | Verify ledger → payout → mark settlement → confirm balance |
| Refund | Failed service or complaint | Verify transaction → refund → terminate/extend subscription → notify |
| Suspend/churn | Non-payment, abuse | Suspend (CoA disconnect) → notify → churn after retention window |
| Incident comms | Sev 1/2 | Status page + SMS to affected zone + ticket bundle |
| Device replacement | Hardware fault | Decommission → register replacement with same zone → push config → verify |

### 8.3 Daily / weekly admin routine

| Cadence | Task | Where |
| --- | --- | --- |
| Daily 08:00 | Alerts triage, outbox DEAD count, failed payments, offline devices | `/admin/alerts`, `/admin/outbox`, `/admin/devices` |
| Daily | Reconcile yesterday's collections vs provider statement | `/admin/reconcile`, provider portal |
| Daily | Answer tickets, update work orders | `/admin/tickets` |
| Weekly | Agent float settlement, voucher stock count, revenue snapshot | `/admin/agents`, reports |
| Weekly | Review auth failures and CoA NACKs | logs, metrics |
| Monthly | P&L per zone, tax pack, churn review | `/admin/zones/{id}/pnl`, `/admin/reports/tax` |

### 8.4 Training and documentation

- 1-day ops training (portal, admin UI, RADIUS basics, what CoA is and why a "ghost session" appears).
- Agent onboarding pack: how to sell, what to do when a code is refused, how float works.
- Customer-facing FAQ + WhatsApp script.
- Field guide: mast safety, 24 V PoE labels, channel/power table, cable and connector discipline.

---

## 9. Phase 5 — Management (governance)

### 9.1 KPIs and targets

| KPI | Definition | Target (steady state) |
| --- | --- | --- |
| Network availability | Minutes zone LIVE and passing traffic / total | ≥ 99.5 % |
| Auth success rate | Access-Accept / (Accept + Reject) excluding wrong passwords | ≥ 99.5 % valid |
| Peak concurrency | Max live sessions per day | Track; alarm > 120 per AP |
| Backhaul utilization | Peak Mbps / contracted Mbps | < 70 % |
| ARPU | Revenue / active subscribers / month | Set after wave 2 |
| Collection rate | Successful transactions / attempted | ≥ 95 % |
| Churn | Subscribers lost / starting base / month | < 8 % |
| Voucher sell-through | Redeemed / issued within 30 days | > 80 % |
| Agent float days | Cash held by agents / daily sales | < 3 days |
| Ticket rate | Tickets per 100 active users per week | < 5 |
| MTTR | Mean time to restore (Sev 1/2) | < 4 h |
| Outbox DEAD events | Count | 0 (alarm on any) |
| CoA NACK rate | NACK / sent | < 1 % |

### 9.2 Operating cadence

- **Daily (15 min):** yesterday's revenue, incidents, alerts, collections mismatch.
- **Weekly (45 min):** KPI review, support themes, agent stock/float, capacity trend.
- **Monthly (2 h):** business review — P&L per zone, ARPU/churn, marketing, roadmap reprioritization, compliance check.
- **Quarterly:** strategy, pricing, capacity expansion, security review, DR test, licence/regulatory review.

### 9.3 Incident management

| Severity | Definition | Response | Comms |
| --- | --- | --- | --- |
| Sev 1 | Whole site down, or payments failing | 15 min, all hands | Status page + SMS within 30 min, hourly updates |
| Sev 2 | One sector degraded, CoA failing | 1 h | Zone SMS |
| Sev 3 | Single customer / billing issue | Next business day | Ticket |
| Sev 4 | Cosmetic / request | Backlog | — |

Every Sev 1/2 gets a blameless postmortem within 5 working days: timeline, cause, detection gap, action items with owners.

### 9.4 Change management

- No direct changes on production without a PR (schema, config, RouterOS script).
- RouterOS changes staged on the bench AP, exported with `/export`, committed under `deploy/magma/mikrotik/site-<code>.rsc`.
- Change window: weekdays 06:00–08:00 or announced maintenance; never Friday evening or during a market's peak.
- Every change has a rollback line in the ticket.

### 9.5 Compliance and regulatory

| Obligation | Detail | Owner |
| --- | --- | --- |
| NCA Internet Hotspot authorisation | Unlicensed spectrum, max 3 administrative districts, 5-year term; keep the certificate on file | Platform lead |
| Data protection (Act 843) | Subscriber data minimisation, retention policy, breach notification, no Ghana Card numbers in logs (they are encrypted at rest) | Platform lead |
| Tax | VAT 15 % + NHIL 2.5 % + GETFund 2.5 % split stored per transaction; monthly returns from `/admin/reports/tax` | Finance |
| Consumer protection | Published prices, clear fair-use policy, receipt for every payment | Ops |
| Payment provider terms | Refund window, dispute handling, settlement account reconciliation | Finance |

---

## 10. Phase 6 — Maintenance

### 10.1 Schedule

| Cadence | Task | Command / location |
| --- | --- | --- |
| Daily | Verify backup completed and is non-zero | object storage; `pg_restore --list` spot check weekly |
| Daily | Check alerts, outbox DEAD, payment mismatches | admin UI |
| Daily | Confirm `syncUsage` ran (ten-minute job) | `/admin/monitoring/health` |
| Weekly | `VACUUM (ANALYZE)` on hot tables; review bloat | `psql` |
| Weekly | Prune stale sessions; close orphaned ones | `SELECT magma_close_stale_sessions();` (0004) or the documented `DELETE` |
| Weekly | Certificate expiry, WireGuard handshakes, disk usage | Caddy logs, `wg show`, `df -h` |
| Weekly | RouterOS firmware check; read changelog before upgrading | `/system package update check-for-updates` |
| Monthly | **Restore drill**: restore last night's dump into scratch DB and run a query | runbook |
| Monthly | Load test at expected peak (off-peak) | `radclient`, `k6` |
| Monthly | Access review: staff accounts, agents, API keys | admin UI |
| Monthly | Secret age check; rotate anything > 90 days | Appendix 13.2 |
| Quarterly | OS patching window (VPS + Pis), RouterOS upgrade one AP at a time | runbook |
| Quarterly | Security review: TLS scan, dependency CVEs, tenant isolation retest | `testssl.sh`, CI |
| Quarterly | Capacity and RF review: per-AP concurrency, channel plan, interference | RouterOS + site visit |
| Annually | Licence renewal, hardware refresh, price review, DR full test | — |

### 10.2 Database maintenance

- Retention: `radius.radacct` and `magma_sessions` 13 months; `magma_audit_log` 24 months; `magma_outbox` DONE rows 30 days.
- Apply `0004_partitioning.sql` once sessions exceed ~1 million rows; otherwise the plain table plus weekly prune is fine for 400 users.
- WAL archiving enabled after wave 2 (turns RPO from 24 h into 15 min).
- Never run `VACUUM FULL` during business hours; use `pg_repack` if bloat matters.

### 10.3 Secret rotation runbook (after E5)

| Secret | Rotation | Blast radius if lost |
| --- | --- | --- |
| `MAGMA_SECRETS_KEY` | Re-encrypt all `*_ciphertext` columns with new key in one transaction; keep old key until verified | Device/RADIUS secrets and Ghana Card numbers unreadable; sessions keep running, new logins of existing subs break |
| `MAGMA_HMAC_KEY` | Drain voucher stock, rotate, rehash redeemed-only records; staff/agent tokens reissued | All unused voucher hashes invalid |
| `MAGMA_VOUCHER_PEPPER` | Same as HMAC key | Same |
| `MAGMA_INTERNAL_SECRET` | Rotate freely with a restart | Job callbacks fail until updated |
| Device RADIUS secrets | Per device: rotate in MagmaNet, push config, confirm auth | That device's logins fail |
| WireGuard keys | Per peer, one at a time | That peer's control traffic stops |
| Staff/agent PINs | Self-service reset via OTP | Account lockout |

### 10.4 Hardware maintenance

- Spares per 4 APs: 1 AP, 1 injector, 1 switch, 1 PSU, 2 patch cables.
- Quarterly: inspect mounts, cable glands, surge arrestors; clean dust; verify PoE voltage with a tester (18–28 V).
- Keep a configuration export of every device in git (`/export hide-sensitive`), named `site-<code>-<date>.rsc`.
- RMA process documented with distributor contact and serial numbers in `magma_devices`.

---

## 11. Risk register

| # | Risk | Likelihood | Impact | Mitigation | Owner |
| --- | --- | --- | --- | --- | --- |
| R1 | Licence level 4 caps hotspot at 200 concurrent users | Medium | High | One zone per AP; buy L5 keys for any AP expected > 150 concurrent | Platform |
| R2 | Single VPS = single point of failure | High | High | WAL archiving + documented 4 h rebuild; move to managed Postgres by wave 3 | Platform |
| R3 | `MAGMA_SECRETS_KEY` lost | Low | Critical | Password manager + sealed offline copy + E5 rotation tooling | Platform |
| R4 | 48 V PoE into an 18–28 V mANTBox | Medium | High | Label every injector; field checklist; only 24 V spares stocked | Field |
| R5 | Pi SD-card failure kills RADIUS | Medium | High | USB SSD, standby Pi, secondary `/radius` entry | Platform |
| R6 | Payment provider dispute or settlement delay | Medium | Medium | Verification-first design, daily reconciliation, dispute SOP | Finance |
| R7 | Interference / channel congestion kills throughput | High | Medium | Channel plan, 5 GHz only for capacity, quarterly RF review, DFS avoided | Field |
| R8 | Agent cash fraud or float drift | Medium | Medium | Ledger with `balance_after` chain, weekly settlement, surprise stock counts | Ops |
| R9 | Theft/vandalism of masts and APs | Medium | Medium | Mount height, tamper-proof enclosures, serial tracking, insurance | Ops |
| R10 | Key-person dependency (one developer) | High | High | This document, runbooks, PR review, second person trained on deploy | Platform |
| R11 | Regulatory scope creep (districts, licence class) | Low | High | Track NCA classes; stay within 3 districts until licensed | Platform |

---

## 12. Milestones

| Milestone | Week | Success looks like |
| --- | --- | --- |
| M0 Baseline green | W1 | CI tests, restores a dump, seeds a database |
| M1 Bench end-to-end | W4 | One AP authenticates a real phone against a real subscription, disconnect CoA proven |
| M2 Money live | W6 | Real GHS payment provisions a plan; receipt SMS delivers |
| M3 Pilot site | W7 | 30 users, 7 days, zero Sev-1 |
| M4 Agents selling | W9 | 100 users, vouchers sell, float settles |
| M5 400 users | W12 | 4 sectors, load test passed, KPIs baselined |
| M6 Steady state | W16 | Monthly review held, restore drill unattended, runbooks used by someone other than the author |

---

## 13. Appendices

### 13.1 Repository map for this roadmap

| Document | Use |
| --- | --- |
| `implementation_plan_MagmaNet Service.md` | Design rationale, schema, service responsibilities |
| `deploy/magma/README.md` | Install runbook: DB, secrets, FreeRADIUS, Pi topology, gateways, jobs, endpoints, tax |
| `deploy/magma/freeradius/`, `mikrotik/`, `wireguard/` | Config templates to copy per site |
| `seeds/magma_demo_data.sql` | Fixture for staging, UAT and training |
| `postman/` | Contract tests + manual API access before E7 exists |
| `migrations/` | Ordered, forward-only schema |

### 13.2 Secret inventory (types only — values live in the password manager)

| Secret | Used by | Where set |
| --- | --- | --- |
| `MAGMA_SECRETS_KEY` (≥32 chars) | API (AES columns) | env |
| `MAGMA_HMAC_KEY` | API (voucher hashes, tokens) | env |
| `MAGMA_VOUCHER_PEPPER` | API (voucher hashes) | env |
| `MAGMA_INTERNAL_SECRET` | API (job endpoints) | env |
| `MAGMA_RADIUS_SECRET` | API → FreeRADIUS (per device when pushed) | env / `magma_devices` |
| DB role passwords (app, radius) | API, FreeRADIUS | env / `pg_hba` |
| Provider keys (Hubtel, Paystack) | API | `business_configuration_data` (encrypted fields) |
| WireGuard keys | VPS, Pis, APs | config files, mode 0600 |
| RouterOS REST credentials | API → AP | `magma_devices` (encrypted) |
| Staff/agent PINs | People | bcrypt in DB |

### 13.3 Go-live checklist (per site)

- [ ] Site survey done: signal ≥ −65 dBm at 80 % of target footprint, iperf3 ≥ 20 Mbps at cell edge
- [ ] Channels set, non-adjacent, power 17–20 dBm, 2.4 GHz low or off
- [ ] 24 V PoE verified at every AP; grounding and surge arrestors fitted
- [ ] UPS tested under load; ONT labelled
- [ ] WireGuard up from every peer; `ping` both directions
- [ ] `/radius` primary + secondary + `incoming accept=yes`; `src-address` = tunnel IP
- [ ] Walled garden contains the portal host
- [ ] Zone LIVE, plans attached, prices verified
- [ ] Device registered, config push all `ok`, `radius.nas` row present
- [ ] One real purchase → online; one voucher → online; one expiry → CoA disconnect
- [ ] `SELECT host(nasipaddress)` shows tunnel IPs only
- [ ] Alerts, status page and support contacts published to the community
- [ ] Config exported and committed; serial numbers recorded

### 13.4 Open questions

1. Walk-in hotspot (phones, vouchers) or fixed-wireless CPEs (20 km claim) for the first 400 — changes AP count, plan design and CPE logistics.
2. Telecel contracted rate, static IP availability, and whether the ONT exposes SFP.
3. Payment provider: Hubtel merchant account status and settlement cycle.
4. Admin UI: build in-repo (Vue/React) or buy time with Retool during the pilot.
5. Whether a second VPS (or managed PostgreSQL) is funded before wave 3.
