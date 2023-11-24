#include <string>
#include <chrono>
#include <map>
#include <vector>

#include <pqxx/pqxx>
#include <jwt/jwt.hpp>
#include <nlohmann/json.hpp>
#include "external/crow_all.h"

#include "utils.h"
#include "config.h"

using json = nlohmann::json;

void register_info_handlers(crow::SimpleApp *app, pqxx::connection *db_conn) {
    // User info
    CROW_ROUTE((*app), "/user_info").methods(crow::HTTPMethod::GET)([db_conn](const crow::request& req){
        std::string user_id;
        if (req.url_params.get("user_id") != nullptr) {
            user_id = req.url_params.get("user_id");
        }
        if (user_id.empty()) {
            return crow::response(400, gen_error_json("bad_request", "User ID is missing!"));
        }

        std::string user_name;
        std::string group_name;
        std::vector<std::string> roles;
        pqxx::work db_work(*db_conn);
        try {
            pqxx::row row = db_work.exec1("SELECT name, group_name, roles FROM users WHERE id = '" + user_id + "';");
            user_name = row[0].as<std::string>();
            if (!row[1].is_null()) {
                group_name = row[1].as<std::string>();
            }
            pqxx::array_parser role_arr = row[2].as_array();
            auto item = role_arr.get_next();
            while (item.first != pqxx::array_parser::juncture::done) {
                if (item.first == pqxx::array_parser::juncture::string_value) {
                    roles.push_back(item.second);
                }
                item = role_arr.get_next();
            }
        } catch (const pqxx::unexpected_rows) {
            return crow::response(400, gen_error_json("user_not_registered", "This user is not registered!"));
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An error while using DB has occured!\n" << e.what();
            return crow::response(500);
        }

        json resp;
        resp["status"] = "ok";
        resp["roles"] = roles;
        resp["name"] = user_name;
        resp["group"] = group_name;
        return crow::response(resp.dump());
    });

    // User roles
    CROW_ROUTE((*app), "/user_roles").methods(crow::HTTPMethod::GET)([db_conn](const crow::request& req){
        std::string user_id;
        if (req.url_params.get("user_id") != nullptr) {
            user_id = req.url_params.get("user_id");
        }
        if (user_id.empty()) {
            return crow::response(400, gen_error_json("bad_request", "User ID is missing!"));
        }

        std::vector<std::string> roles;
        pqxx::work db_work(*db_conn);
        try {
            pqxx::row row = db_work.exec1("SELECT roles FROM users WHERE id = '" + user_id + "';");
            pqxx::array_parser role_arr = row[0].as_array();
            auto item = role_arr.get_next();
            while (item.first != pqxx::array_parser::juncture::done) {
                if (item.first == pqxx::array_parser::juncture::string_value) {
                    roles.push_back(item.second);
                }
                item = role_arr.get_next();
            }
        } catch (const pqxx::unexpected_rows) {
            return crow::response(400, gen_error_json("user_not_registered", "This user is not registered!"));
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An error while using DB has occured!\n" << e.what();
            return crow::response(500);
        }

        json resp;
        resp["status"] = "ok";
        resp["roles"] = roles;
        return crow::response(resp.dump());
    });
}
