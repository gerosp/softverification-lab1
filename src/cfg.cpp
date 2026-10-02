#include <deque>
#include "lab/cfg.hpp"
namespace lab
{
    std::string get_next_id(const Graph &g) { return "n" + std::to_string(g.nodes.size()); }

    GraphNode create_simple_node(Graph &g, const SyntaxNode &node, const std::vector<std::string> &parent, const std::string id)
    {
        GraphNode g_node = GraphNode{id, std::string(node.kind()), std::string(node.text()), node.range()};
        g.nodes.push_back(g_node);
        for (const auto &p : parent)
        {
            g.edges.push_back({p, g_node.id, ""});
        }
        return g_node;
    }

    CfgResult build_cfg(const ParsedMethod &method)
    {
        CfgResult cfg;
        cfg.graph = Graph{};
        Graph &g = cfg.graph.value();
        g.nodes.push_back({"start", "Start", "Start", SourceRange{0, 0}});
        g.nodes.push_back({"end", "End", "End", SourceRange{0, 0}});

        std::function<std::vector<std::string>(SyntaxNode, std::vector<std::string>, unsigned)> visit = [&](SyntaxNode n, std::vector<std::string> p, unsigned depth)
        {
            // auto id = "n" + std::to_string(g.nodes.size());
            auto id = get_next_id(g);

            auto kind = std::string(n.kind());

            if (n.kind() == "local_declaration_statement" || n.kind() == "expression_statement")
            {
                create_simple_node(g, n, p, id);
                return std::vector{id};
            }
            else if (n.kind() == "return_statement")
            {
                GraphNode node = create_simple_node(g, n, p, id);
                g.edges.push_back({node.id, "end", "return"});
                std::vector<std::string> out;
                return out;
            }
            else if (n.kind() == "if_statement")
            {
                SyntaxNode condition_node = n.field("condition");
                GraphNode g_cond = create_simple_node(g, condition_node, p, id);
                SyntaxNode consequence_node = n.field("consequence");
                SyntaxNode alternative_node = n.field("alternative");
                // g.edges.push_back(GraphEdge{g_cond.id, visit(consequence_node, id, depth + 1), "true"});
                // g.edges.push_back(GraphEdge{g_cond.id, visit(alternative_node, id, depth + 1), "false"});
                std::vector<std::string> cons_node_id = visit(consequence_node, std::vector{id}, depth + 1);
                std::vector<std::string> alt_node_id = visit(alternative_node, std::vector{id}, depth + 1);
                // cfg.diagnostics.push_back(Diagnostic{"cons_node_id: ",cons_node_id,n.range()});
                std::vector<std::string> out;
                for (auto &s : cons_node_id)
                {
                    out.push_back(s);
                }
                for (auto &s : alt_node_id)
                {
                    out.push_back(s);
                }
                return out;
            }
            else if (n.kind() == "while_statement")
            {
                SyntaxNode condition_node = n.field("condition");
                GraphNode g_cond = create_simple_node(g, condition_node, p, id);
                SyntaxNode body_node = n.field("body");
                // std::vector<std::string> cond_node_id = visit(condition_node, std::vector{id}, depth + 1);
                std::vector<std::string> body_node_id = visit(body_node, std::vector{id}, depth + 1);
                for (const auto &b : body_node_id)
                {
                    g.edges.push_back(GraphEdge{b, id, "loop"}); // TODO: do every node, not just [0]
                }
                // g.edges.push_back(GraphEdge{body_node_id[0], id, "loop"}); // TODO: do every node, not just [0]
                std::vector<std::string> out;
                // for (auto &s : cond_node_id)
                // {
                //     out.push_back(s);
                // }
                // for (auto &s : body_node_id)
                // {
                //     out.push_back(s);
                // }
                out.push_back(g_cond.id);
                return out;
            }
            else if (n.kind() == "for_statement")
            {
                SyntaxNode init_node = n.field("initializer");
                GraphNode g_init = create_simple_node(g, init_node, p, id);
                SyntaxNode condition_node = n.field("condition");
                GraphNode g_cond = create_simple_node(g, condition_node, {g_init.id}, get_next_id(g));

                SyntaxNode body_node = n.field("body");
                std::vector<std::string> body_node_id = visit(body_node, std::vector{g_cond.id}, depth + 1);
                SyntaxNode update_node = n.field("update");
 
                
                GraphNode g_update = create_simple_node(g, update_node, body_node_id, get_next_id(g));
                g.edges.push_back(GraphEdge{g_update.id, g_cond.id, "loop"});

                // for (const auto &b : body_node_id)// Create an update block for every branch
                // {

                //     GraphNode g_update = create_simple_node(g, update_node, {b}, get_next_id(g));
                //     g.edges.push_back(GraphEdge{g_update.id, g_cond.id, "loop"});
                // }

                return std::vector<std::string>{g_cond.id};
            }
            else // Non terminal nodes
            {
                std::vector<std::string> prev_id = p;
                for (SyntaxNode &child : n.children())
                {
                    prev_id = visit(child, prev_id, depth + 1);
                }
                return prev_id;
            }

            return p;
        };
        visit(method.root(), {"start"}, 0);
        return cfg;
    }

    // CfgResult
    // build_cfg__(const ParsedMethod &method)
    // {
    //     // Implement the pass here. Input: method.root(), method.root().field("body").
    //     // Return {std::move(graph), {}} once the CFG is built. See docs/cfg-pass.md.
    //     CfgResult cfg;

    //     cfg.graph = Graph{};
    //     Graph &g = cfg.graph.value();
    //     // if (!cfg.graph.has_value())
    //     //     return {
    //     //         std::nullopt, {{"No graph provided", "Empty graph was provided", method.root().range()}}};
    //     g.nodes.push_back({"start", "Start", "Start", SourceRange{0, 0}});
    //     g.nodes.push_back({"end", "End", "End", SourceRange{0, 0}});

    //     std::deque<SyntaxNode> q;
    //     std::deque<IfBlock> conditions;
    //     GraphNode previous_node = g.nodes[0];
    //     q.push_back(method.root());
    //     while (!q.empty())
    //     {
    //         SyntaxNode &node = q.back();
    //         q.pop_back();
    //         if (node.kind() == "local_declaration_statement")
    //         {
    //             GraphNode local_decl_node = GraphNode{"n" + std::to_string(g.nodes.size()), std::string(node.kind()), std::string(node.text()), node.range()};
    //             g.nodes.push_back(local_decl_node);
    //             g.edges.push_back({previous_node.id, local_decl_node.id, ""});
    //             // cfg.diagnostics.push_back(Diagnostic{"INFO","Assigned parent " + previous_node.id + "to " + local_decl_node.id ,node.range()});

    //             // TODO: Not intrested in this nodes children (no function calls YET) Recursion?
    //             if (previous_node.kind == "if_statement")
    //             {
    //                 conditions[0].end = local_decl_node;
    //                 g.edges.pop_back(); // delete the wrong condition->end edge
    //             }
    //             previous_node = local_decl_node;
    //         }
    //         else if (node.kind() == "expression_statement")
    //         {
    //             GraphNode expr_node = GraphNode{"n" + std::to_string(g.nodes.size()), std::string(node.kind()), std::string(node.text()), node.range()};
    //             g.nodes.push_back(expr_node);
    //             g.edges.push_back({previous_node.id, expr_node.id, ""});

    //             // Treat the same as a local decl
    //             if (previous_node.kind == "if_statement")
    //             {
    //                 conditions[0].end = expr_node;
    //                 g.edges.pop_back(); // delete the wrong condition->end edge
    //             }
    //             previous_node = expr_node;
    //         }
    //         else if (node.kind() == "return_statement")
    //         {
    //             GraphNode return_node = GraphNode{"n" + std::to_string(g.nodes.size()), std::string(node.kind()), std::string(node.text()), node.range()};
    //             g.nodes.push_back(return_node);
    //             g.edges.push_back({previous_node.id, return_node.id, ""});

    //             if (previous_node.kind == "if_statement")
    //             {
    //                 conditions[0].end = return_node;
    //                 g.edges.pop_back(); // delete the wrong condition->end edge
    //             }
    //             previous_node = return_node;
    //             g.edges.push_back({return_node.id, "end", "Return"});
    //         }
    //         else if (node.kind() == "if_statement")
    //         {
    //             // Add the condition block
    //             GraphNode phi_node = {"n" + std::to_string(g.nodes.size()), "if_statement", std::string(node.child(2).text()), node.child(2).range()};
    //             g.nodes.push_back(phi_node);
    //             g.edges.push_back({previous_node.id, phi_node.id, ""});
    //             // for (auto &child : node.children())
    //             // {
    //             //     q.push_back(child);
    //             // }
    //             SyntaxNode condition_node = node.field("condition");
    //             SyntaxNode consequence_node = node.field("consequence");
    //             SyntaxNode alternative_node = node.field("alternative");

    //             conditions.push_back(IfBlock{condition_node, consequence_node, alternative_node, phi_node, phi_node, IfState::CONDITION});
    //             q.push_back(alternative_node);
    //             q.push_back(consequence_node);
    //             // Add then branch child (not in q)
    //             // GraphNode then_node = {"n" + std::to_string(g.nodes.size()), std::string(consequence_node.kind()), std::string(consequence_node.text()), consequence_node.range()};
    //             // g.nodes.push_back(then_node);
    //             // g.edges.push_back({phi_node.id, then_node.id, "true"});
    //             // for (SyntaxNode &child : consequence_node.children())
    //             // {
    //             //     q.push_back(child);
    //             // }

    //             // // Add else branch child (not in q)
    //             // GraphNode else_node = {"n" + std::to_string(g.nodes.size()), std::string(alternative_node.kind()), std::string(alternative_node.text()), alternative_node.range()};
    //             // g.nodes.push_back(else_node);
    //             // g.edges.push_back({condition_node.id, else_node.id, "false"});
    //             // for (SyntaxNode &child : node.child(6).children())
    //             // {
    //             //     q.push_back(child);
    //             // }

    //             if (previous_node.kind == "if_statement")
    //             {
    //                 conditions[0].end = phi_node;
    //                 g.edges.pop_back(); // delete the wrong condition->end edge
    //             }
    //             previous_node = phi_node;
    //         }
    //         else
    //         {
    //             for (auto &child : node.children())
    //             {
    //                 q.push_front(child);
    //             }
    //             // for (std::uint32_t i = node.children().size() - 1; i > 0; --i)
    //             // {
    //             //     q.push_back(node.child(i));
    //             // };
    //             if (!conditions.empty())
    //             {
    //                 IfBlock condition = conditions.front();
    //                 switch (condition.state)
    //                 {
    //                 case IfState::CONDITION:
    //                     if (&node == &condition.consequence)
    //                     {
    //                         condition.state = IfState::CONSEQUENCE;
    //                         previous_node = condition.phi;
    //                     }

    //                     break;
    //                 case IfState::CONSEQUENCE:
    //                     if (&node == &condition.alternative)
    //                     {
    //                         condition.state = IfState::ALTERNATIVE;
    //                         previous_node = condition.phi;
    //                     }
    //                     break;
    //                 case IfState::ALTERNATIVE:
    //                     // TODO: end branch handling
    //                     break;
    //                 }
    //             }
    //         }
    //     }

    //     return cfg;
    //     // return {std::nullopt, {{"cfg_not_implemented", "CFG construction is not implemented yet.", method.root().range()}}};
    // }
} // namespace lab
