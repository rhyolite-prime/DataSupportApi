-- MagmaNet — FreeRADIUS 3 schema (radius schema)
-- Owned by FreeRADIUS; DataSupportApi only inserts/updates credentials and reads accounting.
-- Configure FreeRADIUS: mods-enabled/sql -> postgresql { radius_db = "radius" }

BEGIN;

CREATE SCHEMA IF NOT EXISTS radius;

CREATE TABLE IF NOT EXISTS radius.nas (
    id          SERIAL PRIMARY KEY,
    nasname     VARCHAR(128) NOT NULL,
    shortname   VARCHAR(32) NOT NULL,
    type        VARCHAR(30) DEFAULT 'other',
    ports       INT,
    secret      VARCHAR(60) NOT NULL,
    server      VARCHAR(64),
    description VARCHAR(200)
);
CREATE INDEX IF NOT EXISTS idx_nas_nasname ON radius.nas (nasname);

CREATE TABLE IF NOT EXISTS radius.radcheck (
    id        SERIAL PRIMARY KEY,
    username  VARCHAR(64) NOT NULL DEFAULT '',
    attribute VARCHAR(64) NOT NULL DEFAULT '',
    op        VARCHAR(2) NOT NULL DEFAULT '==',
    value     VARCHAR(253) NOT NULL DEFAULT ''
);
CREATE INDEX IF NOT EXISTS idx_radcheck_username ON radius.radcheck (username);

CREATE TABLE IF NOT EXISTS radius.radreply (
    id        SERIAL PRIMARY KEY,
    username  VARCHAR(64) NOT NULL DEFAULT '',
    attribute VARCHAR(64) NOT NULL DEFAULT '',
    op        VARCHAR(2) NOT NULL DEFAULT '=',
    value     VARCHAR(253) NOT NULL DEFAULT ''
);
CREATE INDEX IF NOT EXISTS idx_radreply_username ON radius.radreply (username);

CREATE TABLE IF NOT EXISTS radius.radgroupcheck (
    id        SERIAL PRIMARY KEY,
    groupname VARCHAR(64) NOT NULL DEFAULT '',
    attribute VARCHAR(64) NOT NULL DEFAULT '',
    op        VARCHAR(2) NOT NULL DEFAULT '==',
    value     VARCHAR(253) NOT NULL DEFAULT ''
);
CREATE INDEX IF NOT EXISTS idx_radgroupcheck_group ON radius.radgroupcheck (groupname);

CREATE TABLE IF NOT EXISTS radius.radgroupreply (
    id        SERIAL PRIMARY KEY,
    groupname VARCHAR(64) NOT NULL DEFAULT '',
    attribute VARCHAR(64) NOT NULL DEFAULT '',
    op        VARCHAR(2) NOT NULL DEFAULT '=',
    value     VARCHAR(253) NOT NULL DEFAULT ''
);
CREATE INDEX IF NOT EXISTS idx_radgroupreply_group ON radius.radgroupreply (groupname);

CREATE TABLE IF NOT EXISTS radius.radusergroup (
    id        SERIAL PRIMARY KEY,
    username  VARCHAR(64) NOT NULL DEFAULT '',
    groupname VARCHAR(64) NOT NULL DEFAULT '',
    priority  INT NOT NULL DEFAULT 1
);
CREATE INDEX IF NOT EXISTS idx_radusergroup_username ON radius.radusergroup (username);

CREATE TABLE IF NOT EXISTS radius.radacct (
    radacctid           BIGSERIAL PRIMARY KEY,
    acctsessionid       VARCHAR(64) NOT NULL DEFAULT '',
    acctuniqueid        VARCHAR(32) NOT NULL DEFAULT '',
    username            VARCHAR(64) NOT NULL DEFAULT '',
    groupname           VARCHAR(64) NOT NULL DEFAULT '',
    realm               VARCHAR(64),
    nasipaddress        INET NOT NULL,
    nasportid           VARCHAR(32),
    nasporttype         VARCHAR(32),
    acctstarttime       TIMESTAMPTZ,
    acctupdatetime      TIMESTAMPTZ,
    acctstoptime        TIMESTAMPTZ,
    acctinterval        BIGINT,
    acctsessiontime     BIGINT,
    acctauthentic       VARCHAR(32),
    connectinfo_start   VARCHAR(128),
    connectinfo_stop    VARCHAR(128),
    acctinputoctets     BIGINT,
    acctoutputoctets    BIGINT,
    calledstationid     VARCHAR(50),
    callingstationid    VARCHAR(50),
    acctterminatecause  VARCHAR(32),
    servicetype         VARCHAR(32),
    framedprotocol      VARCHAR(32),
    framedipaddress     INET,
    framedipv6address   VARCHAR(45),
    framedipv6prefix    VARCHAR(45),
    framedinterfaceid   VARCHAR(44),
    delegatedipv6prefix VARCHAR(45),
    class               VARCHAR(64)
);
CREATE INDEX IF NOT EXISTS idx_radacct_user_start ON radius.radacct (username, acctstarttime DESC);
CREATE INDEX IF NOT EXISTS idx_radacct_open ON radius.radacct (acctstoptime) WHERE acctstoptime IS NULL;
CREATE INDEX IF NOT EXISTS idx_radacct_unique ON radius.radacct (acctuniqueid);

CREATE TABLE IF NOT EXISTS radius.radpostauth (
    id        BIGSERIAL PRIMARY KEY,
    username  VARCHAR(64) NOT NULL DEFAULT '',
    pass      VARCHAR(64) NOT NULL DEFAULT '',
    reply     VARCHAR(32) NOT NULL DEFAULT '',
    authdate  TIMESTAMPTZ NOT NULL DEFAULT now(),
    class     VARCHAR(64)
);
CREATE INDEX IF NOT EXISTS idx_radpostauth_user ON radius.radpostauth (username, authdate DESC);

COMMIT;
