#include <functional>

#include <pqxx/pqxx>
#include "external/crow_all.h"

#include "main.h"
#include "utils.h"
#include "config.h"
#include "endpoints/user_auth.h"
#include "endpoints/perm_control.h"
#include "endpoints/user_info.h"
#include "endpoints/admin.h"

ConfigWorker CONFIG = ConfigWorker();

void (* HANDLERS[])(App*, pqxx::connection*) = {
    &register_gh_handlers,
    &register_perm_handlers,
    &register_info_handlers,
    &register_admin_handlers
};

const std::string INSECURE_PATHS[] = {
    "/finish_auth"
};

// Security middlewares
void SecurityMiddleware::before_handle(crow::request& req, crow::response& res, context& ctx) {
    auto it = req.headers.find("Security-Token");
    if (INSECURE_PATHS->find(req.url) != std::string::npos) {
        return;
    }
    if (it == req.headers.end() || (it->second != CONFIG.api_secret() && it->second != CONFIG.api_admin_secret())) {
        res.code = 401;
        res.body = gen_error_json("unauthorized", "Unauthorized");
        res.end();
        CROW_LOG_INFO << "Auth attempt fail! Got token " << (it == req.headers.end() ? "null" : it->second);
    }
}
void SecurityMiddleware::after_handle(crow::request& req, crow::response& res, context& ctx) {}

void AdminSecMiddleware::before_handle(crow::request& req, crow::response& res, context& ctx) {
    auto it = req.headers.find("Security-Token");
    if (it->second != CONFIG.api_admin_secret()) {
        res.code = 401;
        res.body = gen_error_json("unauthorized", "Unauthorized");
        res.end();
        CROW_LOG_INFO << "Auth for admin endpoint attempt fail! Got token " << it->second;
    }
}
void AdminSecMiddleware::after_handle(crow::request& req, crow::response& res, context& ctx) {}

int main() {
    // Database
    pqxx::connection db_conn("postgresql:///cfuvAuthServer");
    pqxx::work db_work(db_conn);

    // Init tables
    db_work.exec0(
        "CREATE TABLE IF NOT EXISTS users ("
            "id         int PRIMARY KEY,"
            "chat_id    bigint UNIQUE NOT NULL,"
            "roles      varchar[] NOT NULL,"
            "name       varchar,"
            "group_name varchar"
        ")"
    );

    db_work.exec0(
        "CREATE TABLE IF NOT EXISTS pending_reg ("
            "token   varchar PRIMARY KEY,"
            "chat_id bigint UNIQUE NOT NULL"
        ")"
    );

    db_work.commit();

    // Crow HTTP framework
    App app;

    // Register all declared handlers
    for (auto &it : HANDLERS) {
        it(&app, &db_conn);
    }

    app.port(CONFIG.listen_port()).run();
}
