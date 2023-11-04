#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string gen_error_json(std::string reason, std::string msg) {
    json j;
    j["status"] = "error";
    j["reason"] = reason;
    j["msg"] = msg;
    return j.dump();
}
