#pragma once
#include "lab/graph.hpp"
namespace lab {
nlohmann::json analyze_response(std::string source, nlohmann::json request_id,
    bool include_tree = false, std::string_view view = "syntax");
nlohmann::json error_response(nlohmann::json request_id, std::string code, std::string message);
nlohmann::json handle_request(const nlohmann::json& request);
int response_exit_code(const nlohmann::json& response);
}
