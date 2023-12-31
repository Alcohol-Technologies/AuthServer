struct SecurityMiddleware {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context& ctx);
    void after_handle(crow::request& req, crow::response& res, context& ctx);
};

struct AdminSecMiddleware : crow::ILocalMiddleware {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context& ctx);
    void after_handle(crow::request& req, crow::response& res, context& ctx);
};

typedef crow::App<SecurityMiddleware, AdminSecMiddleware> App;
