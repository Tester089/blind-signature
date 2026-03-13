#include "db.h"

#include <cstdlib>
#include <mutex>

#include <libpq-fe.h>

#include <iostream>

// namespace {

std::once_flag g_init_flag;
bool g_enabled = false;
bool g_ready = false;
std::string g_conninfo;
std::string g_init_error;

PGconn *Connect(std::string &error) {
    PGconn *conn = PQconnectdb(g_conninfo.c_str());
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        error = conn ? PQerrorMessage(conn) : "PQconnectdb failed";
        if (conn) PQfinish(conn);
        return nullptr;
    }
    return conn;
}

bool Exec(PGconn *conn, const char *sql, std::string &error) {
    PGresult *res = PQexec(conn, sql);
    if (!res) {
        error = PQerrorMessage(conn);
        return false;
    }
    auto status = PQresultStatus(res);
    bool ok = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);
    if (!ok) {
        error = PQerrorMessage(conn);
    }
    PQclear(res);
    return ok;
}

void InitOnce() {
    const char *env = std::getenv("BS_DB_CONN");
    std::cout << "BS_DB_CONN: " << (env ? env : "null") << std::endl;
    if (!env || !*env) {
        g_enabled = false;
        g_ready = false;
        return;
    }

    g_enabled = true;
    g_conninfo = env;

    std::string error;
    PGconn *conn = Connect(error);
    if (!conn) {
        g_ready = false;
        g_init_error = error;
        return;
    }

    const char *ext_sql = "CREATE EXTENSION IF NOT EXISTS pgcrypto;";

    const char *users_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "username TEXT PRIMARY KEY,"
        "password_hash TEXT,"
        "created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()"
        ");";

    const char *users_alter_sql =
        "ALTER TABLE users ADD COLUMN IF NOT EXISTS password_hash TEXT;";

    const char *results_sql =
        "CREATE TABLE IF NOT EXISTS poll_results ("
        "poll_id TEXT NOT NULL,"
        "candidate TEXT NOT NULL,"
        "votes INTEGER NOT NULL DEFAULT 0,"
        "updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),"
        "PRIMARY KEY (poll_id, candidate)"
        ");";

    if (!Exec(conn, ext_sql, error) ||
        !Exec(conn, users_sql, error) ||
        !Exec(conn, users_alter_sql, error) ||
        !Exec(conn, results_sql, error)) {
        g_ready = false;
        g_init_error = error;
        PQfinish(conn);
        return;
    }

    PQfinish(conn);
    g_ready = true;
}

bool DbEnabled() {
    std::call_once(g_init_flag, InitOnce);
    return g_enabled;
}

bool DbReady(std::string &error) {
    std::call_once(g_init_flag, InitOnce);
    if (!g_enabled) {
        error = "PostgreSQL is disabled (BS_DB_CONN is not set).";
        return false;
    }
    if (!g_ready) {
        error = g_init_error.empty() ? "PostgreSQL init failed." : g_init_error;
        return false;
    }
    return true;
}

bool DbUserExists(const std::string &username, bool &exists, std::string &error) {
    exists = false;
    if (!DbReady(error)) {
        return false;
    }

    PGconn *conn = Connect(error);
    if (!conn) {
        return false;
    }

    const char *params[1] = { username.c_str() };
    PGresult *res = PQexecParams(conn,
                                 "SELECT 1 FROM users WHERE username = $1",
                                 1,
                                 nullptr,
                                 params,
                                 nullptr,
                                 nullptr,
                                 0);
    if (!res) {
        error = PQerrorMessage(conn);
        PQfinish(conn);
        return false;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        error = PQerrorMessage(conn);
        PQclear(res);
        PQfinish(conn);
        return false;
    }

    exists = PQntuples(res) > 0;
    PQclear(res);
    PQfinish(conn);
    return true;
}

bool DbCheckUserPassword(const std::string &username, const std::string &password, bool &valid, std::string &error) {
    valid = false;
    if (!DbReady(error)) {
        return false;
    }

    PGconn *conn = Connect(error);
    if (!conn) {
        return false;
    }

    const char *params[2] = { username.c_str(), password.c_str() };
    PGresult *res = PQexecParams(conn,
                                 "SELECT 1 FROM users WHERE username = $1 AND password_hash = crypt($2, password_hash)",
                                 2,
                                 nullptr,
                                 params,
                                 nullptr,
                                 nullptr,
                                 0);
    if (!res) {
        error = PQerrorMessage(conn);
        PQfinish(conn);
        return false;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        error = PQerrorMessage(conn);
        PQclear(res);
        PQfinish(conn);
        return false;
    }

    valid = PQntuples(res) > 0;
    PQclear(res);
    PQfinish(conn);
    return true;
}

bool DbRecordVote(const std::string &poll_id, const std::string &candidate, const std::string &username, std::string &error) {
    (void)username;
    if (!DbReady(error)) {
        return false;
    }

    PGconn *conn = Connect(error);
    if (!conn) {
        return false;
    }

    const char *params[2] = { poll_id.c_str(), candidate.c_str() };
    PGresult *res = PQexecParams(conn,
                                 "INSERT INTO poll_results (poll_id, candidate, votes, updated_at) "
                                 "VALUES ($1, $2, 1, NOW()) "
                                 "ON CONFLICT (poll_id, candidate) DO UPDATE "
                                 "SET votes = poll_results.votes + 1, updated_at = NOW()",
                                 2,
                                 nullptr,
                                 params,
                                 nullptr,
                                 nullptr,
                                 0);
    if (!res) {
        error = PQerrorMessage(conn);
        PQfinish(conn);
        return false;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        error = PQerrorMessage(conn);
        PQclear(res);
        PQfinish(conn);
        return false;
    }

    PQclear(res);
    PQfinish(conn);
    return true;
}
