#pragma once
#include "lab/syntax.hpp"
#include <nlohmann/json.hpp>
namespace lab {
struct GraphNode {
    std::string id;
    std::string kind; // Use "condition" for a diamond in DOT output.
    std::string label;
    std::optional<SourceRange> range;
};
enum class GraphEdgeColor { Default, Green, Red, Orange, Blue };
struct GraphEdge {
    std::string from;
    std::string to;
    std::string label;
    GraphEdgeColor color = GraphEdgeColor::Default;
};
struct Graph { std::vector<GraphNode> nodes; std::vector<GraphEdge> edges; };
Graph syntax_graph(const ParsedMethod& method);
std::string to_dot(const Graph& graph, std::string_view name = "Syntax");
nlohmann::json syntax_json(SyntaxNode node, unsigned depth = 0);
std::string syntax_sexp(SyntaxNode node, unsigned depth = 0);
} // namespace lab
