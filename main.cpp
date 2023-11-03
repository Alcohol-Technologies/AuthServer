#include <string>
#include <random>
#include <format>

#include <pqxx/pqxx>
#include <cpr/cpr.h>
#include "crow_all.h"
#include "json.hpp"

#include "config.h"

using json = nlohmann::json;

const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

std::string gen_error_json(std::string reason, std::string msg) {
    json j;
    j["status"] = "error";
    j["reason"] = reason;
    j["msg"] = msg;
    return j.dump();
}

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

    // User register start
    CROW_ROUTE(app, "/start_register").methods(crow::HTTPMethod::POST)([&db_conn](const crow::request& req){
        json data;
        try {
            data = json::parse(req.body);
        } catch (...) {
            return crow::response(400, gen_error_json("bad_request", "Failed to parse body!"));
        }
        
        if (!data.contains("chat_id")) {
            return crow::response(400, gen_error_json("bad_request", "Chat ID is missing!"));
        }

        // Generate token
        std::random_device rd;
        std::default_random_engine generator(rd());
        std::uniform_int_distribution<int> distribution(0, 61);
        std::string token = "";
        for (int i = 0; i < 16; i++) {
            token += CHARACTERS[distribution(generator)];
        }

        // Save in db
        long chat_id = data["chat_id"];
        pqxx::work db_work(db_conn);
        try {
            db_work.exec0(std::format("INSERT INTO pending_reg VALUES ('{}', {})", token, chat_id));
            db_work.commit();
        } catch (const pqxx::unique_violation& e) {
            // Token was already requested for this chat
            // Update it
            db_work.commit();
            pqxx::work db_work(db_conn);
            db_work.exec0(std::format("UPDATE pending_reg SET token = '{}' WHERE chat_id = {}", token, chat_id));
            db_work.commit();
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An error while using DB has occured!\n" << e.what();
            return crow::response(500);
        }

        json resp;
        resp["status"] = "ok";
        resp["url"] = "https://github.com/login/oauth/authorize?client_id=" + std::string(OAUTH_APP_ID) + "&state=" + token;
        return crow::response(resp.dump());
    });

    // Request from GitHub
    CROW_ROUTE(app, "/finish_auth").methods(crow::HTTPMethod::GET)([&db_conn](const crow::request& req){
        if (req.url_params.get("error") != nullptr) {
            return crow::response(
                "Необходимо авторизоваться!\n"
                "Ну пожалуйста :(\n"
            );
        }
        std::string code = req.url_params.get("code");
        std::string state = req.url_params.get("state");

        // Check if token exists and delete from pending table
        pqxx::work db_work(db_conn);
        long chat_id;
        try {
            pqxx::row row = db_work.exec1("SELECT chat_id FROM pending_reg WHERE token = '" + state + "'");
            chat_id = row[0].as<long>();
            db_work.exec0("DELETE FROM pending_reg WHERE token = '" + state + "'");
        } catch (const pqxx::unexpected_rows& e) {
            return crow::response(400, "Invalid token!");
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An error while using DB has occured!\n" << e.what();
            return crow::response(500);
        }

        // Request user data
        cpr::Response token_r = cpr::Post(
            cpr::Url{"https://github.com/login/oauth/access_token"},
            cpr::Payload{
                {"client_id", OAUTH_APP_ID},
                {"client_secret", OAUTH_APP_SECRET},
                {"code", code}
            },
            cpr::Header{{"Accept", "application/json"}}
        );
        if (token_r.status_code != 200) {
            CROW_LOG_ERROR << "GitHub returned a non 200 response code!\n" << token_r.text;
            return crow::response("При взаимодействии с GitHub возникла ошибка!");
        }
        
        // Get token
        json token_parsed = json::parse(token_r.text);
        std::string access_token = token_parsed["access_token"];

        // Get user data
        cpr::Response user_r = cpr::Get(
            cpr::Url{"https://api.github.com/user"},
            cpr::Header{{"Authorization", "Bearer " + access_token}}
        );
        if (user_r.status_code != 200) {
            CROW_LOG_ERROR << "GitHub returned a non 200 response code!\n" << user_r.text;
            return crow::response("При взаимодействии с GitHub возникла ошибка!");
        }
        json parsed = json::parse(user_r.text);

        // Insert to the db
        try {
            db_work.exec0(std::format("INSERT INTO users VALUES ('{}', {}, '{{\"student\"}}', '{}', null)", parsed["id"].template get<long>(), chat_id, parsed["name"].template get<std::string>()));
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An error while using DB has occured!\n" << e.what();
            return crow::response(500);
        }
        db_work.commit();

        return crow::response("Авторизация прошла успешно!");
    });

    app.port(LISTEN_PORT).run();
}
