#include <string>
#include <chrono>
#include <map>

#include <pqxx/pqxx>
#include <jwt/jwt.hpp>
#include <nlohmann/json.hpp>
#include "external/crow_all.h"

#include "utils.h"
#include "config.h"

using json = nlohmann::json;

enum role {
    Student,
    Admin
};

const std::map<std::string, role> ACTION_PERMS = {
    {"get_schedule", Student},
    {"update_schedule", Admin},
    {"start_admin_session", Admin}
};

const std::map<std::string, role> ROLE_MAP = {
    {"student", Student},
    {"admin", Admin}
};

void register_perm_handlers(crow::SimpleApp *app, pqxx::connection *db_conn) {
    // Main perm check method
    CROW_ROUTE((*app), "/auth_action").methods(crow::HTTPMethod::POST)([db_conn](const crow::request& req){
        json data;
        try {
            data = json::parse(req.body);
        } catch (...) {
            return crow::response(400, gen_error_json("bad_request", "Failed to parse body!"));
        }

        if (!data.contains("user_id") || !data.contains("action_type")) {
            return crow::response(400, gen_error_json("bad_request", "Wrong payload format!"));
        }

        std::string action = data["action_type"];
        role required_role;
        try {
            required_role = ACTION_PERMS.at(action);
        } catch (const std::out_of_range& e) {
            return crow::response(400, gen_error_json("unsupported_action", "Server doesn't support this action"));
        }

        bool allowed = false;
        std::string user_name;
        std::string group_name;
        pqxx::work db_work(*db_conn);
        int user_id = data["user_id"];
        try {
            pqxx::row row = db_work.exec1("SELECT name, group_name, roles FROM users WHERE id = '" + std::to_string(user_id) + "';");
            user_name = row[0].as<std::string>();
            if (!row[1].is_null()) {
                group_name = row[1].as<std::string>();
            }

            pqxx::array_parser role_arr = row[2].as_array();
            auto item = role_arr.get_next();
            while (item.first != pqxx::array_parser::juncture::done) {
                try {
                    if (item.first == pqxx::array_parser::juncture::string_value && ROLE_MAP.at(item.second) >= required_role) {
                        allowed = true;
                        break;
                    }
                } catch (std::out_of_range) {
                    CROW_LOG_WARNING << "DB contains unsupported role! (" << item.second << "}";
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
        if (allowed) {
            jwt::jwt_object token {
                jwt::params::algorithm("HS256"),
                jwt::params::secret(JWT_SECRET)
            };
            token.add_claim("sub", std::to_string(user_id));
            token.add_claim("name", user_name);
            token.add_claim("action", action);
            token.add_claim("exp", std::chrono::system_clock::now() + std::chrono::minutes{5});

            if (group_name != "") {
                token.add_claim("group", group_name);
            }
            
            resp["token"] = token.signature();
            return crow::response(resp.dump());
        } else {
            resp["token"] = nullptr;
            return crow::response(403, resp.dump());
        }
    });
}
