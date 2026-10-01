#include "lab/graph.hpp"
#include <functional>
#include <sstream>
#include <unordered_set>
namespace lab {
namespace {
std::string dot_quote(std::string_view text) {
    std::string out = "\"";
    for (unsigned char c : text) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': break;
        case '\t': out += "    "; break;
        default: if (c >= 32) out += static_cast<char>(c); break;
        }
    }
    return out + '"';
}
bool visible(SyntaxNode n, std::string_view field) {
    static const std::unordered_set<std::string_view> operators{
        "+", "-", "*", "/", "%", "=", "+=", "-=", "*=", "/=", "%=", "++", "--",
        "==", "!=", "<", ">", "<=", ">=", "&&", "||", "!", "&", "|", "^", "~", "<<", ">>", "??", "?", ":"};
    return n.named() || n.missing() || field == "operator" || operators.contains(n.kind());
}
std::string snippet(std::string_view text) {
    if (text.size() <= 90) return std::string(text);
    std::size_t end = 90;
    while (end && (static_cast<unsigned char>(text[end]) & 0xc0) == 0x80) --end;
    return std::string(text.substr(0, end)) + "...";
}
}
Graph syntax_graph(const ParsedMethod& method) {
    Graph graph;
    std::function<std::string(SyntaxNode, unsigned)> visit = [&](SyntaxNode n, unsigned depth) {
        auto id = "n" + std::to_string(graph.nodes.size());
        auto label = std::string(n.kind());
        if (n.missing()) label += " [MISSING]";
        if (n.child_count() == 0 && n.text() != n.kind()) label += "\n" + snippet(n.text());
        if (depth >= max_tree_depth) label += " [depth limit]";
        graph.nodes.push_back({id, std::string(n.kind()), label, n.range()});
        if (depth < max_tree_depth) {
            for (std::uint32_t i = 0; i < n.child_count(); ++i) {
                auto c = n.child(i);
                auto field = n.child_field(i);
                if (visible(c, field)) {
                    auto child_id = visit(c, depth + 1);
                    graph.edges.push_back({id, child_id, std::string(field)});
                }
            }
        }
        return id;
    };
    visit(method.root(), 0);
    return graph;
}
std::string to_dot(const Graph& graph, std::string_view name) {
    std::ostringstream out;
    out << "digraph " << dot_quote(name) << " {\n  graph [rankdir=TB, bgcolor=\"transparent\", nodesep=0.25, ranksep=0.45];\n"
           "  node [shape=box, style=\"rounded,filled\", fillcolor=\"#eef2ff\", color=\"#818cf8\", fontname=\"Arial\", fontsize=11];\n"
           "  edge [color=\"#64748b\", fontname=\"Arial\", fontsize=10];\n";
    for (const auto& n : graph.nodes)
        out << "  " << dot_quote(n.id) << " [id=" << dot_quote(n.id) << ", label=" << dot_quote(n.label) << "];\n";
    for (const auto& e : graph.edges)
        out << "  " << dot_quote(e.from) << " -> " << dot_quote(e.to) << " [label=" << dot_quote(e.label) << "];\n";
    return out.str() + "}\n";
}
nlohmann::json syntax_json(SyntaxNode n, unsigned depth) {
    auto r = n.range();
    nlohmann::json out = {{"kind", n.kind()}, {"named", n.named()}, {"missing", n.missing()},
        {"error", n.error()}, {"range", {{"start", r.start}, {"end", r.end}}}, {"children", nlohmann::json::array()}};
    if (n.child_count() == 0) out["text"] = n.text();
    if (depth >= max_tree_depth) { out["truncated"] = true; return out; }
    for (std::uint32_t i = 0; i < n.child_count(); ++i) {
        auto child = syntax_json(n.child(i), depth + 1);
        child["field"] = n.child_field(i);
        out["children"].push_back(std::move(child));
    }
    return out;
}
std::string syntax_sexp(SyntaxNode n, unsigned depth) {
    auto out = "(" + std::string(n.kind());
    if (n.missing()) out += " MISSING";
    if (n.child_count() == 0) out += " " + nlohmann::json(n.text()).dump();
    if (depth >= max_tree_depth) return out + " ...)";
    for (std::uint32_t i = 0; i < n.child_count(); ++i) {
        if (!visible(n.child(i), n.child_field(i))) continue;
        out += " ";
        if (!n.child_field(i).empty()) out += std::string(n.child_field(i)) + ": ";
        out += syntax_sexp(n.child(i), depth + 1);
    }
    return out + ")";
}
} // namespace lab
