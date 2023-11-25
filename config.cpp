#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

#include "config.h"

using json = nlohmann::json;

ConfigWorker::ConfigWorker(std::string file_path) {
    std::ifstream f(file_path);
    json data = json::parse(f);
    assert(data.contains("oauth") && data.contains("net") && data.contains("secrets"));
    // OAuth
    assert(data["oauth"].contains("appId") && data["oauth"].contains("appSecret"));
    oauth_app_id_ = data["oauth"]["appId"];
    oauth_app_secret_ = data["oauth"]["appSecret"];

    // Network
    assert(data["net"].contains("port"));
    listen_port_ = data["net"]["port"];

    // Secrets
    assert(data["secrets"].contains("jwt") && data["secrets"].contains("api") && data["secrets"].contains("apiAdmin"));
    api_secret_ = data["secrets"]["api"];
    api_admin_secret_ = data["secrets"]["apiAdmin"];
    jwt_secret_ = data["secrets"]["jwt"];
}

