#include "lab/SSA.hpp"
#include "lab/cfg.hpp"
#include <cassert>
namespace lab
{
    // std::string get_next_id(const Graph &g) { return "n" + std::to_string(g.nodes.size()); }
    std::string get_next_id(const Graph &g); // TODO: This relies on cfg.hpp include, bad
    std::optional<GraphNode> find_node_in_graph(const Graph &g, const std::string &id)
    {
        for (const auto &n : g.nodes)
        {
            if (n.id == id)
            {
                return n;
            }
        }
        return std::nullopt;
    }
    std::optional<SyntaxNode> find_child1(const SyntaxNode &node, const std::string &kind)
    {
        for (const auto &ch : node.children())
        {
            if (ch.kind() == kind)
            {
                return ch;
            }
        }
        return std::nullopt;
    }
    std::string replace_identifiers(VariableRegistry &vr, const SyntaxNode &n)
    {
        std::string out;
        if (n.kind() == "binary_expression")
        {

            return replace_identifiers(vr, n.field("left")) + replace_identifiers(vr, n.field("operator")) + replace_identifiers(vr, n.field("right"));
        }
        if (n.kind() == "identifier")
        {
            return vr.get_variable(std::string(n.text())).to_string();
        }
        else if (n.kind() == "prefix_unary_expression")
        {
            return replace_identifiers(vr, n.child(0)) + replace_identifiers(vr, n.child(1));// TODO: better to use a loop here
        }
        else
        {
            return std::string(n.text());
        }
        return out;
    }
    void parse_arguments(VariableRegistry &vr, const SyntaxNode &n)
    {
        for (const auto &param : n.field("parameters").children())
        {
            vr.create_variable(std::string(param.field("name").text()), std::string(param.field("type").text()));
        }
    }

    SsaResult build_ssa(const ParsedMethod &method, const Graph &cfg)
    {
        SsaResult ssa;
        ssa.graph = Graph{};
        Graph &g = ssa.graph.value();
        VariableRegistry vr;
        parse_arguments(vr, method.root());
        GraphNode start;
        for (const auto &node : cfg.nodes)
        {
            if (node.id == "start")
            {
                start = node;
                break;
            }
        }

        std::function<NodeResult(GraphNode, NodeResult, unsigned)> visit = [&](GraphNode n, NodeResult p, unsigned depth)
        {
            if (n.kind == "local_declaration_statement")
            {

                std::string type;
                std::string name;
                std::string text;
                if (!n.syntax_node.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"Found declaration without syntax node", "", n.range.value_or(SourceRange{})});
                    return NodeResult{};
                }

                auto variable_declaration = find_child1(n.syntax_node.value(), "variable_declaration");
                if (!variable_declaration.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"Found declaration without syntax node", "No syntax node in variable_declaration", n.range.value_or(SourceRange{})});
                    return NodeResult{};
                }
                type = variable_declaration.value().field("type").text();

                auto variable_declarator = find_child1(variable_declaration.value(), "variable_declarator");
                if (!variable_declarator.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"Found declaration without syntax node", "No syntax node in variable_declarator", n.range.value_or(SourceRange{})});
                    return NodeResult{};
                }
                name = variable_declarator.value().field("name").text();
                auto rest = variable_declarator.value().child(2); // Assume name (0) = (1) *expr* (2)
                Variable var = vr.create_variable(name, type);

                // text = var.to_string() + " = " + std::string(rest.text());
                text = var.to_string() + " = " + replace_identifiers(vr, rest);
                std::string id = "n" + std::to_string(g.nodes.size());

                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id});
                }
                for (const auto &edge : cfg.edges)
                {
                    if (edge.from == n.id)
                    {

                        auto to = find_node_in_graph(cfg, edge.to);
                        if (to.has_value())
                        {
                            visit(to.value(), NodeResult{{ParentEdge{id}}, {}}, depth + 1);
                        }
                    }
                }
            }
            else if (n.kind == "expression_statement")
            {
                if (!n.syntax_node.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"ssa_no_syntax", "Found expression without syntax node", n.range.value_or(SourceRange{})});
                    return NodeResult{};
                }
                std::optional<SyntaxNode> assignment_expression = find_child1(n.syntax_node.value(), "assignment_expression");
                if (!assignment_expression.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"ssa_no_syntax", "Found assignment  without syntax node", n.range.value_or(SourceRange{})});
                    return NodeResult{};
                }
                // Assumes left is identifier
                std::string dest = std::string(assignment_expression.value().field("left").text());
                std::string text;
                std::string right = replace_identifiers(vr, assignment_expression.value().field("right"));
                text = vr.get_and_inc_variable(dest).to_string() + "=" + right;
                std::string id = "n" + std::to_string(g.nodes.size());
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id});
                }
                for (const auto &edge : cfg.edges)
                {
                    if (edge.from == n.id)
                    {

                        auto to = find_node_in_graph(cfg, edge.to);
                        if (to.has_value())
                        {
                            visit(to.value(), NodeResult{{ParentEdge{id}}, {}}, depth + 1);
                        }
                    }
                }
            }
            else // Non terminal nodes
            {

                for (const auto &edge : cfg.edges)
                {
                    if (edge.from == n.id)
                    {

                        auto to = find_node_in_graph(cfg, edge.to);
                        if (to.has_value())
                        {
                            // ssa.diagnostics.push_back(Diagnostic{"Found next node:", to.value().kind + "", n.range.value_or(SourceRange{})});
                            visit(to.value(), p, depth + 1);
                        }
                    }
                }
                // TODO: check graph edges to detect which node to go to next
                // NodeResult prev_id = p;
                // std::vector<LoopControl> lcs;
                // std::vector<std::string> outs;
                // for (SyntaxNode &child : n.outs)
                // {
                //     prev_id = visit(child, NodeResult{prev_id.outs, {}}, depth + 1);
                //     for (const auto &lc : prev_id.lc)
                //     {
                //         lcs.push_back(lc);
                //     }
                // }
                // return NodeResult{prev_id.outs, lcs};
            }

            return p;
            // return NodeResult{};
        };

        visit(start, NodeResult{}, 0);
        // ssa.diagnostics.push_back(Diagnostic{"", "Graph Size:" + std::to_string(ssa.graph.value().nodes.size()), SourceRange{}});

        // ssa.graph = cfg;
        // TODO: implement SSA on top of the method's control-flow graph.

        return ssa;
        // return {std::nullopt, {{"ssa_not_implemented", "SSA construction is not implemented yet.", {0, 0}}}};
    }

    Variable VariableRegistry::create_variable(std::string name, std::string type)
    {
        Variable new_var{0, name, type};
        variables[name] = new_var;
        return new_var;
    }
    Variable VariableRegistry::get_variable(std::string name)
    {
        return variables[name];
    }
    std::size_t VariableRegistry::get_version(std::string name)
    {
        return variables[name].version;
    }
    Variable VariableRegistry::get_and_inc_variable(std::string name)
    {
        variables[name].version++;
        return variables[name];
    }

} // namespace lab
