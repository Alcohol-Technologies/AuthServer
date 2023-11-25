#define CONFIG_PATH "./config.json"

class ConfigWorker {
private:
    std::string oauth_app_id_, oauth_app_secret_;
    int listen_port_;
    std::string api_secret_, api_admin_secret_, jwt_secret_;

public:
    ConfigWorker(std::string file_path=CONFIG_PATH);
    std::string oauth_app_id() const {
        return oauth_app_id_;
    }
    std::string oauth_app_secret() const {
        return oauth_app_secret_;
    }
    int listen_port() const {
        return listen_port_;
    }
    std::string api_secret() const {
        return api_secret_;
    }
    std::string api_admin_secret() const {
        return api_admin_secret_;
    }
    std::string jwt_secret() const {
        return jwt_secret_;
    }
};
