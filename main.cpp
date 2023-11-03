#include <functional>

#include <pqxx/pqxx>
#include "external/crow_all.h"

#include "config.h"
#include "endpoints/user_auth.h"

void (* HANDLERS[])(crow::SimpleApp*, pqxx::connection*) = {
    &register_gh_handlers
};

int main() {
    // Database
    pqxx::connection db_conn("postgresql:///cfuvAuthServer");
    pqxx::work db_work(db_conn);

    // Init tables
    db_work.exec0(
        "CREATE TABLE IF NOT EXISTS users ("
            "id         varchar PRIMARY KEY,"
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
    crow::SimpleApp app;

    // Register all declared handlers
    for (auto &it : HANDLERS) {
        it(&app, &db_conn);
    }

    app.port(LISTEN_PORT).run();
}
