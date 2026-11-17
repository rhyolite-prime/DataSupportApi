# MagmaNet Postman collection

`MagmaNet.postman_collection.json` covers all 114 MagmaNet routes (15 folders) with example
bodies, query parameters and descriptions. `MagmaNet.local.postman_environment.json` holds the
local defaults.

## Import

Postman → **Import** → drop both files → select the **MagmaNet - local** environment.

## Variables

| Variable | Purpose |
| --- | --- |
| `baseUrl` | API base, default `http://localhost:5702` |
| `clientId` / `clientSecret` | Business API key (bcrypt-hashed in `business_api_keys`) |
| `internalSecret` | `MAGMA_INTERNAL_SECRET`, used by `/internal/jobs/*` |
| `businessId` | Tenant UUID, for reference (the server derives it from the API key) |
| `zoneCode` | Zone used by the public portal and webhooks |
| `zoneId`, `planId`, `subscriberId`, `subscriptionId`, `transactionId`, `intentId`, `batchId`, `agentId`, `alertId`, `ticketId`, `workOrderId`, `deviceId`, `staffId` | Captured automatically by the create requests |
| `mac`, `voucherCode`, `deviceToken` | Values you fill in when testing a hotspot client |

## Authentication

* `/admin/*` and `/internal/*` requests send `ClientId` + `ClientSecret`. They resolve to a
  `business_id`, which scopes every query — there is no tenant parameter anywhere in the paths.
  Create a key with the existing business API-key service (bcrypt); do not insert a plaintext
  secret into `business_api_keys`.
* `/internal/jobs/*` instead send `X-Magma-Internal-Secret` and also accept `GET`, because some
  schedulers can only issue GET requests.
* `/portal/*` and `/webhooks/*` are public. Webhooks re-verify with the provider before anything is
  provisioned, so they are safe to leave unauthenticated.

Every response has the shape `{ "success": bool, "message": string, "result": object|array,
"error": object, "targetUrl": string }`.

## Sandbox walkthrough

With `MAGMA_SANDBOX_PAYMENTS=true` (the default) no provider is contacted, so the full purchase
flow works locally:

1. **Portal → Bootstrap** — confirm the zone resolves and plans are returned.
2. **Portal → Create checkout** — captures `intentId`; status comes back `PROVISIONED`.
3. **Portal → Checkout status** — verify `result.ready_to_login`.
4. **Portal → Hotspot login** — returns 409 until the MAC is visible in the router's host table,
   which is expected on a laptop that is not actually on the hotspot.
5. **Redeem a voucher** instead by setting `voucherCode` from a batch created in
   **Operations → vouchers**.

## Notes

* Routes were checked against the `ADD_METHOD_TO` table in `controllers/Magma*Controller.h`, so
  every request in the collection maps to a real handler.
* `Operations` and `Admin` share the `/api/v1/magma/admin/*` prefix; the folders only group by
  intent.
* The first time you run **Register Quartz schedules**, the ten-minute/hourly/daily jobs start
  firing at the API's own base URL — make sure that host is reachable from the scheduler.
