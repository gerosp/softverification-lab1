#include <deque>
#include "lab/cfg.hpp"
namespace lab
{
    CfgResult build_cfg(const ParsedMethod &method)
    {
        // Implement the pass here. Input: method.root(), method.root().field("body").
        // Return {std::move(graph), {}} once the CFG is built. See docs/cfg-pass.md.
        CfgResult cfg;

        cfg.graph = Graph{};
        Graph &g = cfg.graph.value();
        // if (!cfg.graph.has_value())
        //     return {
        //         std::nullopt, {{"No graph provided", "Empty graph was provided", method.root().range()}}};
        g.nodes.push_back({"start", "Start", "Start", SourceRange{0, 0}});
        g.nodes.push_back({"end", "End", "End", SourceRange{0, 0}});

        std::deque<SyntaxNode> q;
        std::deque<IfBlock> conditions;
        GraphNode previous_node = g.nodes[0];
        q.push_back(method.root());
        while (!q.empty())
        {
            SyntaxNode& node = q.front();
            q.pop_front();
            if (node.kind() == "local_declaration_statement")
            {
                GraphNode local_decl_node = GraphNode{"n" + std::to_string(g.nodes.size()), std::string(node.kind()), std::string(node.text()), node.range()};
                g.nodes.push_back(local_decl_node);
                g.edges.push_back({previous_node.id, local_decl_node.id, ""});
                // cfg.diagnostics.push_back(Diagnostic{"INFO","Assigned parent " + previous_node.id + "to " + local_decl_node.id ,node.range()});

                // TODO: Not intrested in this nodes children (no function calls YET) Recursion?
                if (previous_node.kind == "if_statement")
                {
                    conditions[0].end = local_decl_node;
                    g.edges.pop_back(); // delete the wrong condition->end edge
                }
                previous_node = local_decl_node;
            }
            else if (node.kind() == "expression_statement")
            {
                GraphNode expr_node = GraphNode{"n" + std::to_string(g.nodes.size()), std::string(node.kind()), std::string(node.text()), node.range()};
                g.nodes.push_back(expr_node);
                g.edges.push_back({previous_node.id, expr_node.id, ""});

                // Treat the same as a local decl
                if (previous_node.kind == "if_statement")
                {
                    conditions[0].end = expr_node;
                    g.edges.pop_back(); // delete the wrong condition->end edge
                }
                previous_node = expr_node;
            }
            else if (node.kind() == "return_statement")
            {
                GraphNode return_node = GraphNode{"n" + std::to_string(g.nodes.size()), std::string(node.kind()), std::string(node.text()), node.range()};
                g.nodes.push_back(return_node);
                g.edges.push_back({previous_node.id, return_node.id, ""});

                if (previous_node.kind == "if_statement")
                {
                    conditions[0].end = return_node;
                    g.edges.pop_back(); // delete the wrong condition->end edge
                }
                previous_node = return_node;
                g.edges.push_back({return_node.id, "end", "Return"});
            }
            else if (node.kind() == "if_statement")
            {
                // Add the condition block
                GraphNode phi_node = {"n" + std::to_string(g.nodes.size()), "if_statement", std::string(node.child(2).text()), node.child(2).range()};
                g.nodes.push_back(phi_node);
                g.edges.push_back({previous_node.id, phi_node.id, ""});
                // for (auto &child : node.children())
                // {
                //     q.push_back(child);
                // }
                SyntaxNode condition_node = node.field("condition");
                SyntaxNode consequence_node = node.field("consequence");
                SyntaxNode alternative_node = node.field("alternative");

                conditions.push_back(IfBlock{condition_node, consequence_node, alternative_node, phi_node, phi_node, IfState::CONDITION});

                // Add then branch child (not in q)
                // GraphNode then_node = {"n" + std::to_string(g.nodes.size()), std::string(consequence_node.kind()), std::string(consequence_node.text()), consequence_node.range()};
                // g.nodes.push_back(then_node);
                // g.edges.push_back({phi_node.id, then_node.id, "true"});
                // for (SyntaxNode &child : consequence_node.children())
                // {
                //     q.push_back(child);
                // }

                // // Add else branch child (not in q)
                // GraphNode else_node = {"n" + std::to_string(g.nodes.size()), std::string(alternative_node.kind()), std::string(alternative_node.text()), alternative_node.range()};
                // g.nodes.push_back(else_node);
                // g.edges.push_back({condition_node.id, else_node.id, "false"});
                // for (SyntaxNode &child : node.child(6).children())
                // {
                //     q.push_back(child);
                // }

                if (previous_node.kind == "if_statement")
                {
                    conditions[0].end = phi_node;
                    g.edges.pop_back(); // delete the wrong condition->end edge
                }
                previous_node = phi_node;
            }
            else
            {
                for (auto &child : node.children())
                {
                    q.push_back(child);
                }
                // for (std::uint32_t i = node.child_count() - 1; i > 0; --i)
                // {
                //     q.push_back(node.child(i));
                // };
                if (!conditions.empty())
                {
                    IfBlock condition = conditions.front();
                    switch (condition.state)
                    {
                    case IfState::CONDITION:
                        if (&node == &condition.consequence)
                        {
                            condition.state = IfState::CONSEQUENCE;
                            previous_node = condition.phi;
                        }

                        break;
                    case IfState::CONSEQUENCE:
                        if (&node == &condition.alternative)
                        {
                            condition.state = IfState::ALTERNATIVE;
                            previous_node = condition.phi;
                        }
                        break;
                    case IfState::ALTERNATIVE:
                        // TODO: end branch handling
                        break;
                    }
                }
            }
        }

        return cfg;
        // return {std::nullopt, {{"cfg_not_implemented", "CFG construction is not implemented yet.", method.root().range()}}};
    }
} // namespace lab
