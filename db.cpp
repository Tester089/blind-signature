#include "db.h"

#include <cstdlib>
#include <mutex>
#include <memory>

#include <libpq-fe.h>

#include <iostream>


std::once_flag g_init_flag;
bool g_enabled = false;
bool g_ready = false;
std::string g_conninfo;
std::string g_init_error;

struct PgConnDeleter {
    void operator()(PGconn *conn) const noexcept {
        if (conn) PQfinish(conn);
    }
};

struct PgResultDeleter {
    void operator()(PGresult *res) const noexcept {
        if (res) PQclear(res);
    }
};

using PgConnPtr = std::unique_ptr<PGconn, PgConnDeleter>;
using PgResultPtr = std::unique_ptr<PGresult, PgResultDeleter>;

PgConnPtr Connect(std::string &error) {
    PgConnPtr conn(PQconnectdb(g_conninfo.c_str()));
    if (!conn || PQstatus(conn.get()) != CONNECTION_OK) {
        error = conn ? PQerrorMessage(conn.get()) : "PQconnectdb failed";
        return {};
    }
    return conn;
}

bool Exec(PGconn *conn, const char *sql, std::string &error) {
    PgResultPtr res(PQexec(conn, sql));
    if (!res) {
        error = PQerrorMessage(conn);
        return false;
    }
    auto status = PQresultStatus(res.get());
    bool ok = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);
    if (!ok) {
        error = PQerrorMessage(conn);
    }
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
    auto conn = Connect(error);
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

    if (!Exec(conn.get(), ext_sql, error) ||
        !Exec(conn.get(), users_sql, error) ||
        !Exec(conn.get(), users_alter_sql, error) ||
        !Exec(conn.get(), results_sql, error)) {
        g_ready = false;
        g_init_error = error;
        return;
    }

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


class UserStore {
public:
    virtual ~UserStore() = default;
    virtual bool CreateUser(const std::string &username, const std::string &password, bool &created, std::string &error) = 0;
    virtual bool ListUsers(std::vector<std::string> &users, std::string &error) = 0;
};

class PostgresUserStore : public UserStore {
public:
    bool CreateUser(const std::string &username, const std::string &password, bool &created, std::string &error) override {
        created = false;
        if (!DbReady(error)) {
            return false;
        }

        auto conn = Connect(error);
        if (!conn) {
            return false;
        }

        const char *params[2] = { username.c_str(), password.c_str() };
        PgResultPtr res(PQexecParams(conn.get(),
                                     "INSERT INTO users (username, password_hash) "
                                     "VALUES ($1, crypt($2, gen_salt('bf'))) "
                                     "ON CONFLICT (username) DO NOTHING",
                                     2,
                                     nullptr,
                                     params,
                                     nullptr,
                                     nullptr,
                                     0));
        if (!res) {
            error = PQerrorMessage(conn.get());
            return false;
        }

        if (PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
            error = PQerrorMessage(conn.get());
            return false;
        }

        const char *rows = PQcmdTuples(res.get());
        created = rows && std::atoi(rows) > 0;
        return true;
    }
    bool ListUsers(std::vector<std::string> &users, std::string &error) override {
        users.clear();
        if (!DbReady(error)) {
            return false;
        }

        auto conn = Connect(error);
        if (!conn) {
            return false;
        }

        PgResultPtr res(PQexec(conn.get(), "SELECT username FROM users ORDER BY username"));
        if (!res) {
            error = PQerrorMessage(conn.get());
            return false;
        }

        if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
            error = PQerrorMessage(conn.get());
            return false;
        }

        int rows = PQntuples(res.get());
        users.reserve(rows);
        for (int i = 0; i < rows; ++i) {
            char *value = PQgetvalue(res.get(), i, 0);
            if (value) users.emplace_back(value);
        }

        return true;
    }
};

class DisabledUserStore : public UserStore {
public:
    bool CreateUser(const std::string &username, const std::string &password, bool &created, std::string &error) override {
        (void)username;
        (void)password;
        created = false;
        error = "PostgreSQL is disabled (BS_DB_CONN is not set).";
        return false;
    }

    bool ListUsers(std::vector<std::string> &users, std::string &error) override {
        users.clear();
        error = "PostgreSQL is disabled (BS_DB_CONN is not set).";
        return false;
    }
};

UserStore &GetUserStore() {
    static std::once_flag flag;
    static std::unique_ptr<UserStore> store;
    std::call_once(flag, [] {
        const char *env = std::getenv("BS_DB_CONN");
        if (!env || !*env) {
            store = std::make_unique<DisabledUserStore>();
        } else {
            store = std::make_unique<PostgresUserStore>();
        }
    });
    return *store;
}
bool DbUserExists(const std::string &username, bool &exists, std::string &error) {
    exists = false;
    if (!DbReady(error)) {
        return false;
    }

    auto conn = Connect(error);
    if (!conn) {
        return false;
    }

    const char *params[1] = { username.c_str() };
    PgResultPtr res(PQexecParams(conn.get(),
                                 "SELECT 1 FROM users WHERE username = $1",
                                 1,
                                 nullptr,
                                 params,
                                 nullptr,
                                 nullptr,
                                 0));
    if (!res) {
        error = PQerrorMessage(conn.get());
        return false;
    }

    if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
        error = PQerrorMessage(conn.get());
        return false;
    }

    exists = PQntuples(res.get()) > 0;
    return true;
}

bool DbCheckUserPassword(const std::string &username, const std::string &password, bool &valid, std::string &error) {
    valid = false;
    if (!DbReady(error)) {
        return false;
    }

    auto conn = Connect(error);
    if (!conn) {
        return false;
    }

    const char *params[2] = { username.c_str(), password.c_str() };
    PgResultPtr res(PQexecParams(conn.get(),
                                 "SELECT 1 FROM users WHERE username = $1 AND password_hash = crypt($2, password_hash)",
                                 2,
                                 nullptr,
                                 params,
                                 nullptr,
                                 nullptr,
                                 0));
    if (!res) {
        error = PQerrorMessage(conn.get());
        return false;
    }

    if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
        error = PQerrorMessage(conn.get());
        return false;
    }

    valid = PQntuples(res.get()) > 0;
    return true;
}


bool DbCreateUser(const std::string &username, const std::string &password, bool &created, std::string &error) {
    return GetUserStore().CreateUser(username, password, created, error);
}

bool DbListUsers(std::vector<std::string> &users, std::string &error) {
    return GetUserStore().ListUsers(users, error);
}

bool DbRecordVote(const std::string &poll_id, const std::string &candidate, const std::string &username, std::string &error) {
    (void)username;
    if (!DbReady(error)) {
        return false;
    }

    auto conn = Connect(error);
    if (!conn) {
        return false;
    }

    const char *params[2] = { poll_id.c_str(), candidate.c_str() };
    PgResultPtr res(PQexecParams(conn.get(),
                                 "INSERT INTO poll_results (poll_id, candidate, votes, updated_at) "
                                 "VALUES ($1, $2, 1, NOW()) "
                                 "ON CONFLICT (poll_id, candidate) DO UPDATE "
                                 "SET votes = poll_results.votes + 1, updated_at = NOW()",
                                 2,
                                 nullptr,
                                 params,
                                 nullptr,
                                 nullptr,
                                 0));
    if (!res) {
        error = PQerrorMessage(conn.get());
        return false;
    }

    if (PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
        error = PQerrorMessage(conn.get());
        return false;
    }

    return true;
}
}
