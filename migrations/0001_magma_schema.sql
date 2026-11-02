-- MagmaNet community WiFi — core schema (public)
-- Apply with: psql "$DATABASE_URL" -f migrations/0001_magma_schema.sql
--
-- Notes:
--  * Every table is scoped by business_id (multi-tenant / franchise friendly).
--  * Money is NUMERIC(12,2); Drogon returns DECIMAL as text.
--  * All timestamps are TIMESTAMPTZ and UTC; render in Africa/Accra at the edge.
--  * magma_sessions is intentionally NOT partitioned in v1 (ON CONFLICT needs the
--    partition key). migrations/0004_partitioning.sql contains the scale-up path.

BEGIN;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- ---------------------------------------------------------------------------
-- Zones and assets
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS magma_zones (
    id                    UUID PRIMARY KEY,
    business_id           UUID NOT NULL REFERENCES business_accounts(id),
    zone_code             VARCHAR(24) NOT NULL,
    name                  VARCHAR(120) NOT NULL,
    district              VARCHAR(80) NOT NULL,
    region                VARCHAR(80) NOT NULL,
    gps_lat               NUMERIC(9,6),
    gps_lng               NUMERIC(9,6),
    backhaul_provider     VARCHAR(24),            -- MTN_FIBRE | TELECEL_FIBRE | OTHER
    backhaul_circuit_ref  VARCHAR(64),
    upstream_down_mbps    NUMERIC(8,2),
    upstream_up_mbps      NUMERIC(8,2),
    upstream_monthly_cost NUMERIC(12,2),
    upstream_is_cgnat     BOOLEAN DEFAULT TRUE,
    public_ip             INET,
    tunnel_ip             INET,
    timezone              VARCHAR(64) DEFAULT 'Africa/Accra',
    status                VARCHAR(16) NOT NULL DEFAULT 'PLANNED',
    launched_at           TIMESTAMPTZ,
    notes                 TEXT,
    created_at            TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at            TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_zones_status_chk CHECK (status IN ('PLANNED','BUILD','LIVE','PAUSED','RETIRED')),
    CONSTRAINT magma_zones_code_uq UNIQUE (business_id, zone_code)
);
CREATE INDEX IF NOT EXISTS idx_magma_zones_business ON magma_zones (business_id, status);

CREATE TABLE IF NOT EXISTS magma_devices (
    id                       UUID PRIMARY KEY,
    business_id              UUID NOT NULL,
    zone_id                  UUID REFERENCES magma_zones(id) ON DELETE SET NULL,
    name                     VARCHAR(120) NOT NULL,
    role                     VARCHAR(16) NOT NULL,
    vendor                   VARCHAR(32) DEFAULT 'MikroTik',
    model                    VARCHAR(64),
    serial_number            VARCHAR(64) UNIQUE,
    mac_address              MACADDR,
    routeros_version         VARCHAR(24),
    management_ip            INET,
    tunnel_ip                INET,
    rest_scheme              VARCHAR(8) DEFAULT 'http',
    rest_port                INT DEFAULT 80,
    api_username             VARCHAR(64),
    api_secret_ciphertext    TEXT,
    radius_secret_ciphertext TEXT,
    enrolment_status         VARCHAR(16) NOT NULL DEFAULT 'PENDING',
    status                   VARCHAR(16) NOT NULL DEFAULT 'UNKNOWN',
    last_seen_at             TIMESTAMPTZ,
    uptime_seconds           BIGINT,
    cpu_load                 SMALLINT,
    free_memory_mb           INT,
    temperature_c            NUMERIC(5,2),
    installed_at             DATE,
    assigned_subscriber_id   UUID,
    asset_cost               NUMERIC(12,2),
    notes                    TEXT,
    created_at               TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at               TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_devices_role_chk CHECK (role IN ('GATEWAY','AP','SWITCH','CPE','ONT','UPS')),
    CONSTRAINT magma_devices_status_chk CHECK (status IN ('ONLINE','OFFLINE','DEGRADED','UNKNOWN'))
);
CREATE INDEX IF NOT EXISTS idx_magma_devices_zone ON magma_devices (zone_id, role);
CREATE INDEX IF NOT EXISTS idx_magma_devices_status ON magma_devices (business_id, status);

-- ---------------------------------------------------------------------------
-- Plans
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS magma_data_plans (
    id                 UUID PRIMARY KEY,
    business_id        UUID NOT NULL,
    plan_code          VARCHAR(32) NOT NULL,
    name               VARCHAR(80) NOT NULL,
    plan_type          VARCHAR(16) NOT NULL,
    duration_minutes   INT,
    data_cap_mb        BIGINT,
    fair_use_cap_mb    BIGINT,
    throttle_down_kbps INT,
    throttle_up_kbps   INT,
    down_kbps          INT NOT NULL,
    up_kbps            INT NOT NULL,
    burst_down_kbps    INT,
    burst_up_kbps      INT,
    device_limit       SMALLINT NOT NULL DEFAULT 1,
    price              NUMERIC(12,2) NOT NULL,
    wholesale_price    NUMERIC(12,2),
    agent_commission   NUMERIC(12,2) NOT NULL DEFAULT 0,
    tax_inclusive      BOOLEAN NOT NULL DEFAULT TRUE,
    validity_days      SMALLINT NOT NULL DEFAULT 30,
    is_public          BOOLEAN NOT NULL DEFAULT TRUE,
    is_active          BOOLEAN NOT NULL DEFAULT TRUE,
    sort_order         SMALLINT NOT NULL DEFAULT 0,
    effective_from     TIMESTAMPTZ NOT NULL DEFAULT now(),
    effective_to       TIMESTAMPTZ,
    created_at         TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at         TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_plans_type_chk CHECK (plan_type IN ('TIME','VOLUME','UNLIMITED')),
    CONSTRAINT magma_plans_code_uq UNIQUE (business_id, plan_code)
);
CREATE INDEX IF NOT EXISTS idx_magma_plans_active ON magma_data_plans (business_id, is_active);

CREATE TABLE IF NOT EXISTS magma_plan_zones (
    plan_id UUID NOT NULL REFERENCES magma_data_plans(id) ON DELETE CASCADE,
    zone_id UUID NOT NULL REFERENCES magma_zones(id) ON DELETE CASCADE,
    PRIMARY KEY (plan_id, zone_id)
);

-- ---------------------------------------------------------------------------
-- Subscribers and client devices
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS magma_subscribers (
    id                      UUID PRIMARY KEY,
    business_id             UUID NOT NULL,
    msisdn                  VARCHAR(20),
    full_name               VARCHAR(120),
    email                   VARCHAR(120),
    ghana_card_ciphertext   TEXT,
    ghana_card_masked       VARCHAR(16),
    subscriber_type         VARCHAR(16) NOT NULL DEFAULT 'WALK_IN',
    auth_type               VARCHAR(16) NOT NULL DEFAULT 'HOTSPOT',
    pppoe_username          VARCHAR(64),
    pppoe_secret_ciphertext TEXT,
    home_zone_id            UUID REFERENCES magma_zones(id) ON DELETE SET NULL,
    service_address         TEXT,
    gps_lat                 NUMERIC(9,6),
    gps_lng                 NUMERIC(9,6),
    status                  VARCHAR(16) NOT NULL DEFAULT 'ACTIVE',
    kyc_status              VARCHAR(16) NOT NULL DEFAULT 'NOT_REQUIRED',
    consent_at              TIMESTAMPTZ,
    consent_version         VARCHAR(16),
    first_seen_at           TIMESTAMPTZ DEFAULT now(),
    last_seen_at            TIMESTAMPTZ,
    created_at              TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at              TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_subscribers_type_chk CHECK (subscriber_type IN ('WALK_IN','RESIDENTIAL','BUSINESS')),
    CONSTRAINT magma_subscribers_auth_chk CHECK (auth_type IN ('HOTSPOT','PPPOE')),
    CONSTRAINT magma_subscribers_status_chk CHECK (status IN ('ACTIVE','SUSPENDED','CHURNED'))
);
CREATE UNIQUE INDEX IF NOT EXISTS magma_subscribers_msisdn_uq
    ON magma_subscribers (business_id, msisdn) WHERE msisdn IS NOT NULL;
CREATE INDEX IF NOT EXISTS idx_magma_subscribers_home_zone ON magma_subscribers (home_zone_id, status);

CREATE TABLE IF NOT EXISTS magma_client_devices (
    id             UUID PRIMARY KEY,
    business_id    UUID NOT NULL,
    subscriber_id  UUID NOT NULL REFERENCES magma_subscribers(id) ON DELETE CASCADE,
    mac_address    MACADDR NOT NULL,
    label          VARCHAR(64),
    first_seen_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_seen_at   TIMESTAMPTZ,
    is_blocked     BOOLEAN NOT NULL DEFAULT FALSE,
    blocked_reason TEXT,
    CONSTRAINT magma_client_devices_uq UNIQUE (business_id, mac_address)
);
CREATE INDEX IF NOT EXISTS idx_magma_client_devices_sub ON magma_client_devices (subscriber_id);

-- ---------------------------------------------------------------------------
-- Subscriptions / sessions / transactions
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS magma_subscriptions (
    id                UUID PRIMARY KEY,
    business_id       UUID NOT NULL,
    subscriber_id     UUID NOT NULL REFERENCES magma_subscribers(id),
    plan_id           UUID NOT NULL REFERENCES magma_data_plans(id),
    zone_id           UUID NOT NULL REFERENCES magma_zones(id),
    radius_username   VARCHAR(64) NOT NULL UNIQUE,
    radius_secret_ct  TEXT NOT NULL,
    status            VARCHAR(16) NOT NULL DEFAULT 'PENDING',
    purchased_minutes INT,
    purchased_mb      BIGINT,
    used_seconds      BIGINT NOT NULL DEFAULT 0,
    used_mb           BIGINT NOT NULL DEFAULT 0,
    starts_at         TIMESTAMPTZ,
    expires_at        TIMESTAMPTZ,
    activated_at      TIMESTAMPTZ,
    ended_at          TIMESTAMPTZ,
    plan_snapshot     JSONB NOT NULL,
    payment_id        UUID,
    voucher_id        UUID,
    agent_id          UUID,
    parent_id         UUID REFERENCES magma_subscriptions(id),
    auto_renew        BOOLEAN NOT NULL DEFAULT FALSE,
    throttled_at      TIMESTAMPTZ,
    created_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_subs_status_chk CHECK (status IN ('PENDING','ACTIVE','EXPIRED','DEPLETED','SUSPENDED','CANCELLED'))
);
CREATE INDEX IF NOT EXISTS idx_magma_subs_enforce ON magma_subscriptions (status, expires_at);
CREATE INDEX IF NOT EXISTS idx_magma_subs_subscriber ON magma_subscriptions (subscriber_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_magma_subs_zone ON magma_subscriptions (zone_id, status);

CREATE TABLE IF NOT EXISTS magma_sessions (
    id                 UUID PRIMARY KEY,
    business_id        UUID NOT NULL,
    subscription_id    UUID NOT NULL REFERENCES magma_subscriptions(id) ON DELETE CASCADE,
    subscriber_id      UUID NOT NULL,
    zone_id            UUID NOT NULL,
    device_id          UUID REFERENCES magma_devices(id) ON DELETE SET NULL,
    acct_session_id    VARCHAR(128) NOT NULL UNIQUE,
    nas_ip             INET,
    framed_ip          INET,
    mac_address        MACADDR,
    started_at         TIMESTAMPTZ NOT NULL,
    last_interim_at    TIMESTAMPTZ,
    ended_at           TIMESTAMPTZ,
    last_input_octets  BIGINT NOT NULL DEFAULT 0,
    last_output_octets BIGINT NOT NULL DEFAULT 0,
    input_octets       BIGINT NOT NULL DEFAULT 0,
    output_octets      BIGINT NOT NULL DEFAULT 0,
    session_seconds    BIGINT NOT NULL DEFAULT 0,
    terminate_cause    VARCHAR(48)
);
CREATE INDEX IF NOT EXISTS idx_magma_sessions_sub ON magma_sessions (subscription_id, started_at DESC);
CREATE INDEX IF NOT EXISTS idx_magma_sessions_open ON magma_sessions (ended_at) WHERE ended_at IS NULL;

CREATE TABLE IF NOT EXISTS magma_transactions (
    id                 UUID PRIMARY KEY,
    business_id        UUID NOT NULL,
    subscriber_id      UUID,
    subscription_id    UUID,
    agent_id           UUID,
    voucher_id         UUID,
    purpose            VARCHAR(24) NOT NULL,
    amount             NUMERIC(12,2) NOT NULL,
    tax_amount         NUMERIC(12,2) NOT NULL DEFAULT 0,
    net_amount         NUMERIC(12,2),
    tax_breakdown      JSONB,
    currency           CHAR(3) NOT NULL DEFAULT 'GHS',
    payment_method     VARCHAR(24),
    provider           VARCHAR(24),
    provider_reference VARCHAR(128),
    internal_reference VARCHAR(64) NOT NULL UNIQUE,
    status             VARCHAR(16) NOT NULL DEFAULT 'PENDING',
    payer_msisdn       VARCHAR(20),
    failure_reason     TEXT,
    raw_callback       JSONB,
    initiated_at       TIMESTAMPTZ NOT NULL DEFAULT now(),
    paid_at            TIMESTAMPTZ,
    created_at         TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at         TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_tx_status_chk CHECK (status IN ('PENDING','SUCCESS','FAILED','REVERSED')),
    CONSTRAINT magma_tx_provider_ref_uq UNIQUE (provider, provider_reference)
);
CREATE INDEX IF NOT EXISTS idx_magma_tx_business ON magma_transactions (business_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_magma_tx_status ON magma_transactions (status, initiated_at);

CREATE TABLE IF NOT EXISTS magma_checkout_intents (
    id              UUID PRIMARY KEY,
    business_id     UUID NOT NULL,
    zone_id         UUID NOT NULL,
    plan_id         UUID NOT NULL,
    mac_address     MACADDR NOT NULL,
    ip_address      INET,
    msisdn          VARCHAR(20),
    subscriber_id   UUID,
    transaction_id  UUID,
    status          VARCHAR(16) NOT NULL DEFAULT 'NEW',
    login_attempts  SMALLINT NOT NULL DEFAULT 0,
    expires_at      TIMESTAMPTZ NOT NULL,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_intent_status_chk CHECK (status IN ('NEW','AWAITING_PAYMENT','PAID','PROVISIONED','FAILED','EXPIRED'))
);
CREATE INDEX IF NOT EXISTS idx_magma_intents_mac ON magma_checkout_intents (mac_address, created_at DESC);

-- ---------------------------------------------------------------------------
-- Vouchers and agents
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS magma_voucher_batches (
    id               UUID PRIMARY KEY,
    business_id      UUID NOT NULL,
    batch_code       VARCHAR(32) NOT NULL UNIQUE,
    plan_id          UUID NOT NULL REFERENCES magma_data_plans(id),
    zone_id          UUID REFERENCES magma_zones(id) ON DELETE SET NULL,
    quantity         INT NOT NULL,
    unit_price       NUMERIC(12,2) NOT NULL,
    wholesale_price  NUMERIC(12,2) NOT NULL,
    owner_type       VARCHAR(16) NOT NULL DEFAULT 'BUSINESS',
    owner_agent_id   UUID,
    status           VARCHAR(16) NOT NULL DEFAULT 'DRAFT',
    expires_at       TIMESTAMPTZ,
    created_by       UUID,
    created_at       TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_batch_owner_chk CHECK (owner_type IN ('BUSINESS','AGENT')),
    CONSTRAINT magma_batch_status_chk CHECK (status IN ('DRAFT','ACTIVE','EXHAUSTED','VOID'))
);

CREATE TABLE IF NOT EXISTS magma_vouchers (
    id                     UUID PRIMARY KEY,
    business_id            UUID NOT NULL,
    batch_id               UUID NOT NULL REFERENCES magma_voucher_batches(id) ON DELETE CASCADE,
    plan_id                UUID NOT NULL,
    zone_id                UUID,
    serial                 VARCHAR(32) NOT NULL UNIQUE,
    code_hash              TEXT NOT NULL UNIQUE,
    code_ciphertext        TEXT NOT NULL,
    status                 VARCHAR(16) NOT NULL DEFAULT 'AVAILABLE',
    agent_id               UUID,
    price                  NUMERIC(12,2),
    commission             NUMERIC(12,2),
    sold_at                TIMESTAMPTZ,
    redeemed_at            TIMESTAMPTZ,
    redeemed_subscriber_id UUID,
    subscription_id        UUID,
    created_at             TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at             TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_voucher_status_chk CHECK (status IN ('AVAILABLE','ASSIGNED','SOLD','REDEEMED','EXPIRED','VOID'))
);
CREATE INDEX IF NOT EXISTS idx_magma_vouchers_agent ON magma_vouchers (agent_id, status);
CREATE INDEX IF NOT EXISTS idx_magma_vouchers_batch ON magma_vouchers (batch_id, status);

CREATE TABLE IF NOT EXISTS magma_agents (
    id                UUID PRIMARY KEY,
    business_id       UUID NOT NULL,
    agent_code        VARCHAR(24) NOT NULL UNIQUE,
    name              VARCHAR(120) NOT NULL,
    msisdn            VARCHAR(20) NOT NULL,
    email             VARCHAR(120),
    pin_hash          VARCHAR(255) NOT NULL,
    tier              VARCHAR(24) NOT NULL DEFAULT 'STANDARD',
    commission_rate   NUMERIC(5,2) NOT NULL DEFAULT 0,
    territory_zone_id UUID REFERENCES magma_zones(id) ON DELETE SET NULL,
    wallet_account_id VARCHAR(64),
    status            VARCHAR(16) NOT NULL DEFAULT 'ACTIVE',
    kyc_status        VARCHAR(16) NOT NULL DEFAULT 'PENDING',
    created_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_agents_status_chk CHECK (status IN ('ACTIVE','SUSPENDED','CLOSED')),
    CONSTRAINT magma_agents_msisdn_uq UNIQUE (business_id, msisdn)
);

CREATE TABLE IF NOT EXISTS magma_agent_ledger (
    id             UUID PRIMARY KEY,
    business_id    UUID NOT NULL,
    agent_id       UUID NOT NULL REFERENCES magma_agents(id) ON DELETE CASCADE,
    entry_type     VARCHAR(24) NOT NULL,
    amount         NUMERIC(12,2) NOT NULL,
    balance_after  NUMERIC(12,2) NOT NULL,
    reference      VARCHAR(64) UNIQUE,
    transaction_id UUID,
    voucher_id     UUID,
    description    TEXT,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_ledger_type_chk CHECK (entry_type IN ('EARNING','PAYOUT','ADJUSTMENT','STOCK_PURCHASE','REVERSAL'))
);
CREATE INDEX IF NOT EXISTS idx_magma_ledger_agent ON magma_agent_ledger (agent_id, created_at DESC);

-- ---------------------------------------------------------------------------
-- Invoices, costs, ops
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS magma_invoices (
    id              UUID PRIMARY KEY,
    business_id     UUID NOT NULL,
    subscriber_id   UUID NOT NULL,
    subscription_id UUID,
    invoice_number  VARCHAR(32) NOT NULL UNIQUE,
    period_start    DATE NOT NULL,
    period_end      DATE NOT NULL,
    subtotal        NUMERIC(12,2) NOT NULL,
    tax_amount      NUMERIC(12,2) NOT NULL,
    total           NUMERIC(12,2) NOT NULL,
    amount_paid     NUMERIC(12,2) NOT NULL DEFAULT 0,
    status          VARCHAR(16) NOT NULL DEFAULT 'DRAFT',
    due_at          DATE,
    issued_at       TIMESTAMPTZ,
    paid_at         TIMESTAMPTZ,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_invoice_status_chk CHECK (status IN ('DRAFT','ISSUED','PAID','OVERDUE','VOID'))
);
CREATE INDEX IF NOT EXISTS idx_magma_invoices_sub ON magma_invoices (subscriber_id, status);

CREATE TABLE IF NOT EXISTS magma_zone_costs (
    id           UUID PRIMARY KEY,
    business_id  UUID NOT NULL,
    zone_id      UUID NOT NULL REFERENCES magma_zones(id) ON DELETE CASCADE,
    category     VARCHAR(24) NOT NULL,
    amount       NUMERIC(12,2) NOT NULL,
    period_start DATE NOT NULL,
    period_end   DATE NOT NULL,
    vendor       VARCHAR(120),
    reference    VARCHAR(64),
    notes        TEXT,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_cost_category_chk CHECK (category IN ('BACKHAUL','POWER','RENT','STAFF','TRANSPORT','OTHER'))
);
CREATE INDEX IF NOT EXISTS idx_magma_costs_zone ON magma_zone_costs (zone_id, period_start DESC);

CREATE TABLE IF NOT EXISTS magma_tickets (
    id            UUID PRIMARY KEY,
    business_id   UUID NOT NULL,
    zone_id       UUID,
    subscriber_id UUID,
    agent_id      UUID,
    category      VARCHAR(24) NOT NULL,
    priority      VARCHAR(8) NOT NULL DEFAULT 'NORMAL',
    status        VARCHAR(16) NOT NULL DEFAULT 'OPEN',
    subject       VARCHAR(160) NOT NULL,
    description   TEXT,
    assigned_to   UUID,
    sla_due_at    TIMESTAMPTZ,
    resolved_at   TIMESTAMPTZ,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_ticket_status_chk CHECK (status IN ('OPEN','IN_PROGRESS','RESOLVED','CLOSED'))
);
CREATE INDEX IF NOT EXISTS idx_magma_tickets_status ON magma_tickets (business_id, status, created_at DESC);

CREATE TABLE IF NOT EXISTS magma_ticket_messages (
    id          UUID PRIMARY KEY,
    ticket_id   UUID NOT NULL REFERENCES magma_tickets(id) ON DELETE CASCADE,
    author_type VARCHAR(16) NOT NULL,
    author_id   UUID,
    body        TEXT NOT NULL,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS magma_work_orders (
    id            UUID PRIMARY KEY,
    business_id   UUID NOT NULL,
    zone_id       UUID,
    subscriber_id UUID,
    order_type    VARCHAR(16) NOT NULL,
    status        VARCHAR(16) NOT NULL DEFAULT 'SCHEDULED',
    assigned_to   UUID,
    scheduled_at  TIMESTAMPTZ,
    checklist     JSONB,
    materials     JSONB,
    photos        JSONB,
    cost          NUMERIC(12,2) NOT NULL DEFAULT 0,
    completed_at  TIMESTAMPTZ,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_wo_type_chk CHECK (order_type IN ('INSTALL','REPAIR','RECOVERY','DEINSTALL')),
    CONSTRAINT magma_wo_status_chk CHECK (status IN ('SCHEDULED','EN_ROUTE','DONE','CANCELLED'))
);

CREATE TABLE IF NOT EXISTS magma_staff (
    id            UUID PRIMARY KEY,
    business_id   UUID NOT NULL,
    full_name     VARCHAR(120) NOT NULL,
    msisdn        VARCHAR(20),
    email         VARCHAR(120),
    pin_hash      VARCHAR(255) NOT NULL,
    role          VARCHAR(24) NOT NULL,
    zone_ids      UUID[],
    status        VARCHAR(16) NOT NULL DEFAULT 'ACTIVE',
    last_login_at TIMESTAMPTZ,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT magma_staff_role_chk CHECK (role IN ('ADMIN','OPS','TECHNICIAN','SUPPORT','FINANCE','AGENT_MANAGER')),
    CONSTRAINT magma_staff_status_chk CHECK (status IN ('ACTIVE','SUSPENDED'))
);

-- ---------------------------------------------------------------------------
-- Alerts, outbox, audit
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS magma_alerts (
    id              BIGSERIAL PRIMARY KEY,
    business_id     UUID NOT NULL,
    zone_id         UUID,
    device_id       UUID,
    alert_type      VARCHAR(32) NOT NULL,
    severity        VARCHAR(8) NOT NULL,
    message         TEXT NOT NULL,
    payload         JSONB,
    acknowledged_by UUID,
    acknowledged_at TIMESTAMPTZ,
    resolved_at     TIMESTAMPTZ,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_magma_alerts_open ON magma_alerts (business_id, created_at DESC)
    WHERE resolved_at IS NULL;

CREATE TABLE IF NOT EXISTS magma_outbox (
    id              BIGSERIAL PRIMARY KEY,
    event_type      VARCHAR(48) NOT NULL,
    aggregate_id    UUID,
    payload         JSONB NOT NULL,
    status          VARCHAR(16) NOT NULL DEFAULT 'PENDING',
    attempts        SMALLINT NOT NULL DEFAULT 0,
    next_attempt_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_error      TEXT,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    processed_at    TIMESTAMPTZ,
    CONSTRAINT magma_outbox_status_chk CHECK (status IN ('PENDING','DONE','FAILED','DEAD'))
);
CREATE INDEX IF NOT EXISTS idx_magma_outbox_due ON magma_outbox (status, next_attempt_at);

CREATE TABLE IF NOT EXISTS magma_audit_log (
    id          BIGSERIAL PRIMARY KEY,
    business_id UUID,
    actor_type  VARCHAR(16),
    actor_id    UUID,
    action      VARCHAR(64) NOT NULL,
    entity      VARCHAR(64) NOT NULL,
    entity_id   UUID,
    before      JSONB,
    after       JSONB,
    ip_address  INET,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_magma_audit_entity ON magma_audit_log (entity, entity_id, created_at DESC);

COMMIT;
