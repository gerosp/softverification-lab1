#include "lab/protocol.hpp"
#include "lab/cfg.hpp"
#include "lab/SSA.hpp"
namespace lab {
nlohmann::json error_response(nlohmann::json id, std::string code, std::string message) {
    return {{"protocolVersion", 1}, {"requestId", std::move(id)}, {"status", "error"},
        {"diagnostics", nlohmann::json::array({{{"code", std::move(code)}, {"message", std::move(message)}, {"range", {{"start", 0}, {"end", 0}}}}})},
        {"dot", nullptr}, {"nodeRanges", nlohmann::json::object()}};
}
nlohmann::json analyze_response(std::string source, nlohmann::json id, bool include_tree, std::string_view view) {
    if (view != "syntax" && view != "cfg" && view != "ssa")
        return error_response(id, "unsupported_view", "Supported views: 'syntax' (AST), 'cfg', 'ssa'.");
    auto result = parse_method(std::move(source));
    nlohmann::json out = {{"protocolVersion", 1}, {"requestId", std::move(id)}, {"status", result.ok() ? "ok" : "syntax_error"},
        {"diagnostics", nlohmann::json::array()}, {"dot", nullptr}, {"nodeRanges", nlohmann::json::object()}};
    for (const auto& d : result.diagnostics)
        out["diagnostics"].push_back({{"code", d.code}, {"message", d.message}, {"range", {{"start", d.range.start}, {"end", d.range.end}}}});
    if (include_tree) out["tree"] = result.method ? syntax_json(result.method->root()) : nlohmann::json(nullptr);
    std::optional<Graph> graph;
    if (view == "syntax" && result.method) {
        graph = syntax_graph(*result.method);
    } else if ((view == "cfg" || view == "ssa") && result.ok()) {
        auto cfg = build_cfg(*result.method);
        if (!cfg.diagnostics.empty() || !cfg.graph) {
            out["status"] = "error";
            for (const auto& d : cfg.diagnostics)
                out["diagnostics"].push_back({{"code", d.code}, {"message", d.message}, {"range", {{"start", d.range.start}, {"end", d.range.end}}}});
            if (cfg.diagnostics.empty())
                out["diagnostics"].push_back({{"code", "cfg_no_graph"}, {"message", "CFG pass returned neither a graph nor diagnostics."},
                    {"range", {{"start", 0}, {"end", 0}}}});
        } else {
            graph = std::move(cfg.graph);
        }
    }
    if (view == "ssa" && graph) {
        auto ssa = build_ssa(*result.method, *graph);
        graph.reset();
        if (!ssa.diagnostics.empty() || !ssa.graph) {
            out["status"] = "error";
            for (const auto& d : ssa.diagnostics)
                out["diagnostics"].push_back({{"code", d.code}, {"message", d.message}, {"range", {{"start", d.range.start}, {"end", d.range.end}}}});
            if (ssa.diagnostics.empty())
                out["diagnostics"].push_back({{"code", "ssa_no_graph"}, {"message", "SSA pass returned neither a graph nor diagnostics."},
                    {"range", {{"start", 0}, {"end", 0}}}});
        } else {
            graph = std::move(ssa.graph);
        }
    }
    // Invalid syntax may be displayed as a partial AST, but never passed to CFG/SSA.
    if (graph) {
        out["dot"] = to_dot(*graph, view == "cfg" ? "CFG" : view == "ssa" ? "SSA" : "Syntax");
        for (const auto& n : graph->nodes)
            if (n.range) out["nodeRanges"][n.id] = {{"start", n.range->start}, {"end", n.range->end}};
    }
    return out;
}
nlohmann::json handle_request(const nlohmann::json& request) {
    auto id = request.is_object() ? request.value("requestId", nlohmann::json(nullptr)) : nlohmann::json(nullptr);
    if (!request.is_object() || !request.contains("protocolVersion") || request["protocolVersion"] != 1 ||
        !id.is_number_integer() || !request.contains("source") || !request["source"].is_string() ||
        !request.contains("view") || !request["view"].is_string())
        return error_response(id, "invalid_request", "Expected protocolVersion: 1, integer requestId, string source and string view.");
    return analyze_response(request["source"].get<std::string>(), id, false, request["view"].get<std::string>());
}
int response_exit_code(const nlohmann::json& response) {
    auto status = response.at("status").get<std::string>();
    return status == "ok" ? 0 : status == "syntax_error" ? 1 : 2;
}
}
