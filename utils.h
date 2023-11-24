enum role {
    Student,
    Admin
};

const std::map<std::string, role> ROLE_MAP = {
    {"student", Student},
    {"admin", Admin}
};

std::string gen_error_json(std::string reason, std::string msg);
