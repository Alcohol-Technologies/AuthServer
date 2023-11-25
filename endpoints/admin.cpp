#include <string>
#include <vector>
#include <format>

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include "external/crow_all.h"

#include "main.h"
#include "utils.h"

using json = nlohmann::json;

void register_admin_handlers(App *app, pqxx::connection *db_conn) {
    // Alter user roles
    CROW_ROUTE((*app), "/update_user_roles").methods(crow::HTTPMethod::POST)
    .CROW_MIDDLEWARES((*app), AdminSecMiddleware)([db_conn](const crow::request& req){
        json data;
        try {
            data = json::parse(req.body);
        } catch (...) {
            return crow::response(400, gen_error_json("bad_request", "Failed to parse body!"));
        }
        if (!data.contains("user_id")) {
            return crow::response(400, gen_error_json("bad_request", "User ID is missing!"));
        }
        if (!data.contains("roles") || data["roles"].size() == 0) {
            return crow::response(400, gen_error_json("bad_request", "Roles are missing!"));
        }

        // Update db
        long user_id = data["user_id"];
        std::string roles_str = "'{";
        for (auto &it : data["roles"].items()) {
            std::string cur_role = it.value();
            if (ROLE_MAP.count(cur_role) > 0) {
                roles_str += "\"" + cur_role + "\", ";
            } else {
                return crow::response(400, gen_error_json("bad_request", "Unsupported role: '" + cur_role + "'"));
            }
        }
        roles_str.resize(roles_str.length() - 2);
        roles_str += "}'";

        pqxx::work db_work(*db_conn);
        try {
            db_work.exec0(std::format("UPDATE users SET roles = {} WHERE id = {}", roles_str, user_id));
            db_work.commit();
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An error while using DB has occured!\n" << e.what();
            return crow::response(500);
        }

        json resp;
        resp["status"] = "ok";
        return crow::response(resp.dump());
    });

    // Alter user data
    CROW_ROUTE((*app), "/update_user_data").methods(crow::HTTPMethod::POST)
    .CROW_MIDDLEWARES((*app), AdminSecMiddleware)([db_conn](const crow::request& req){
        json data;
        try {
            data = json::parse(req.body);
        } catch (...) {
            return crow::response(400, gen_error_json("bad_request", "Failed to parse body!"));
        }
        if (!data.contains("user_id")) {
            return crow::response(400, gen_error_json("bad_request", "User ID is missing!"));
        }
        if (!data.contains("name") && !data.contains("group")) {
            return crow::response(400, gen_error_json("bad_request", "Name or group is required!"));
        }

        long user_id = data["user_id"];
        std::string sql_req = "UPDATE users SET ";
        if (data.contains("name")) {
            std::string name = data["name"];
            sql_req += std::format("name = '{}', ", name);
        }
        if (data.contains("group")) {
            std::string group = data["group"];
            sql_req += std::format("group_name = '{}'", group);
        }
        if (sql_req[sql_req.length() - 1] == ' ') {
            sql_req.resize(sql_req.length() - 2);
        }
        sql_req += std::format(" WHERE id = {}", user_id);

        pqxx::work db_work(*db_conn);
        try {
            // Check for user existance
            pqxx::row row = db_work.exec1(std::format("SELECT * FROM users WHERE id = '{}'", user_id));
            // Update data
            db_work.exec0(sql_req);
            db_work.commit();
        } catch (const pqxx::unexpected_rows& e) {
            return crow::response(400, gen_error_json("user_not_registered", "This user is not registered!"));
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An error while using DB has occured!\n" << e.what();
            return crow::response(500);
        }

        json resp;
        resp["status"] = "ok";
        return crow::response(resp.dump());
    });

    // Delete user
    CROW_ROUTE((*app), "/delete_user").methods(crow::HTTPMethod::POST)
    .CROW_MIDDLEWARES((*app), AdminSecMiddleware)([db_conn](const crow::request& req){
        json data;
        try {
            data = json::parse(req.body);
        } catch (...) {
            return crow::response(400, gen_error_json("bad_request", "Failed to parse body!"));
        }
        if (!data.contains("user_id")) {
            return crow::response(400, gen_error_json("bad_request", "User ID is missing!"));
        }

        long user_id = data["user_id"];
        pqxx::work db_work(*db_conn);
        try {
            // Check for user existance
            pqxx::row row = db_work.exec1(std::format("SELECT * FROM users WHERE id = '{}'", user_id));
            // Update data
            db_work.exec0(std::format("DELETE FROM users WHERE id = {}", user_id));
            db_work.commit();
        } catch (const pqxx::unexpected_rows& e) {
            return crow::response(400, gen_error_json("user_not_registered", "This user is not registered!"));
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An error while using DB has occured!\n" << e.what();
            return crow::response(500);
        }

        json resp;
        resp["status"] = "ok";
        return crow::response(resp.dump());
    });
}
