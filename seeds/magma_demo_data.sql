-- ===========================================================================
-- MagmaNet — demo / test data
-- ===========================================================================
-- Loads a realistic tenant: 3 zones, 6 devices, 12 subscribers with
-- subscriptions, sessions, payments, invoices, 70 vouchers, 3 agents,
-- staff, tickets, work orders, alerts, audit rows, outbox events and
-- zone costs.
--
--   psql "$DATABASE_URL" \
--        -v business_id="<business uuid>" \
--        -v secrets_key="$MAGMA_SECRETS_KEY" \
--        -v hmac_key="$MAGMA_HMAC_KEY" \
--        -v pepper="$MAGMA_VOUCHER_PEPPER" \
--        -f seeds/magma_demo_data.sql
--
-- Every argument is optional. Without them the first business_accounts row
-- is used and the development keys below are assumed. The keys must match the
-- ones the API is running with, otherwise seeded voucher codes cannot be
-- redeemed and seeded device/subscriber secrets cannot be decrypted:
--
--   secrets_key  MAGMA_SECRETS_KEY   (>= 32 chars, AES-256 key material)
--   hmac_key     MAGMA_HMAC_KEY
--   pepper       MAGMA_VOUCHER_PEPPER (defaults to hmac_key in the app)
--
-- The script is idempotent: every UUID is derived from a fixed label via
-- magma_demo_uuid(), so re-running it skips rows that already exist.
-- To start clean, drop and recreate the database (or truncate the magma_*
-- tables); this file never deletes anything.
--
-- Requires the pgcrypto extension (PostgreSQL contrib) for the same
-- AES-256-CBC / HMAC-SHA256 primitives the API uses, plus bcrypt PIN hashes.
-- ===========================================================================

\set ON_ERROR_STOP on

\if :{?business_id}
\else
  \set business_id ''
\endif
\if :{?secrets_key}
\else
  \set secrets_key 'magmanet-demo-secrets-key-32-chars!'
\endif
\if :{?hmac_key}
\else
  \set hmac_key 'magmanet-demo-hmac-key'
\endif
\if :{?pepper}
\else
  \set pepper 'magmanet-demo-voucher-pepper'
\endif

BEGIN;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- Stable pseudo-UUID for a label, so re-running the seed is a no-op.
-- The tenant is folded in through the magma.demo_scope setting, so the same
-- script can be loaded for several tenants without UUID collisions.
CREATE OR REPLACE FUNCTION magma_demo_uuid(p_key TEXT)
RETURNS UUID
LANGUAGE sql
STABLE
AS $fn$
    SELECT (substr(m, 1, 8) || '-' || substr(m, 9, 4) || '-4' || substr(m, 14, 3) ||
            '-8' || substr(m, 18, 3) || '-' || substr(m, 21, 12))::uuid
    FROM (SELECT md5('magma-demo:' ||
                     coalesce(current_setting('magma.demo_scope', true), '') || ':' ||
                     p_key) AS m) AS h;
$fn$;

-- Same envelope as utils/CryptoUtils::encrypt(): AES-256-CBC, random IV
-- prepended, standard Base64 with no line breaks, key = first 32 bytes.
CREATE OR REPLACE FUNCTION magma_demo_encrypt(p_plain TEXT, p_key TEXT)
RETURNS TEXT
LANGUAGE sql
AS $fn$
    SELECT replace(encode(encrypt(convert_to(p_plain, 'UTF8'),
                                  convert_to(left(p_key, 32), 'UTF8'),
                                  'aes-cbc/pad:pkcs'), 'base64'), E'\n', '');
$fn$;

-- Same as MagmaVoucherService: HMAC-SHA256 over the normalised code, keyed
-- with hmacKey || '|' || voucherPepper, lower-case hex.
CREATE OR REPLACE FUNCTION magma_demo_voucher_hash(p_code TEXT, p_hmac TEXT, p_pepper TEXT)
RETURNS TEXT
LANGUAGE sql
AS $fn$
    SELECT encode(hmac(convert_to(p_code, 'UTF8'),
                       convert_to(p_hmac || '|' || p_pepper, 'UTF8'),
                       'sha256'), 'hex');
$fn$;

COMMIT;

-- ===========================================================================
-- Loader
-- ===========================================================================
CREATE OR REPLACE FUNCTION magma_demo_load(
    p_business UUID,
    p_secrets  TEXT,
    p_hmac     TEXT,
    p_pepper   TEXT)
RETURNS TABLE (kind TEXT, detail TEXT)
LANGUAGE plpgsql
AS $fn$
DECLARE
    -- zones / plans / devices
    v_zone1 UUID; v_zone2 UUID; v_zone3 UUID;
    v_plan_hour UUID; v_plan_day UUID; v_plan_day3 UUID; v_plan_week UUID; v_plan_month UUID;
    v_gw1 UUID; v_ap1 UUID; v_ap2 UUID; v_gw2 UUID; v_ap3 UUID; v_gw3 UUID;
    v_gw1_secret TEXT := 'demo-radius-kaneshie-1';
    v_gw2_secret TEXT := 'demo-radius-mamprobi-1';
    v_gw3_secret TEXT := 'demo-radius-teshie-1';
    -- per-iteration values
    v_i INT;
    v_j INT;
    v_n INT;
    v_m INT;
    v_id UUID;
    v_sub_id UUID;
    v_batch_id UUID;
    v_agent1 UUID;
    v_agent2 UUID;
    v_agent3 UUID;
    v_ticket UUID;
    v_plan UUID;
    v_plan_code TEXT;
    v_plan_name TEXT;
    v_plan_type TEXT;
    v_duration INT;
    v_cap BIGINT;
    v_down INT;
    v_up INT;
    v_price NUMERIC(12,2);
    v_zone UUID;
    v_sub_status TEXT;
    v_msisdn TEXT;
    v_code TEXT;
    v_tag TEXT;
    v_vat NUMERIC(12,2);
    v_net NUMERIC(12,2);
    v_tax NUMERIC(12,2);
    -- literals
    v_names TEXT[] := ARRAY['Ama Mensah', 'Kofi Boateng', 'Akosua Owusu', 'Yaw Owusu',
                            'Abena Asante', 'Kwame Nkrumah', 'Efua Mensah', 'Kojo Antwi',
                            'Adjoa Safo', 'Nana Yaa', 'Selorm Agbeko', 'Ibrahim Fuseini'];
    v_msisdns TEXT[] := ARRAY['0244123456', '0201234567', '0551234567', '0271234567',
                              '0244555666', '0243111222', '0599111222', '0209998887',
                              '0544332211', '0267778889', '0248889991', '0207776665'];
    v_types TEXT[] := ARRAY['WALK_IN', 'WALK_IN', 'RESIDENTIAL', 'RESIDENTIAL', 'WALK_IN',
                            'WALK_IN', 'BUSINESS', 'WALK_IN', 'WALK_IN', 'RESIDENTIAL',
                            'WALK_IN', 'RESIDENTIAL'];
    v_subs TEXT[] := ARRAY['ACTIVE', 'ACTIVE', 'ACTIVE', 'ACTIVE', 'SUSPENDED', 'CHURNED',
                           'ACTIVE', 'ACTIVE', 'ACTIVE', 'ACTIVE', 'ACTIVE', 'SUSPENDED'];
    v_plans TEXT[] := ARRAY['HOUR-1', 'DAY-1', 'DAY-3', 'WEEK-1', 'MONTH-1'];
BEGIN
    IF p_business IS NULL THEN
        RAISE EXCEPTION 'No tenant: pass -v business_id=<uuid> (no business_accounts row found)';
    END IF;
    IF NOT EXISTS (SELECT 1 FROM business_accounts WHERE id = p_business) THEN
        RAISE EXCEPTION 'business_accounts row % not found', p_business;
    END IF;
    IF length(p_secrets) < 32 THEN
        RAISE EXCEPTION 'secrets key must be at least 32 characters, got %', length(p_secrets);
    END IF;

    -- Fail loudly rather than seed ciphertext the API cannot read.
    IF convert_from(
           decrypt(decode(magma_demo_encrypt('probe', p_secrets), 'base64'),
                   convert_to(left(p_secrets, 32), 'UTF8'), 'aes-cbc/pad:pkcs'),
           'UTF8') <> 'probe' THEN
        RAISE EXCEPTION 'pgcrypto AES round-trip failed; is the pgcrypto extension usable?';
    END IF;

    -- Scope the deterministic UUIDs to this tenant and derive a short numeric
    -- tag for the globally-unique codes below (device serials, voucher codes,
    -- agent codes, transaction references).
    PERFORM set_config('magma.demo_scope', p_business::text, true);
    v_tag := lpad(((('x' || substr(md5(p_business::text), 1, 8))::bit(32)::bigint)
                   % 100000)::text, 5, '0');

    -- ------------------------------------------------------------------
    -- Catalogue and zones (0003 helpers). Plans are the tenant catalogue,
    -- not demo-only data, so they are left in place on re-runs.
    -- ------------------------------------------------------------------
    PERFORM magma_seed_default_plans(p_business);
    PERFORM magma_seed_provider_config(p_business);

    v_zone1 := magma_seed_default_zone(p_business, 'KANESHIE', 'Kaneshie Market',
                                       'Okaikoi South', 'Greater Accra');
    v_zone2 := magma_seed_default_zone(p_business, 'MAMPROBI', 'Mamprobi Junction',
                                       'Ablekuma South', 'Greater Accra');
    v_zone3 := magma_seed_default_zone(p_business, 'TESHIE', 'Teshie Nungua',
                                       'Ledzokuku', 'Greater Accra');

    UPDATE magma_zones
       SET status = 'LIVE', backhaul_provider = 'MTN_FIBRE',
           upstream_down_mbps = 100, upstream_up_mbps = 50,
           upstream_monthly_cost = 1500, upstream_is_cgnat = TRUE,
           tunnel_ip = '10.200.1.10', gps_lat = 5.573600, gps_lng = -0.234500,
           notes = 'Gateway on the market manager''s wall'
     WHERE id = v_zone1;

    UPDATE magma_zones
       SET status = 'LIVE', backhaul_provider = 'TELECEL_FIBRE',
           upstream_down_mbps = 60, upstream_up_mbps = 30,
           upstream_monthly_cost = 900, upstream_is_cgnat = TRUE,
           tunnel_ip = '10.200.1.20', gps_lat = 5.556700, gps_lng = -0.241200,
           notes = 'Backhaul shared with the cold store'
     WHERE id = v_zone2;

    UPDATE magma_zones
       SET status = 'BUILD', backhaul_provider = 'MTN_FIBRE',
           upstream_down_mbps = 50, upstream_up_mbps = 25,
           upstream_monthly_cost = 800, upstream_is_cgnat = TRUE,
           tunnel_ip = '10.200.1.30', gps_lat = 5.576300, gps_lng = -0.103400,
           notes = 'Sector install in progress'
     WHERE id = v_zone3;

    SELECT id INTO v_plan_hour  FROM magma_data_plans WHERE business_id = p_business AND plan_code = 'HOUR-1';
    SELECT id INTO v_plan_day   FROM magma_data_plans WHERE business_id = p_business AND plan_code = 'DAY-1';
    SELECT id INTO v_plan_day3  FROM magma_data_plans WHERE business_id = p_business AND plan_code = 'DAY-3';
    SELECT id INTO v_plan_week  FROM magma_data_plans WHERE business_id = p_business AND plan_code = 'WEEK-1';
    SELECT id INTO v_plan_month FROM magma_data_plans WHERE business_id = p_business AND plan_code = 'MONTH-1';

    -- ------------------------------------------------------------------
    -- Devices (one gateway per zone + sector APs)
    -- ------------------------------------------------------------------
    v_gw1 := magma_demo_uuid('device:gw-kaneshie');
    v_ap1 := magma_demo_uuid('device:ap-kaneshie-north');
    v_ap2 := magma_demo_uuid('device:ap-kaneshie-south');
    v_gw2 := magma_demo_uuid('device:gw-mamprobi');
    v_ap3 := magma_demo_uuid('device:ap-mamprobi');
    v_gw3 := magma_demo_uuid('device:gw-teshie');

    INSERT INTO magma_devices (
        id, business_id, zone_id, name, role, vendor, model, serial_number, mac_address,
        routeros_version, management_ip, tunnel_ip, rest_scheme, rest_port, api_username,
        api_secret_ciphertext, radius_secret_ciphertext, enrolment_status, status,
        last_seen_at, uptime_seconds, cpu_load, free_memory_mb, installed_at, notes)
    VALUES
        (v_gw1, p_business, v_zone1, 'Kaneshie GW1', 'GATEWAY', 'MikroTik', 'RB5009UG+S+IN',
         'DEMO-' || v_tag || '-GW-001', ('48:8F:5A:' || substr(v_tag, 1, 2) || ':00:01')::macaddr, '7.16.2', '192.168.88.1', '10.200.1.10', 'http', 80,
         'magma-api', magma_demo_encrypt('demo-api-pass-1', p_secrets),
         magma_demo_encrypt(v_gw1_secret, p_secrets), 'ENROLLED', 'ONLINE',
         now() - interval '20 seconds', 864000, 12, 780, now() - interval '90 days',
         'Hotspot + DHCP + queues'),
        (v_ap1, p_business, v_zone1, 'Kaneshie North AP', 'AP', 'MikroTik', 'mANTBox ax 15s',
         'DEMO-' || v_tag || '-AP-001', ('48:8F:5A:' || substr(v_tag, 1, 2) || ':00:02')::macaddr, '7.16.2', '192.168.89.11', '10.200.1.11', 'http', 80,
         'magma-api', magma_demo_encrypt('demo-api-pass-2', p_secrets),
         magma_demo_encrypt('demo-radius-ap-1', p_secrets), 'ENROLLED', 'ONLINE',
         now() - interval '45 seconds', 432000, 8, 190, now() - interval '85 days',
         'North sector, 120 degrees'),
        (v_ap2, p_business, v_zone1, 'Kaneshie South AP', 'AP', 'MikroTik', 'mANTBox ax 15s',
         'DEMO-' || v_tag || '-AP-002', ('48:8F:5A:' || substr(v_tag, 1, 2) || ':00:03')::macaddr, '7.16.2', '192.168.89.12', '10.200.1.12', 'http', 80,
         'magma-api', magma_demo_encrypt('demo-api-pass-3', p_secrets),
         magma_demo_encrypt('demo-radius-ap-2', p_secrets), 'ENROLLED', 'ONLINE',
         now() - interval '40 seconds', 400000, 7, 205, now() - interval '85 days',
         'South sector, 120 degrees'),
        (v_gw2, p_business, v_zone2, 'Mamprobi GW1', 'GATEWAY', 'MikroTik', 'RB5009UG+S+IN',
         'DEMO-' || v_tag || '-GW-002', ('48:8F:5A:' || substr(v_tag, 1, 2) || ':00:04')::macaddr, '7.16.2', '192.168.88.1', '10.200.1.20', 'http', 80,
         'magma-api', magma_demo_encrypt('demo-api-pass-4', p_secrets),
         magma_demo_encrypt(v_gw2_secret, p_secrets), 'ENROLLED', 'ONLINE',
         now() - interval '25 seconds', 300000, 10, 640, now() - interval '60 days',
         'Hotspot only, no PPPoE'),
        (v_ap3, p_business, v_zone2, 'Mamprobi AP', 'AP', 'MikroTik', 'mANTBox ax 15s',
         'DEMO-' || v_tag || '-AP-003', ('48:8F:5A:' || substr(v_tag, 1, 2) || ':00:05')::macaddr, '7.16.2', '192.168.89.21', '10.200.1.21', 'http', 80,
         'magma-api', magma_demo_encrypt('demo-api-pass-5', p_secrets),
         magma_demo_encrypt('demo-radius-ap-3', p_secrets), 'ENROLLED', 'OFFLINE',
         now() - interval '3 hours', 120000, 0, 0, now() - interval '60 days',
         'PoE injector suspect'),
        (v_gw3, p_business, v_zone3, 'Teshie GW1', 'GATEWAY', 'MikroTik', 'RB5009UG+S+IN',
         'DEMO-' || v_tag || '-GW-003', ('48:8F:5A:' || substr(v_tag, 1, 2) || ':00:06')::macaddr, '7.16.1', '192.168.88.1', '10.200.1.30', 'http', 80,
         'magma-api', magma_demo_encrypt('demo-api-pass-6', p_secrets),
         magma_demo_encrypt(v_gw3_secret, p_secrets), 'PENDING', 'UNKNOWN', NULL, NULL, NULL,
         NULL, NULL, 'Waiting for the site visit')
    ON CONFLICT (id) DO NOTHING;

    -- RADIUS clients for the gateways (FreeRADIUS reads this table).
    IF NOT EXISTS (SELECT 1 FROM radius.nas WHERE nasname = '10.200.1.10') THEN
        INSERT INTO radius.nas (nasname, shortname, type, ports, secret, description)
        VALUES ('10.200.1.10', 'DEMO-KANESHIE-GW', 'other', 3799, v_gw1_secret,
                'MagmaNet demo gateway (Kaneshie)');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM radius.nas WHERE nasname = '10.200.1.20') THEN
        INSERT INTO radius.nas (nasname, shortname, type, ports, secret, description)
        VALUES ('10.200.1.20', 'DEMO-MAMPROBI-GW', 'other', 3799, v_gw2_secret,
                'MagmaNet demo gateway (Mamprobi)');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM radius.nas WHERE nasname = '10.200.1.30') THEN
        INSERT INTO radius.nas (nasname, shortname, type, ports, secret, description)
        VALUES ('10.200.1.30', 'DEMO-TESHIE-GW', 'other', 3799, v_gw3_secret,
                'MagmaNet demo gateway (Teshie)');
    END IF;

    -- ------------------------------------------------------------------
    -- Subscribers, devices, subscriptions
    -- ------------------------------------------------------------------
    FOR v_i IN 1..12 LOOP
        v_id := magma_demo_uuid('subscriber:' || v_i);
        v_zone := CASE WHEN v_i >= 10 THEN v_zone2 ELSE v_zone1 END;

        INSERT INTO magma_subscribers (
            id, business_id, msisdn, full_name, email, ghana_card_ciphertext,
            ghana_card_masked, subscriber_type, auth_type, pppoe_username,
            pppoe_secret_ciphertext, home_zone_id, service_address, gps_lat, gps_lng,
            status, kyc_status, consent_at, consent_version, first_seen_at, last_seen_at,
            created_at, updated_at)
        VALUES (
            v_id, p_business, v_msisdns[v_i],
            v_names[v_i],
            lower(replace(v_names[v_i], ' ', '.')) || '@example.com',
            CASE WHEN v_i <= 4
                 THEN magma_demo_encrypt('GHA-' || lpad(v_i::text, 9, '0') || '-0', p_secrets)
                 ELSE NULL END,
            CASE WHEN v_i <= 4 THEN 'GHA-*****' || lpad(v_i::text, 2, '0') || '-0' ELSE NULL END,
            v_types[v_i],
            CASE WHEN v_types[v_i] = 'RESIDENTIAL' THEN 'PPPOE' ELSE 'HOTSPOT' END,
            CASE WHEN v_types[v_i] = 'RESIDENTIAL' THEN 'mg-pppoe-' || lpad(v_i::text, 2, '0') END,
            CASE WHEN v_types[v_i] = 'RESIDENTIAL'
                 THEN magma_demo_encrypt('pppoe-pass-' || v_i, p_secrets) END,
            v_zone,
            CASE WHEN v_i >= 10 THEN 'Mamprobi, House ' || v_i ELSE 'Kaneshie Market, Stall ' || (40 + v_i) END,
            CASE WHEN v_i >= 10 THEN 5.556700 ELSE 5.573600 END,
            CASE WHEN v_i >= 10 THEN -0.241200 ELSE -0.234500 END,
            v_subs[v_i],
            CASE WHEN v_i <= 4 THEN 'VERIFIED' ELSE 'NOT_REQUIRED' END,
            CASE WHEN v_i <= 4 THEN now() - interval '80 days' END,
            CASE WHEN v_i <= 4 THEN 'v1' END,
            now() - (v_i || ' days')::interval,
            now() - interval '1 hour',
            now() - (v_i || ' days')::interval,
            now())
        ON CONFLICT DO NOTHING;

        -- one client device per subscriber (a second for the business account)
        INSERT INTO magma_client_devices (
            id, business_id, subscriber_id, mac_address, label, first_seen_at,
            last_seen_at, is_blocked, blocked_reason)
        VALUES (
            magma_demo_uuid('client:' || v_i || ':1'), p_business, v_id,
            ('DC:A6:32:' || substr(v_tag, 1, 2) || ':00:' || lpad(to_hex(v_i), 2, '0'))::macaddr,
            'Phone', now() - (v_i || ' days')::interval, now() - interval '2 hours',
            v_i = 6, CASE WHEN v_i = 6 THEN 'CHURNED' END)
        ON CONFLICT DO NOTHING;

        IF v_types[v_i] = 'BUSINESS' THEN
            INSERT INTO magma_client_devices (
                id, business_id, subscriber_id, mac_address, label, first_seen_at, last_seen_at)
            VALUES (
                magma_demo_uuid('client:' || v_i || ':2'), p_business, v_id,
                ('DC:A6:32:' || substr(v_tag, 1, 2) || ':01:' || lpad(to_hex(v_i), 2, '0'))::macaddr,
                'Shop laptop', now() - interval '30 days', now() - interval '1 day')
            ON CONFLICT DO NOTHING;
        END IF;

        -- ------------------------------------------------ subscription --
        SELECT p.id, p.plan_code, p.name, p.plan_type, COALESCE(p.duration_minutes, 0),
               COALESCE(p.data_cap_mb, 0), p.down_kbps, p.up_kbps, p.price
          INTO v_plan, v_plan_code, v_plan_name, v_plan_type, v_duration, v_cap, v_down, v_up, v_price
          FROM magma_data_plans p
         WHERE p.business_id = p_business AND p.plan_code = v_plans[(v_i % 5) + 1];

        v_sub_id := magma_demo_uuid('subscription:' || v_i);
        v_sub_status := CASE
            WHEN v_i = 5 THEN 'SUSPENDED'
            WHEN v_i = 6 THEN 'CANCELLED'
            WHEN v_i = 11 THEN 'EXPIRED'
            ELSE 'ACTIVE' END;

        INSERT INTO magma_subscriptions (
            id, business_id, subscriber_id, plan_id, zone_id, radius_username,
            radius_secret_ct, status, purchased_minutes, purchased_mb, used_seconds, used_mb,
            starts_at, expires_at, activated_at, ended_at, plan_snapshot, auto_renew,
            throttled_at, created_at, updated_at)
        VALUES (
            v_sub_id, p_business, v_id, v_plan, v_zone,
            'mg:' || v_sub_id::text,
            magma_demo_encrypt('demo-radius-' || v_i, p_secrets),
            v_sub_status,
            NULLIF(v_duration, 0),
            NULLIF(v_cap, 0),
            (v_i * 600)::bigint,
            LEAST(v_i * 120, GREATEST(COALESCE(v_cap, 5000), 120))::bigint,
            now() - (v_i || ' days')::interval,
            CASE v_sub_status
                WHEN 'ACTIVE'    THEN now() + ((15 - v_i) || ' days')::interval
                WHEN 'SUSPENDED' THEN now() + interval '3 days'
                ELSE now() - interval '2 days' END,
            now() - (v_i || ' days')::interval,
            CASE WHEN v_sub_status IN ('EXPIRED', 'CANCELLED') THEN now() - interval '2 days' END,
            jsonb_build_object(
                'plan_id', v_plan, 'plan_code', v_plan_code, 'name', v_plan_name,
                'plan_type', v_plan_type, 'duration_minutes', v_duration,
                'data_cap_mb', v_cap, 'down_kbps', v_down, 'up_kbps', v_up,
                'price', v_price, 'paid_amount', v_price, 'validity_days', 30),
            v_i IN (7, 9),
            CASE WHEN v_plan_code = 'MONTH-1' AND v_i IN (4, 9)
                 THEN now() - interval '1 day' END,
            now() - (v_i || ' days')::interval,
            now())
        ON CONFLICT DO NOTHING;

        -- RADIUS credentials and reply for the live subscriptions
        IF v_sub_status = 'ACTIVE' THEN
            DELETE FROM radius.radcheck WHERE username = 'mg:' || v_sub_id::text;
            INSERT INTO radius.radcheck (username, attribute, op, value)
            VALUES ('mg:' || v_sub_id::text, 'Cleartext-Password', ':=', 'demo-radius-' || v_i);
            INSERT INTO radius.radcheck (username, attribute, op, value)
            VALUES ('mg:' || v_sub_id::text, 'Simultaneous-Use', ':=',
                    CASE WHEN v_plan_code = 'DAY-3' THEN '2' ELSE '1' END);

            DELETE FROM radius.radreply WHERE username = 'mg:' || v_sub_id::text;
            INSERT INTO radius.radreply (username, attribute, op, value)
            VALUES ('mg:' || v_sub_id::text, 'Mikrotik-Rate-Limit', '=',
                    v_down || 'k/' || v_up || 'k');
            INSERT INTO radius.radreply (username, attribute, op, value)
            VALUES ('mg:' || v_sub_id::text, 'Session-Timeout', ':=', '86400');
            INSERT INTO radius.radreply (username, attribute, op, value)
            VALUES ('mg:' || v_sub_id::text, 'Idle-Timeout', ':=', '1800');
            INSERT INTO radius.radreply (username, attribute, op, value)
            VALUES ('mg:' || v_sub_id::text, 'Acct-Interim-Interval', ':=', '60');
        END IF;

        -- ---------------------------------------------------- sessions --
        FOR v_j IN 0..2 LOOP
            INSERT INTO magma_sessions (
                id, business_id, subscription_id, subscriber_id, zone_id, device_id,
                acct_session_id, nas_ip, framed_ip, mac_address, started_at, last_interim_at,
                ended_at, last_input_octets, last_output_octets, input_octets, output_octets,
                session_seconds, terminate_cause)
            VALUES (
                magma_demo_uuid('session:' || v_i || ':' || v_j), p_business, v_sub_id, v_id,
                v_zone, CASE WHEN v_zone = v_zone1 THEN v_gw1 ELSE v_gw2 END,
                'demo-' || v_tag || '-' || v_i || '-' || v_j,
                (CASE WHEN v_zone = v_zone1 THEN '10.200.1.10'
                      ELSE '10.200.1.20' END)::inet,
                ('10.10.' || v_i || '.' || (10 + v_j))::inet,
                ('DC:A6:32:' || substr(v_tag, 1, 2) || ':00:' || lpad(to_hex(v_i), 2, '0'))::macaddr,
                now() - ((v_j + 1) * interval '1 day') - (v_i * interval '17 minutes'),
                now() - ((v_j + 1) * interval '1 day') - (v_i * interval '17 minutes') + interval '44 minutes',
                now() - ((v_j + 1) * interval '1 day') - (v_i * interval '17 minutes') + interval '45 minutes',
                (2000000 + v_i * 100000)::bigint, (15000000 + v_j * 2000000)::bigint,
                (2000000 + v_i * 100000)::bigint, (15000000 + v_j * 2000000)::bigint,
                2700, 'User-Request')
            ON CONFLICT DO NOTHING;
        END LOOP;

        -- live sessions for a few subscribers
        IF v_i IN (1, 2, 7) THEN
            INSERT INTO magma_sessions (
                id, business_id, subscription_id, subscriber_id, zone_id, device_id,
                acct_session_id, nas_ip, framed_ip, mac_address, started_at, last_interim_at,
                input_octets, output_octets, session_seconds)
            VALUES (
                magma_demo_uuid('session-live:' || v_i), p_business, v_sub_id, v_id, v_zone, v_gw1,
                'demo-live-' || v_tag || '-' || v_i, '10.200.1.10', ('10.10.' || v_i || '.200')::inet,
                ('DC:A6:32:' || substr(v_tag, 1, 2) || ':00:' || lpad(to_hex(v_i), 2, '0'))::macaddr,
                now() - interval '35 minutes', now() - interval '30 seconds',
                (120000 + v_i * 1000)::bigint, (900000 + v_i * 5000)::bigint, 2100)
            ON CONFLICT DO NOTHING;
        END IF;
    END LOOP;

    -- ------------------------------------------------------------------
    -- Agents, wallets, voucher batches and vouchers
    -- ------------------------------------------------------------------
    v_agent1 := magma_demo_uuid('agent:1');
    v_agent2 := magma_demo_uuid('agent:2');
    v_agent3 := magma_demo_uuid('agent:3');

    INSERT INTO magma_agents (
        id, business_id, agent_code, name, msisdn, email, pin_hash, tier, commission_rate,
        territory_zone_id, wallet_account_id, status, kyc_status, created_at, updated_at)
    VALUES
        (v_agent1, p_business, 'AG-' || v_tag || '-01', 'Kofi Boateng', '0244555666', 'kofi.agent@example.com',
         crypt('1234', gen_salt('bf', 8)), 'STANDARD', 0.10, v_zone1, 'DEMO-WALLET-01',
         'ACTIVE', 'VERIFIED', now() - interval '60 days', now()),
        (v_agent2, p_business, 'AG-' || v_tag || '-02', 'Adwoa Serwaa', '0201112223', 'adwoa.agent@example.com',
         crypt('1234', gen_salt('bf', 8)), 'PREMIER', 0.12, v_zone2, 'DEMO-WALLET-02',
         'ACTIVE', 'VERIFIED', now() - interval '40 days', now()),
        (v_agent3, p_business, 'AG-' || v_tag || '-03', 'Issahaku Musah', '0271112223', 'issahaku.agent@example.com',
         crypt('1234', gen_salt('bf', 8)), 'STANDARD', 0.15, v_zone3, 'DEMO-WALLET-03',
         'SUSPENDED', 'PENDING', now() - interval '20 days', now())
    ON CONFLICT DO NOTHING;

    INSERT INTO magma_agent_ledger (
        id, business_id, agent_id, entry_type, amount, balance_after, reference, description, created_at)
    VALUES
        (magma_demo_uuid('ledger:1:1'), p_business, v_agent1, 'ADJUSTMENT', 500.00, 500.00,
         'DEMO-OPEN-01', 'Opening float', now() - interval '60 days'),
        (magma_demo_uuid('ledger:1:2'), p_business, v_agent1, 'STOCK_PURCHASE', -200.00, 300.00,
         'DEMO-STOCK-01', '20 x HOUR-1 vouchers', now() - interval '30 days'),
        (magma_demo_uuid('ledger:1:3'), p_business, v_agent1, 'EARNING', 45.00, 345.00,
         'DEMO-EARN-01', 'Commission on 15 vouchers', now() - interval '10 days'),
        (magma_demo_uuid('ledger:1:4'), p_business, v_agent1, 'PAYOUT', -100.00, 245.00,
         'DEMO-PAYOUT-01', 'MoMo settlement', now() - interval '5 days'),
        (magma_demo_uuid('ledger:2:1'), p_business, v_agent2, 'ADJUSTMENT', 200.00, 200.00,
         'DEMO-OPEN-02', 'Opening float', now() - interval '40 days'),
        (magma_demo_uuid('ledger:2:2'), p_business, v_agent2, 'EARNING', 30.00, 230.00,
         'DEMO-EARN-02', 'Commission on 10 vouchers', now() - interval '8 days')
    ON CONFLICT DO NOTHING;

    INSERT INTO magma_voucher_batches (
        id, business_id, batch_code, plan_id, zone_id, quantity, unit_price, wholesale_price,
        owner_type, owner_agent_id, status, expires_at, created_at)
    VALUES
        (magma_demo_uuid('batch:1'), p_business, 'VB-' || v_tag || '-001', v_plan_day, v_zone1, 40, 8.00, 6.00,
         'BUSINESS', NULL, 'ACTIVE', now() + interval '90 days', now() - interval '30 days'),
        (magma_demo_uuid('batch:2'), p_business, 'VB-' || v_tag || '-002', v_plan_hour, v_zone1, 20, 3.00, 2.00,
         'AGENT', v_agent1, 'ACTIVE', now() + interval '60 days', now() - interval '20 days'),
        (magma_demo_uuid('batch:3'), p_business, 'VB-' || v_tag || '-003', v_plan_week, v_zone2, 10, 40.00, 30.00,
         'BUSINESS', NULL, 'VOID', now() + interval '30 days', now() - interval '10 days')
    ON CONFLICT DO NOTHING;

    FOR v_n IN 1..70 LOOP
        IF v_n <= 40 THEN
            v_batch_id := magma_demo_uuid('batch:1');
            v_plan := v_plan_day;
            v_price := 8.00;
            v_zone := v_zone1;
        ELSIF v_n <= 60 THEN
            v_batch_id := magma_demo_uuid('batch:2');
            v_plan := v_plan_hour;
            v_price := 3.00;
            v_zone := v_zone1;
        ELSE
            v_batch_id := magma_demo_uuid('batch:3');
            v_plan := v_plan_week;
            v_price := 40.00;
            v_zone := v_zone2;
        END IF;

        v_code := 'TEST-' || v_tag || '-' || lpad(v_n::text, 3, '0');

        INSERT INTO magma_vouchers (
            id, business_id, batch_id, plan_id, zone_id, serial, code_hash, code_ciphertext,
            status, agent_id, price, commission, sold_at, redeemed_at,
            redeemed_subscriber_id, subscription_id, created_at, updated_at)
        VALUES (
            magma_demo_uuid('voucher:' || v_n), p_business, v_batch_id, v_plan, v_zone,
            'DEMO-' || v_tag || '-' || lpad(v_n::text, 6, '0'),
            magma_demo_voucher_hash(replace(v_code, '-', ''), p_hmac, p_pepper),
            magma_demo_encrypt(v_code, p_secrets),
            CASE
                WHEN v_n > 60 THEN 'VOID'
                WHEN v_n <= 3 THEN 'REDEEMED'
                WHEN v_n <= 13 THEN 'ASSIGNED'
                WHEN v_n <= 17 THEN 'SOLD'
                WHEN v_n = 18 THEN 'EXPIRED'
                ELSE 'AVAILABLE' END,
            CASE WHEN v_n BETWEEN 4 AND 13 THEN v_agent1 END,
            v_price,
            round(v_price * 0.10, 2),
            CASE WHEN v_n BETWEEN 14 AND 17 THEN now() - interval '3 days' END,
            CASE WHEN v_n <= 3 THEN now() - interval '2 days' END,
            CASE WHEN v_n <= 3 THEN magma_demo_uuid('subscriber:' || v_n) END,
            CASE WHEN v_n <= 3 THEN magma_demo_uuid('subscription:' || v_n) END,
            now() - interval '30 days', now())
        ON CONFLICT DO NOTHING;
    END LOOP;

    -- ------------------------------------------------------------------
    -- Payments, invoices and checkout intents
    -- ------------------------------------------------------------------
    FOR v_n IN 1..30 LOOP
        v_price := (ARRAY[3.00, 8.00, 8.00, 20.00, 40.00, 120.00])[(v_n % 6) + 1];
        v_net := round(v_price / 1.20, 2);
        v_tax := v_price - v_net;
        v_vat := round(v_net * 0.15, 2);
        v_sub_id := magma_demo_uuid('subscription:' || ((v_n % 12) + 1));
        v_zone := CASE WHEN (v_n % 12) + 1 >= 10 THEN v_zone2 ELSE v_zone1 END;

        INSERT INTO magma_transactions (
            id, business_id, subscriber_id, subscription_id, agent_id, voucher_id, purpose,
            amount, tax_amount, net_amount, tax_breakdown, payment_method, provider,
            provider_reference, internal_reference, status, payer_msisdn, initiated_at,
            paid_at, created_at, updated_at)
        VALUES (
            magma_demo_uuid('tx:' || v_n), p_business,
            magma_demo_uuid('subscriber:' || ((v_n % 12) + 1)), v_sub_id,
            CASE WHEN v_n % 7 = 0 THEN v_agent1 END,
            CASE WHEN v_n % 7 = 0 THEN magma_demo_uuid('voucher:' || (v_n % 40 + 1)) END,
            CASE WHEN v_n % 7 = 0 THEN 'VOUCHER' ELSE 'BUNDLE' END,
            v_price, v_tax, v_net,
            jsonb_build_object('vat', v_vat, 'nhil', round(v_net * 0.025, 2),
                               'getFund', round(v_net * 0.025, 2)),
            (ARRAY['MOMO_MTN', 'MOMO_TELECEL', 'MOMO_AIRTELTIGO', 'CARD'])[(v_n % 4) + 1],
            (ARRAY['HUBTEL', 'HUBTEL', 'PAYSTACK'])[(v_n % 3) + 1],
            'DEMO-' || v_tag || '-TX-' || lpad(v_n::text, 6, '0'),
            'DEMO-' || v_tag || '-INT-' || lpad(v_n::text, 6, '0'),
            CASE WHEN v_n % 10 = 0 THEN 'PENDING'
                 WHEN v_n % 9 = 0 THEN 'FAILED'
                 ELSE 'SUCCESS' END,
            v_msisdns[((v_n % 12) + 1)],
            now() - ((31 - v_n) || ' days')::interval,
            CASE WHEN v_n % 10 <> 0 AND v_n % 9 <> 0
                 THEN now() - ((31 - v_n) || ' days')::interval + interval '3 minutes' END,
            now() - ((31 - v_n) || ' days')::interval,
            now())
        ON CONFLICT DO NOTHING;
    END LOOP;

    FOR v_n IN 1..4 LOOP
        v_sub_id := magma_demo_uuid('subscription:' || (CASE v_n WHEN 1 THEN 3 WHEN 2 THEN 4
                                                              WHEN 3 THEN 12 ELSE 10 END));
        INSERT INTO magma_invoices (
            id, business_id, subscriber_id, subscription_id, invoice_number, period_start,
            period_end, subtotal, tax_amount, total, amount_paid, status, due_at, issued_at,
            paid_at, created_at)
        VALUES (
            magma_demo_uuid('invoice:' || v_n), p_business,
            magma_demo_uuid('subscriber:' || (CASE v_n WHEN 1 THEN 3 WHEN 2 THEN 4
                                              WHEN 3 THEN 12 ELSE 10 END)),
            v_sub_id, 'INV-' || v_tag || '-' || lpad(v_n::text, 4, '0'),
            date_trunc('month', now() - interval '1 month')::date,
            (date_trunc('month', now()) - interval '1 day')::date,
            round(120.00 / 1.20, 2), 24.00, 120.00,
            CASE WHEN v_n = 1 THEN 120.00 ELSE 0 END,
            (ARRAY['PAID', 'ISSUED', 'OVERDUE', 'DRAFT'])[v_n],
            (date_trunc('month', now()) + interval '6 days')::timestamptz,
            (date_trunc('month', now()))::timestamptz,
            CASE WHEN v_n = 1 THEN (date_trunc('month', now()) + interval '2 days')::timestamptz END,
            (date_trunc('month', now()))::timestamptz)
        ON CONFLICT DO NOTHING;
    END LOOP;

    FOR v_n IN 1..5 LOOP
        INSERT INTO magma_checkout_intents (
            id, business_id, zone_id, plan_id, mac_address, ip_address, msisdn, subscriber_id,
            transaction_id, status, login_attempts, expires_at, created_at, updated_at)
        VALUES (
            magma_demo_uuid('intent:' || v_n), p_business, v_zone1,
            CASE v_n WHEN 1 THEN v_plan_day WHEN 2 THEN v_plan_hour WHEN 3 THEN v_plan_day
                     WHEN 4 THEN v_plan_week ELSE v_plan_month END,
            ('DC:A6:32:' || substr(v_tag, 1, 2) || ':02:' || lpad(to_hex(v_n), 2, '0'))::macaddr,
            ('10.10.1.' || (100 + v_n))::inet,
            '0244000' || lpad(v_n::text, 4, '0'), NULL,
            NULL,
            (ARRAY['NEW', 'AWAITING_PAYMENT', 'PROVISIONED', 'EXPIRED', 'FAILED'])[v_n],
            CASE WHEN v_n = 4 THEN 3 ELSE 0 END,
            CASE WHEN v_n IN (4, 5) THEN now() - interval '1 day' ELSE now() + interval '30 minutes' END,
            now() - (v_n * interval '2 hours'), now())
        ON CONFLICT DO NOTHING;
    END LOOP;

    -- ------------------------------------------------------------------
    -- Staff, tickets, work orders
    -- ------------------------------------------------------------------
    INSERT INTO magma_staff (
        id, business_id, full_name, msisdn, email, pin_hash, role, zone_ids, status,
        last_login_at, created_at)
    VALUES
        (magma_demo_uuid('staff:admin'), p_business, 'Nana Admin', '0244000001',
         'admin@magmanet.gh', crypt('2468', gen_salt('bf', 8)), 'ADMIN',
         ARRAY[v_zone1, v_zone2], 'ACTIVE', now() - interval '2 hours', now() - interval '120 days'),
        (magma_demo_uuid('staff:ops'), p_business, 'Araba Ops', '0244000002',
         'ops@magmanet.gh', crypt('2468', gen_salt('bf', 8)), 'OPS',
         ARRAY[v_zone1], 'ACTIVE', now() - interval '1 day', now() - interval '100 days'),
        (magma_demo_uuid('staff:tech'), p_business, 'Yaw Technician', '0244777888',
         'yaw@magmanet.gh', crypt('2468', gen_salt('bf', 8)), 'TECHNICIAN',
         ARRAY[v_zone1, v_zone2], 'ACTIVE', now() - interval '3 days', now() - interval '80 days'),
        (magma_demo_uuid('staff:support'), p_business, 'Esi Support', '0244000004',
         'esi@magmanet.gh', crypt('2468', gen_salt('bf', 8)), 'SUPPORT',
         ARRAY[v_zone1], 'ACTIVE', now() - interval '5 hours', now() - interval '60 days'),
        (magma_demo_uuid('staff:finance'), p_business, 'Kojo Finance', '0244000005',
         'finance@magmanet.gh', crypt('2468', gen_salt('bf', 8)), 'FINANCE',
         ARRAY[v_zone1], 'ACTIVE', now() - interval '1 day', now() - interval '45 days')
    ON CONFLICT DO NOTHING;

    FOR v_n IN 1..5 LOOP
        v_ticket := magma_demo_uuid('ticket:' || v_n);
        INSERT INTO magma_tickets (
            id, business_id, zone_id, subscriber_id, agent_id, category, priority, status,
            subject, description, assigned_to, sla_due_at, resolved_at, created_at, updated_at)
        VALUES (
            v_ticket, p_business,
            CASE WHEN v_n >= 4 THEN v_zone2 ELSE v_zone1 END,
            magma_demo_uuid('subscriber:' || v_n),
            NULL,
            (ARRAY['CONNECTIVITY', 'BILLING', 'DEVICE', 'CONNECTIVITY', 'OTHER'])[v_n],
            (ARRAY['HIGH', 'NORMAL', 'URGENT', 'NORMAL', 'LOW'])[v_n],
            (ARRAY['OPEN', 'IN_PROGRESS', 'RESOLVED', 'OPEN', 'CLOSED'])[v_n],
            (ARRAY['[demo] No internet after payment', '[demo] Double charged for day pass',
                   '[demo] AP keeps dropping', '[demo] Slow speeds in the evening',
                   '[demo] Change phone number'])[v_n],
            '[demo] Seeded by seeds/magma_demo_data.sql',
            CASE WHEN v_n IN (2, 3) THEN magma_demo_uuid('staff:tech') END,
            now() - interval '2 days',
            CASE WHEN v_n IN (3, 5) THEN now() - interval '1 day' END,
            now() - (v_n || ' days')::interval,
            now() - interval '4 hours')
        ON CONFLICT DO NOTHING;

        INSERT INTO magma_ticket_messages (id, ticket_id, author_type, author_id, body, created_at)
        VALUES
            (magma_demo_uuid('ticket-msg:' || v_n || ':1'), v_ticket, 'CUSTOMER',
             magma_demo_uuid('subscriber:' || v_n),
             'Reporting the issue from the portal.', now() - (v_n || ' days')::interval),
            (magma_demo_uuid('ticket-msg:' || v_n || ':2'), v_ticket, 'STAFF',
             magma_demo_uuid('staff:support'),
             'Thanks, we are looking into it.', now() - interval '6 hours')
        ON CONFLICT DO NOTHING;
    END LOOP;

    INSERT INTO magma_work_orders (
        id, business_id, zone_id, subscriber_id, order_type, status, assigned_to,
        scheduled_at, checklist, materials, photos, cost, completed_at, created_at)
    VALUES
        (magma_demo_uuid('wo:1'), p_business, v_zone1, magma_demo_uuid('subscriber:3'),
         'INSTALL', 'SCHEDULED', magma_demo_uuid('staff:tech'),
         now() + interval '1 day',
         '["Mount AP","Run cable","Configure SSID"]', '["20m Cat6"]', '[]', 0, NULL,
         now() - interval '3 days'),
        (magma_demo_uuid('wo:2'), p_business, v_zone2, magma_demo_uuid('subscriber:10'),
         'REPAIR', 'EN_ROUTE', magma_demo_uuid('staff:tech'),
         now() + interval '2 hours',
         '["Check PoE injector","Test backhaul"]', '["PoE injector"]', '[]', 150.00, NULL,
         now() - interval '1 day'),
        (magma_demo_uuid('wo:3'), p_business, v_zone1, magma_demo_uuid('subscriber:7'),
         'RECOVERY', 'DONE', magma_demo_uuid('staff:ops'),
         now() - interval '5 days',
         '["Collect arrears","Restore service"]', '[]', '[]', 0, now() - interval '4 days',
         now() - interval '6 days')
    ON CONFLICT DO NOTHING;

    -- ------------------------------------------------------------------
    -- Alerts, audit, outbox, zone costs (serial keys: guarded inserts)
    -- ------------------------------------------------------------------
    IF NOT EXISTS (SELECT 1 FROM magma_alerts
                    WHERE business_id = p_business AND message LIKE '[demo]%') THEN
        INSERT INTO magma_alerts (business_id, zone_id, device_id, alert_type, severity, message,
                                  payload, acknowledged_by, acknowledged_at, resolved_at, created_at)
        VALUES
            (p_business, v_zone2, v_ap3, 'DEVICE_OFFLINE', 'CRITICAL',
             '[demo] Device Mamprobi AP is unreachable: timeout', '{"demo": true}'::jsonb,
             NULL, NULL, NULL, now() - interval '3 hours'),
            (p_business, v_zone1, v_gw1, 'BACKHAUL_DEGRADED', 'WARNING',
             '[demo] Downstream below 20 Mbps for 15 minutes', '{"demo": true, "mbps": 17}'::jsonb,
             magma_demo_uuid('staff:ops'), now() - interval '90 minutes', NULL,
             now() - interval '2 hours'),
            (p_business, v_zone1, NULL, 'PAYMENT_MISMATCH', 'CRITICAL',
             '[demo] Hubtel callback amount differs from the intent', '{"demo": true}'::jsonb,
             NULL, NULL, now() - interval '1 day', now() - interval '2 days'),
            (p_business, v_zone1, v_gw1, 'DEVICE_ONLINE', 'INFO',
             '[demo] Device Kaneshie GW1 is back online', '{"demo": true}'::jsonb,
             NULL, NULL, now() - interval '1 day', now() - interval '2 days'),
            (p_business, v_zone3, v_gw3, 'VOUCHER_STOCK_LOW', 'WARNING',
             '[demo] Fewer than 10 vouchers left for DAY-1', '{"demo": true}'::jsonb,
             NULL, NULL, NULL, now() - interval '6 hours');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM magma_audit_log
                    WHERE business_id = p_business AND action LIKE 'DEMO_%') THEN
        INSERT INTO magma_audit_log (business_id, actor_type, actor_id, action, entity, entity_id,
                                     before, after, ip_address, created_at)
        VALUES
            (p_business, 'STAFF', magma_demo_uuid('staff:admin'), 'DEMO_ZONE_CREATED', 'magma_zones',
             v_zone1, NULL, '{"status":"LIVE"}'::jsonb, '10.0.0.5', now() - interval '30 days'),
            (p_business, 'STAFF', magma_demo_uuid('staff:ops'), 'DEMO_SUBSCRIBER_SUSPENDED',
             'magma_subscribers', magma_demo_uuid('subscriber:5'), '{"status":"ACTIVE"}'::jsonb,
             '{"status":"SUSPENDED"}'::jsonb, '10.0.0.6', now() - interval '4 days'),
            (p_business, 'STAFF', magma_demo_uuid('staff:finance'), 'DEMO_REFUND_ISSUED',
             'magma_transactions', magma_demo_uuid('tx:9'), NULL,
             '{"reason":"Duplicate payment"}'::jsonb, '10.0.0.7', now() - interval '6 days'),
            (p_business, 'STAFF', magma_demo_uuid('staff:admin'), 'DEMO_VOUCHERS_VOIDED',
             'magma_voucher_batches', magma_demo_uuid('batch:3'), NULL,
             '{"reason":"Damaged stock"}'::jsonb, '10.0.0.5', now() - interval '8 days'),
            (p_business, 'STAFF', magma_demo_uuid('staff:tech'), 'DEMO_CONFIG_PUSHED',
             'magma_devices', v_gw1, NULL, '{"hotspot":"magma-kaneshie"}'::jsonb,
             '10.0.0.9', now() - interval '2 days'),
            (p_business, 'SYSTEM', NULL, 'DEMO_RADIUS_PROVISIONED', 'magma_subscriptions',
             magma_demo_uuid('subscription:1'), NULL, '{"radcheck":true}'::jsonb,
             '10.200.0.1', now() - interval '1 day');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM magma_outbox
                    WHERE payload->>'demo' = 'true' AND payload->>'scope' = p_business::text) THEN
        INSERT INTO magma_outbox (event_type, aggregate_id, payload, status, attempts,
                                  next_attempt_at, last_error, created_at, processed_at)
        VALUES
            ('RADIUS_PROVISION', magma_demo_uuid('subscription:1'),
             ('{"demo":true,"scope":"' || p_business || '","username":"mg:' || magma_demo_uuid('subscription:1') || '"}')::jsonb,
             'PENDING', 0, now() - interval '2 minutes', NULL, now() - interval '10 minutes', NULL),
            ('SMS_SEND', magma_demo_uuid('subscription:2'),
             ('{"demo":true,"scope":"' || p_business || '","msisdn":"0201234567","message":"Your MagmaNet pass is active"}')::jsonb,
             'PENDING', 1, now() - interval '1 minute', 'SMS gateway timeout',
             now() - interval '20 minutes', NULL),
            ('RADIUS_DEPROVISION', magma_demo_uuid('subscription:11'),
             ('{"demo":true,"scope":"' || p_business || '","username":"mg:' || magma_demo_uuid('subscription:11') || '"}')::jsonb,
             'DEAD', 8, now() + interval '30 minutes', 'NAS unreachable after 8 attempts',
             now() - interval '2 days', NULL),
            ('SUBSCRIPTION_ENDED', magma_demo_uuid('subscription:6'),
             ('{"demo":true,"scope":"' || p_business || '"}')::jsonb, 'DONE', 1, now() - interval '5 days', NULL,
             now() - interval '6 days', now() - interval '6 days');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM magma_zone_costs
                    WHERE business_id = p_business AND reference LIKE 'DEMO-%') THEN
        FOR v_m IN 0..1 LOOP
            INSERT INTO magma_zone_costs (id, business_id, zone_id, category, amount,
                                          period_start, period_end, vendor, reference, notes)
            VALUES
                (magma_demo_uuid('cost:' || v_m || ':1:backhaul'), p_business, v_zone1, 'BACKHAUL',
                 1500.00, (date_trunc('month', now()) - (v_m || ' months')::interval)::date,
                 (date_trunc('month', now()) - (v_m || ' months')::interval + interval '1 month - 1 day')::date,
                 'MTN Business', 'DEMO-COST-' || v_m || '-1', '100 Mbps business fibre'),
                (magma_demo_uuid('cost:' || v_m || ':1:power'), p_business, v_zone1, 'POWER',
                 250.00, (date_trunc('month', now()) - (v_m || ' months')::interval)::date,
                 (date_trunc('month', now()) - (v_m || ' months')::interval + interval '1 month - 1 day')::date,
                 'ECG', 'DEMO-COST-' || v_m || '-2', 'Prepaid meter top-up'),
                (magma_demo_uuid('cost:' || v_m || ':1:rent'), p_business, v_zone1, 'RENT',
                 600.00, (date_trunc('month', now()) - (v_m || ' months')::interval)::date,
                 (date_trunc('month', now()) - (v_m || ' months')::interval + interval '1 month - 1 day')::date,
                 'Market Association', 'DEMO-COST-' || v_m || '-3', 'Gateway mounting space'),
                (magma_demo_uuid('cost:' || v_m || ':2:backhaul'), p_business, v_zone2, 'BACKHAUL',
                 900.00, (date_trunc('month', now()) - (v_m || ' months')::interval)::date,
                 (date_trunc('month', now()) - (v_m || ' months')::interval + interval '1 month - 1 day')::date,
                 'Telecel', 'DEMO-COST-' || v_m || '-4', '60 Mbps fibre'),
                (magma_demo_uuid('cost:' || v_m || ':3:backhaul'), p_business, v_zone3, 'BACKHAUL',
                 800.00, (date_trunc('month', now()) - (v_m || ' months')::interval)::date,
                 (date_trunc('month', now()) - (v_m || ' months')::interval + interval '1 month - 1 day')::date,
                 'MTN Business', 'DEMO-COST-' || v_m || '-5', '50 Mbps fibre')
            ON CONFLICT DO NOTHING;
        END LOOP;
    END IF;

    -- ------------------------------------------------------------------
    -- Summary
    -- ------------------------------------------------------------------
    kind := 'tenant'; detail := p_business::text; RETURN NEXT;
    kind := 'zones'; detail := (SELECT count(*)::text FROM magma_zones WHERE business_id = p_business); RETURN NEXT;
    kind := 'plans'; detail := (SELECT count(*)::text FROM magma_data_plans WHERE business_id = p_business); RETURN NEXT;
    kind := 'devices'; detail := (SELECT count(*)::text FROM magma_devices WHERE business_id = p_business); RETURN NEXT;
    kind := 'subscribers'; detail := (SELECT count(*)::text FROM magma_subscribers WHERE business_id = p_business); RETURN NEXT;
    kind := 'subscriptions'; detail := (SELECT count(*)::text FROM magma_subscriptions WHERE business_id = p_business); RETURN NEXT;
    kind := 'sessions'; detail := (SELECT count(*)::text FROM magma_sessions WHERE business_id = p_business); RETURN NEXT;
    kind := 'vouchers'; detail := (SELECT count(*)::text FROM magma_vouchers WHERE business_id = p_business); RETURN NEXT;
    kind := 'vouchers redeemable'; detail := (SELECT count(*)::text FROM magma_vouchers WHERE business_id = p_business AND status = 'AVAILABLE'); RETURN NEXT;
    kind := 'transactions'; detail := (SELECT count(*)::text FROM magma_transactions WHERE business_id = p_business); RETURN NEXT;
    kind := 'agents'; detail := (SELECT count(*)::text FROM magma_agents WHERE business_id = p_business); RETURN NEXT;
    kind := 'staff'; detail := (SELECT count(*)::text FROM magma_staff WHERE business_id = p_business); RETURN NEXT;
    kind := 'tickets'; detail := (SELECT count(*)::text FROM magma_tickets WHERE business_id = p_business); RETURN NEXT;
    kind := 'alerts (demo)'; detail := (SELECT count(*)::text FROM magma_alerts WHERE business_id = p_business AND message LIKE '[demo]%'); RETURN NEXT;
    kind := 'test agent'; detail := '0244555666 / PIN 1234'; RETURN NEXT;
    kind := 'test staff'; detail := '0244000001 / PIN 2468'; RETURN NEXT;
    kind := 'test voucher'; detail := 'TEST-' || v_tag || '-001 (plan DAY-1, Kaneshie)'; RETURN NEXT;
    kind := 'test hotspot login'; detail := 'mg:<subscription uuid> / demo-radius-1'; RETURN NEXT;
END;
$fn$;

-- ===========================================================================
-- Run it
-- ===========================================================================
SELECT * FROM magma_demo_load(
    COALESCE(NULLIF(:'business_id', '')::uuid,
             (SELECT id FROM business_accounts ORDER BY created_at LIMIT 1)),
    :'secrets_key',
    :'hmac_key',
    :'pepper');
