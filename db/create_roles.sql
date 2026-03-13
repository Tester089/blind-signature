-- you need admin access to run this file

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS users (
    username TEXT PRIMARY KEY,
    password_hash TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS poll_results (
    poll_id TEXT NOT NULL,
    candidate TEXT NOT NULL,
    votes INTEGER NOT NULL DEFAULT 0,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (poll_id, candidate)
);

DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'blind_app') THEN
        CREATE ROLE blind_app LOGIN PASSWORD 'change_me_app';
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'blind_admin') THEN
        CREATE ROLE blind_admin LOGIN PASSWORD 'change_me_admin';
    END IF;
END $$;

DO $$
DECLARE dbname TEXT := current_database();
BEGIN
    EXECUTE format('GRANT CONNECT ON DATABASE %I TO blind_app, blind_admin', dbname);
END $$;

GRANT USAGE ON SCHEMA public TO blind_app, blind_admin;

GRANT SELECT, INSERT ON TABLE users TO blind_app;
GRANT INSERT, UPDATE ON TABLE poll_results TO blind_app;

GRANT SELECT, INSERT, UPDATE ON TABLE users TO blind_admin;
GRANT SELECT, INSERT, UPDATE ON TABLE poll_results TO blind_admin;
