-- MagmaNet — seed data and tenant bootstrap helpers.
-- Apply with: psql "$DATABASE_URL" -f migrations/0003_magma_seed.sql
--
-- Nothing here is global: every helper takes the tenant UUID so a franchise
-- can be provisioned without touching another business's catalogue.

BEGIN;

-- ---------------------------------------------------------------------------
-- Default plan catalogue for one tenant.
-- Prices are Ghana cedis, tax inclusive, and deliberately conservative; the
-- operator edits them in the admin app afterwards.
-- ---------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION magma_seed_default_plans(p_business UUID)
RETURNS INT
LANGUAGE plpgsql
AS $$
DECLARE
    inserted INT := 0;
BEGIN
    INSERT INTO magma_data_plans (
        id, business_id, plan_code, name, plan_type, duration_minutes, data_cap_mb,
        fair_use_cap_mb, throttle_down_kbps, throttle_up_kbps, down_kbps, up_kbps,
        device_limit, price, wholesale_price, agent_commission, tax_inclusive,
        validity_days, is_public, is_active, sort_order)
    VALUES
        (gen_random_uuid(), p_business, 'HOUR-1', '1 Hour Pass', 'TIME', 60, NULL,
         NULL, NULL, NULL, 4000, 2000, 1, 3.00, 2.00, 0.20, TRUE, 1, TRUE, TRUE, 10),
        (gen_random_uuid(), p_business, 'DAY-1', '24 Hour Pass', 'TIME', 1440, NULL,
         NULL, NULL, NULL, 5000, 2500, 1, 8.00, 6.00, 0.50, TRUE, 1, TRUE, TRUE, 20),
        (gen_random_uuid(), p_business, 'DAY-3', '3 Day Pass', 'TIME', 4320, NULL,
         NULL, NULL, NULL, 5000, 2500, 2, 20.00, 15.00, 1.50, TRUE, 3, TRUE, TRUE, 30),
        (gen_random_uuid(), p_business, 'WEEK-1', '7 Day Pass', 'VOLUME', NULL, 15000,
         NULL, NULL, NULL, 6000, 3000, 2, 40.00, 30.00, 3.00, TRUE, 7, TRUE, TRUE, 40),
        (gen_random_uuid(), p_business, 'MONTH-1', '30 Day Unlimited', 'UNLIMITED', NULL, NULL,
         30000, 1024, 512, 8000, 4000, 3, 120.00, 95.00, 8.00, TRUE, 30, TRUE, TRUE, 50)
    ON CONFLICT (business_id, plan_code) DO NOTHING;

    GET DIAGNOSTICS inserted = ROW_COUNT;
    RETURN inserted;
END;
$$;

-- ---------------------------------------------------------------------------
-- Default zone for one tenant (edit coordinates before going live).
-- ---------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION magma_seed_default_zone(
    p_business UUID,
    p_zone_code TEXT,
    p_name TEXT,
    p_district TEXT DEFAULT 'Accra Metropolitan',
    p_region TEXT DEFAULT 'Greater Accra')
RETURNS UUID
LANGUAGE plpgsql
AS $$
DECLARE
    v_id UUID;
BEGIN
    INSERT INTO magma_zones (id, business_id, zone_code, name, district, region, status,
                             backhaul_provider, upstream_down_mbps, upstream_up_mbps,
                             upstream_is_cgnat)
    VALUES (gen_random_uuid(), p_business, p_zone_code, p_name, p_district, p_region, 'LIVE',
            'MTN_FIBRE', 100, 50, TRUE)
    ON CONFLICT (business_id, zone_code) DO NOTHING
    RETURNING id INTO v_id;

    IF v_id IS NULL THEN
        SELECT id INTO v_id FROM magma_zones
        WHERE business_id = p_business AND zone_code = p_zone_code;
    END IF;
    RETURN v_id;
END;
$$;

-- ---------------------------------------------------------------------------
-- Provider configuration blocks.
--
-- Secrets are read from configuration_data by MagmaPayService / MagmaSmsService.
-- Never put live keys in this file: the placeholders below only create the
-- shape so the admin UI has something to edit. `MagmaSettings` also accepts
-- environment overrides (HUBTEL_*, PAYSTACK_*).
-- ---------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION magma_seed_provider_config(p_business UUID)
RETURNS VOID
LANGUAGE plpgsql
AS $$
BEGIN
    UPDATE business_accounts
    SET configuration_data = COALESCE(configuration_data, '{}'::jsonb)
        || jsonb_build_object(
            'HubtelPaymentApi', COALESCE(configuration_data->'HubtelPaymentApi',
                '{"ApiIdOrUsername":"","ApiKeyOrPassword":"","PosSalesAccountId":"","CallbackUrl":""}'::jsonb),
            'HubtelSmsApi', COALESCE(configuration_data->'HubtelSmsApi',
                '{"ApiIdOrUsername":"","ApiKeyOrPassword":"","SenderId":"MagmaNet"}'::jsonb),
            'PaystackApi', COALESCE(configuration_data->'PaystackApi',
                '{"ApiKey":"","CallbackUrl":""}'::jsonb)
        )
    WHERE id = p_business;
END;
$$;

-- ---------------------------------------------------------------------------
-- Convenience wrapper used by the provisioning runbook:
--   SELECT magma_provision_tenant('<business uuid>', 'KANESHIE', 'Kaneshie Market');
-- ---------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION magma_provision_tenant(
    p_business UUID,
    p_zone_code TEXT,
    p_zone_name TEXT,
    p_district TEXT DEFAULT 'Accra Metropolitan')
RETURNS TABLE (plans INT, zone UUID)
LANGUAGE plpgsql
AS $$
BEGIN
    PERFORM magma_seed_provider_config(p_business);
    RETURN QUERY SELECT magma_seed_default_plans(p_business),
                        magma_seed_default_zone(p_business, p_zone_code, p_zone_name, p_district);
END;
$$;

COMMIT;

-- Example (run manually, once per tenant):
--   SELECT * FROM magma_provision_tenant(
--       (SELECT id FROM business_accounts ORDER BY created_at LIMIT 1),
--       'KANESHIE', 'Kaneshie Market', 'Okaikoi South');
