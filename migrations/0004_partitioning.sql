-- MagmaNet — partitioning and retention.
-- Apply with: psql "$DATABASE_URL" -f migrations/0004_partitioning.sql
--
-- 0001 keeps magma_sessions as a plain table because ON CONFLICT needs the
-- partition key. This migration converts it to monthly RANGE partitions once
-- session volume justifies it, and installs the retention helpers.

BEGIN;

-- ---------------------------------------------------------------------------
-- Monthly partition helper. Creates every month from p_from to p_to inclusive.
-- ---------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION magma_ensure_session_partitions(p_from DATE, p_to DATE)
RETURNS INT
LANGUAGE plpgsql
AS $$
DECLARE
    v_start DATE := date_trunc('month', p_from)::date;
    v_end   DATE := date_trunc('month', p_to)::date;
    v_name  TEXT;
    created INT := 0;
BEGIN
    IF to_regclass('public.magma_sessions') IS NULL THEN
        RETURN 0;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_partitioned_table pt
                   JOIN pg_class c ON c.oid = pt.partrelid
                   WHERE c.relname = 'magma_sessions') THEN
        RETURN 0;  -- plain table: nothing to partition
    END IF;

    WHILE v_start < v_end LOOP
        v_name := 'magma_sessions_' || to_char(v_start, 'YYYY_MM');
        IF to_regclass('public.' || v_name) IS NULL THEN
            EXECUTE format(
                'CREATE TABLE %I PARTITION OF magma_sessions FOR VALUES FROM (%L) TO (%L)',
                v_name, v_start, (v_start + interval '1 month')::date);
            created := created + 1;
        END IF;
        v_start := (v_start + interval '1 month')::date;
    END LOOP;
    RETURN created;
END;
$$;

-- ---------------------------------------------------------------------------
-- Convert magma_sessions into a partitioned table (idempotent).
--
-- The legacy table is copied in place. Unique indexes on a partitioned table
-- must include the partition key, so acct_session_id becomes unique per month;
-- that is sufficient because FreeRADIUS restarts Acct-Session-Id per session.
-- ---------------------------------------------------------------------------
DO $$
DECLARE
    already_partitioned BOOLEAN;
    row_count BIGINT;
    v_min DATE;
BEGIN
    SELECT EXISTS (SELECT 1 FROM pg_partitioned_table pt
                   JOIN pg_class c ON c.oid = pt.partrelid
                   WHERE c.relname = 'magma_sessions')
      INTO already_partitioned;

    IF already_partitioned THEN
        RAISE NOTICE 'magma_sessions is already partitioned';
        RETURN;
    END IF;

    SELECT count(*) INTO row_count FROM magma_sessions;
    RAISE NOTICE 'Converting magma_sessions (% rows) to partitioned table', row_count;

    ALTER TABLE magma_sessions RENAME TO magma_sessions_legacy;
    ALTER INDEX IF EXISTS idx_magma_sessions_sub RENAME TO idx_magma_sessions_sub_legacy;
    ALTER INDEX IF EXISTS idx_magma_sessions_open RENAME TO idx_magma_sessions_open_legacy;

    CREATE TABLE magma_sessions (
        id                 UUID NOT NULL,
        business_id        UUID NOT NULL,
        subscription_id    UUID NOT NULL REFERENCES magma_subscriptions(id) ON DELETE CASCADE,
        subscriber_id      UUID NOT NULL,
        zone_id            UUID NOT NULL,
        device_id          UUID REFERENCES magma_devices(id) ON DELETE SET NULL,
        acct_session_id    VARCHAR(128) NOT NULL,
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
    ) PARTITION BY RANGE (started_at);

    -- Cover every month that already holds rows, plus two years forward.
    SELECT date_trunc('month', COALESCE(min(started_at), now()))::date
      INTO v_min FROM magma_sessions_legacy;
    PERFORM magma_ensure_session_partitions(
        v_min, (date_trunc('year', now()) + interval '2 year')::date);

    INSERT INTO magma_sessions SELECT * FROM magma_sessions_legacy;

    CREATE UNIQUE INDEX idx_magma_sessions_acct
        ON magma_sessions (acct_session_id, started_at);
    CREATE INDEX idx_magma_sessions_sub
        ON magma_sessions (subscription_id, started_at DESC);
    CREATE INDEX idx_magma_sessions_open
        ON magma_sessions (started_at) WHERE ended_at IS NULL;
    CREATE INDEX idx_magma_sessions_business
        ON magma_sessions (business_id, started_at DESC);

    DROP TABLE magma_sessions_legacy;
END;
$$;

-- ---------------------------------------------------------------------------
-- Retention. Keep 400 days of sessions and 180 days of audit/outbox noise;
-- FreeRADIUS owns its own radacct retention policy.
-- ---------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION magma_prune_old_rows(p_session_days INT DEFAULT 400,
                                                p_audit_days INT DEFAULT 180)
RETURNS TABLE (sessions_deleted BIGINT, audit_deleted BIGINT, outbox_deleted BIGINT)
LANGUAGE plpgsql
AS $$
DECLARE
    v_sessions BIGINT := 0;
    v_audit    BIGINT := 0;
    v_outbox   BIGINT := 0;
BEGIN
    IF EXISTS (SELECT 1 FROM pg_partitioned_table pt
               JOIN pg_class c ON c.oid = pt.partrelid
               WHERE c.relname = 'magma_sessions') THEN
        -- Dropping whole partitions is instant; create next month first.
        PERFORM magma_ensure_session_partitions(
            date_trunc('month', now())::date,
            (date_trunc('month', now()) + interval '2 month')::date);
        PERFORM magma_ensure_session_partitions(
            (date_trunc('month', now()) - interval '2 year')::date,
            (date_trunc('month', now()) - interval '1 month')::date);
        EXECUTE format('DROP TABLE IF EXISTS %I',
                       'magma_sessions_' || to_char(now() - (p_session_days || ' days')::interval,
                                                   'YYYY_MM'));
    ELSE
        DELETE FROM magma_sessions WHERE started_at < now() - (p_session_days || ' days')::interval;
        GET DIAGNOSTICS v_sessions = ROW_COUNT;
    END IF;

    DELETE FROM magma_audit_log WHERE created_at < now() - (p_audit_days || ' days')::interval;
    GET DIAGNOSTICS v_audit = ROW_COUNT;

    DELETE FROM magma_outbox
    WHERE status IN ('DONE','DEAD') AND created_at < now() - interval '30 days';
    GET DIAGNOSTICS v_outbox = ROW_COUNT;

    RETURN QUERY SELECT v_sessions, v_audit, v_outbox;
END;
$$;

CREATE OR REPLACE FUNCTION magma_close_stale_sessions(p_hours INT DEFAULT 12)
RETURNS INT
LANGUAGE plpgsql
AS $$
DECLARE
    closed INT := 0;
BEGIN
    UPDATE magma_sessions
    SET ended_at = COALESCE(last_interim_at, started_at),
        terminate_cause = COALESCE(terminate_cause, 'STALE')
    WHERE ended_at IS NULL AND started_at < now() - (p_hours || ' hours')::interval;
    GET DIAGNOSTICS closed = ROW_COUNT;
    RETURN closed;
END;
$$;

SELECT magma_ensure_session_partitions(
    (date_trunc('year', now()) - interval '1 year')::date,
    (date_trunc('year', now()) + interval '2 year')::date);

COMMIT;

-- Nightly cron suggestion (run outside the API):
--   psql "$DATABASE_URL" -c "SELECT * FROM magma_prune_old_rows();"
--   psql "$DATABASE_URL" -c "SELECT magma_close_stale_sessions();"
