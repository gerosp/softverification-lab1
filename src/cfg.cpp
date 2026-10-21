#include <deque>
#include "lab/cfg.hpp"
namespace lab
{
    static std::string last_case_value_name = "";
    std::string get_next_id(const Graph &g) { return "n" + std::to_string(g.nodes.size()); }
    const std::vector<ParentEdge> create_simple_parents(const std::vector<std::string> &ids, const std::string &comment = "", const GraphEdgeColor color = GraphEdgeColor::Default)
    {
        std::vector<ParentEdge> res;
        for (const auto &p : ids)
        {
            res.push_back(ParentEdge{p, comment, color});
        }
        return res;
    }
    const std::vector<std::string> extract_simple_parents(const std::vector<ParentEdge> &es)
    {
        std::vector<std::string> res;
        for (const auto &e : es)
        {
            res.push_back(e.id);
        }
        return res;
    }
    void handle_lc(Graph &g, GraphNode &g_cond, std::vector<LoopControl> &lcs, std::vector<ParentEdge> &out)
    {
        for (const auto &lc : lcs)
        {
            if (lc.kind == "continue_statement")
            {
                for (auto &from : lc.from)
                {
                    g.edges.push_back(GraphEdge{from, g_cond.id, "continue", GraphEdgeColor::Blue});
                }
            }
            else if (lc.kind == "break_statement")
            {
                for (const auto &from : lc.from)
                {
                    out.push_back(ParentEdge{from, "break", GraphEdgeColor::Orange});
                }
            }
        }
    }

    GraphNode create_simple_node(Graph &g, const SyntaxNode &node, const std::vector<ParentEdge> &parent, const std::string id, const std::string kind = "", const std::optional<SourceRange> range = std::nullopt)
    {
        GraphNode g_node = GraphNode{id, kind == "" ? std::string(node.kind()) : kind, std::string(node.text()), range.has_value() ? range.value() : node.range(), node};
        g.nodes.push_back(g_node);
        for (const auto &p : parent)
        {
            g.edges.push_back({p.id, g_node.id, p.comment, p.color});
        }
        return g_node;
    }

    CfgResult build_cfg(const ParsedMethod &method)
    {
        CfgResult cfg;

        cfg.graph = Graph{};
        Graph &g = cfg.graph.value();
        g.nodes.push_back({"start", "Start", "Start", SourceRange{0, 0}, std::nullopt});
        g.nodes.push_back({"end", "End", "End", SourceRange{0, 0}, std::nullopt});
        std::deque<std::string> switch_case_value;
        std::function<NodeResult(SyntaxNode, NodeResult, unsigned)> visit = [&](SyntaxNode n, NodeResult p, unsigned depth)
        {
            // auto id = "n" + std::to_string(g.nodes.size());
            auto id = get_next_id(g);

            auto kind = std::string(n.kind());

            if (n.kind() == "local_declaration_statement")
            {
                // create_simple_node(g, n.child(0), p.outs, id); // TODO: This breaks SSA as it expects local_declaration_statement
                create_simple_node(g, n, p.outs, id);

                return NodeResult{create_simple_parents({id}), {}};
            }
            else if (n.kind() == "expression_statement")
            {
                create_simple_node(g, n, p.outs, id);
                return NodeResult{create_simple_parents({id}), {}};
            }
            else if (n.kind() == "return_statement")
            {
                GraphNode node = create_simple_node(g, n, p.outs, id); // TODO: remove trailing ';'
                g.edges.push_back({node.id, "end", "return"});
                std::vector<std::string> out;
                return NodeResult{create_simple_parents(out), {}};
            }
            else if (n.kind() == "if_statement")
            {
                SyntaxNode condition_node = n.field("condition");
                GraphNode g_cond = create_simple_node(g, condition_node, p.outs, id, "condition");
                SyntaxNode consequence_node = n.field("consequence");
                SyntaxNode alternative_node = n.field("alternative");
                // g.edges.push_back(GraphEdge{g_cond.id, visit(consequence_node, id, depth + 1), "true"});
                // g.edges.push_back(GraphEdge{g_cond.id, visit(alternative_node, id, depth + 1), "false"});
                NodeResult cons_node_id = visit(consequence_node, NodeResult{create_simple_parents({id}, "true", GraphEdgeColor::Green), {}}, depth + 1);
                NodeResult alt_node_id = visit(alternative_node, NodeResult{create_simple_parents({id}, "false", GraphEdgeColor::Red), {}}, depth + 1);
                // cfg.diagnostics.push_back(Diagnostic{"cons_node_id: ",cons_node_id,n.range()});
                NodeResult nr;
                for (auto &s : cons_node_id.outs)
                {
                    nr.outs.push_back(s);

                    // nr.outs.push_back(ParentEdge{s.id, "true", GraphEdgeColor::Green});
                }
                for (auto &s : alt_node_id.outs)
                {
                    nr.outs.push_back(s);
                    // nr.outs.push_back(ParentEdge{s.id, "false", GraphEdgeColor::Red});
                }
                for (auto &n : cons_node_id.lc)
                {
                    nr.lc.push_back(n);
                }
                for (auto &n : alt_node_id.lc)
                {
                    nr.lc.push_back(n);
                }
                return nr;
            }
            else if (n.kind() == "while_statement")
            {
                SyntaxNode condition_node = n.field("condition");
                GraphNode g_cond = create_simple_node(g, condition_node, p.outs, id, "condition");
                SyntaxNode body_node = n.field("body");
                // std::vector<std::string> cond_node_id = visit(condition_node, std::vector{id}, depth + 1);
                NodeResult body_node_id = visit(body_node, NodeResult{create_simple_parents({id}), {}}, depth + 1);
                for (const auto &b : body_node_id.outs)
                {
                    g.edges.push_back(GraphEdge{b.id, id, "loop"}); // TODO: do every node, not just [0]
                }
                // g.edges.push_back(GraphEdge{body_node_id[0], id, "loop"}); // TODO: do every node, not just [0]
                std::vector<ParentEdge> out;
                // if (!body_node_id.lc.empty())
                //     cfg.diagnostics.push_back(Diagnostic{"Non empty LC", "", n.range()});
                handle_lc(g, g_cond, body_node_id.lc, out);

                // for (auto &s : cond_node_id)
                // {
                //     out.push_back(s);
                // }
                // for (auto &s : body_node_id)
                // {
                //     out.push_back(s);
                // }
                out.push_back(ParentEdge{g_cond.id});
                return NodeResult{out, {}}; // All loop conditions should be handled there
            }
            else if (n.kind() == "for_statement")
            {
                SyntaxNode init_node = n.field("initializer");
                GraphNode g_init = create_simple_node(g, init_node, p.outs, id, "local_declaration_statement");
                SyntaxNode condition_node = n.field("condition");
                GraphNode g_cond = create_simple_node(g, condition_node, create_simple_parents({g_init.id}), get_next_id(g), "condition");

                SyntaxNode body_node = n.field("body");
                NodeResult body_node_id = visit(body_node, NodeResult{create_simple_parents({g_cond.id}), {}}, depth + 1);
                SyntaxNode update_node = n.field("update");

                GraphNode g_update = create_simple_node(g, update_node, body_node_id.outs, get_next_id(g), std::string(update_node.kind()));
                g.edges.push_back(GraphEdge{g_update.id, g_cond.id, "loop"});
                std::vector<ParentEdge> out = {ParentEdge{g_cond.id}};

                handle_lc(g, g_update, body_node_id.lc, out);
                // for (const auto &b : body_node_id)// Create an update block for every branch
                // {

                //     GraphNode g_update = create_simple_node(g, update_node, {b}, get_next_id(g));
                //     g.edges.push_back(GraphEdge{g_update.id, g_cond.id, "loop"});
                // }

                return NodeResult{out, {}};
            }
            else if (n.kind() == "do_statement")
            {
                SyntaxNode condition_node = n.field("condition");
                SyntaxNode body_node = n.field("body");
                NodeResult body_node_id = visit(body_node, p, depth + 1);
                GraphNode g_cond = create_simple_node(g, condition_node, body_node_id.outs, get_next_id(g), "condition");
                std::vector<ParentEdge> out = {ParentEdge{g_cond.id}};
                handle_lc(g, g_cond, body_node_id.lc, out);

                g.edges.push_back(GraphEdge{g_cond.id, id, "loop"}); // TODO: might cause issues (assuming id is given to the first body node)
                // for (const auto &b : body_node_id)
                // {
                //     g.edges.push_back(GraphEdge{b, id, "loop"}); // TODO: do every node, not just [0]
                // }
                return NodeResult{out, {}};
            }
            else if (n.kind() == "continue_statement" || n.kind() == "break_statement")
            {
                // create an edge from the parent to the loop condition
                // g.edges.push_back(GraphEdge{p.outs[0], "end", "continue"});
                return NodeResult{create_simple_parents({}), {LoopControl{std::string(n.kind()), extract_simple_parents(p.outs)}}}; // Do not return a parent
            }
            else if (n.kind() == "switch_statement")
            {
                SyntaxNode value = n.field("value");
                last_case_value_name = value.text();
                switch_case_value.push_front(std::string(value.text()));
                NodeResult out;
                SyntaxNode body = n.field("body");
                NodeResult nr = p;
                for (const auto &child : body.children())
                {
                    nr = visit(child, nr, depth + 1);
                    for (const auto &lc : nr.lc)
                    {
                        for (const auto &f : lc.from)
                        {
                            out.outs.push_back(ParentEdge{f});
                        }
                    }
                }
                switch_case_value.pop_front();
                return out; // eat break statements
            }
            else if (n.kind() == "switch_section")
            {
                std::string constant_pattern = "";
                // Check if one of the children is constant_pattern
                for (const auto &child : n.children())
                {
                    if (child.kind() == "constant_pattern")
                    {
                        constant_pattern = child.text();
                        break;
                    }
                }
                NodeResult nr;
                // std::string value = "val"; // TODO: find parent in graph, get its value. We ACTUALLY DONT know its id by p.outs[0]
                // std::string value = last_case_value_name; // TODO: this WILL break
                std::string value = switch_case_value.front();
                if (constant_pattern != "")
                { // regular branch
                    std::string cond_id = get_next_id(g);
                    auto synthetic = lab::parse_method("bool __synthetic() { return " + value + " == " + constant_pattern + "; }");

                    if (!synthetic.ok())
                    {
                        cfg.diagnostics.push_back(Diagnostic{"cfg-invalid-synthetic", "Could not create synthetic switch node " + synthetic.diagnostics[0].message, n.range()});
                        return NodeResult{};
                    }
                    // SyntaxNode cond_syntax(synthetic.method->root().field("body"), &method);
                    SyntaxNode cond_syntax = synthetic.method->root().field("body").children(true).front().children(true).front();
                    const_cast<ParsedMethod &>(method).synthetic_methods.push_back(std::move(synthetic.method)); // TODO: hehe potentially unsafe
                    g.nodes.push_back(GraphNode{cond_id, "condition", value + "==" + constant_pattern, n.range(), cond_syntax});
                    for (const auto &pr : p.outs)
                    {
                        g.edges.push_back(GraphEdge{pr.id, cond_id, pr.comment, pr.color});
                    }
                    // g.edges.push_back(GraphEdge{p.outs[0], cond_id, ""});
                    nr = {{ParentEdge{cond_id, "true", GraphEdgeColor::Green}}, {}};
                    for (const auto &child : n.children())
                    {
                        nr = visit(child, nr, depth + 1);
                    }
                    nr.outs.push_back(ParentEdge{cond_id, "false", GraphEdgeColor::Red});
                }
                else
                { // default branch
                    nr = p;
                    for (const auto &child : n.children())
                    {
                        nr = visit(child, nr, depth + 1);
                    }
                }
                return nr;
                // return NodeResult{nr.outs, {}};
                // eat break node if present, make the necessary adjustment (add break to outs)
            }
            else if (n.kind() == "foreach_statement")
            {
                // cfg.diagnostics.push_back(Diagnostic{"INFO", "Foreach", n.range()});

                std::string id = get_next_id(g);
                SyntaxNode left = n.field("left");
                SyntaxNode right = n.field("right");
                std::vector<ParentEdge> out;
                auto synthetic = lab::parse_method("bool __synthetic() { var enumerator_" + std::string(right.text()) + " = " + std::string(right.text()) + ".GetEnumerator() " + "; }");
                if (!synthetic.ok())
                {
                    cfg.diagnostics.push_back(Diagnostic{"cfg-invalid-synthetic", "Could not create synthetic foreach init node " + synthetic.diagnostics[0].message, n.range()});
                    return NodeResult{};
                }
                SyntaxNode init_syntax = synthetic.method->root().field("body").children(true).front().children(true).front();
                const_cast<ParsedMethod &>(method).synthetic_methods.push_back(std::move(synthetic.method));
                // SyntaxNode cond_syntax(synthetic.method->root().field("body"), &method);

                // parse("Enerator enumerator = +"right.text()+".GetEnumerator();")
                GraphNode g_init = create_simple_node(g, init_syntax, p.outs, get_next_id(g), "local_declaration_statement", right.range());

                // SyntaxNode enumerator = %left%.getEnumerator()

                SyntaxNode body = n.field("body");
                // SyntaxNode condition{TSNode{}, &method};
                auto synthetic_cond = lab::parse_method("bool __synthetic() { enumerator_" + std::string(right.text()) + ".MoveNext(); }");
                if (!synthetic_cond.ok())
                {
                    cfg.diagnostics.push_back(Diagnostic{"cfg-invalid-synthetic", "Could not create synthetic foreach cond node " + synthetic.diagnostics[0].message, n.range()});
                    return NodeResult{};
                }
                SyntaxNode cond_syntax = synthetic_cond.method->root().field("body").children(true).front().children(true).front();
                const_cast<ParsedMethod &>(method).synthetic_methods.push_back(std::move(synthetic_cond.method));

                // SyntaxNode condition; // enumerator.MoveNext()
                GraphNode g_cond = create_simple_node(g, cond_syntax, {ParentEdge{g_init.id, ""}}, get_next_id(g), "condition", right.range());

                // Create   requested variable as enumerator.Current
                // parse("var "+right.text+"= enumerator.Current;")

                auto synthetic_li = lab::parse_method("bool __synthetic() { var " + std::string(left.text()) + " = enumerator_" + std::string(right.text()) + ".Current; }");
                if (!synthetic_li.ok())
                {
                    cfg.diagnostics.push_back(Diagnostic{"cfg-invalid-synthetic", "Could not create synthetic foreach cond node " + synthetic.diagnostics[0].message, right.range()});
                    return NodeResult{};
                }
                SyntaxNode loop_init_syntax = synthetic_li.method->root().field("body").children(true).front().children(true).front();
                const_cast<ParsedMethod &>(method).synthetic_methods.push_back(std::move(synthetic_li.method));

                GraphNode loop_init = create_simple_node(g, loop_init_syntax, {ParentEdge{g_cond.id, "true", GraphEdgeColor::Green}}, get_next_id(g), "local_declaration_statement", left.range());
                NodeResult body_node_id = visit(body, NodeResult{create_simple_parents({loop_init.id}), {}}, depth + 1);
                for (const auto &b : body_node_id.outs)
                {
                    g.edges.push_back(GraphEdge{b.id, g_cond.id, "loop"});
                }
                handle_lc(g, g_cond, body_node_id.lc, out);
                out.push_back(ParentEdge{g_cond.id, "false", GraphEdgeColor::Red});
                return NodeResult{out, {}};
            }
            else // Non terminal nodes
            {
                NodeResult prev_id = p;
                std::vector<LoopControl> lcs;
                std::vector<std::string> outs;
                for (SyntaxNode &child : n.children())
                {
                    prev_id = visit(child, NodeResult{prev_id.outs, {}}, depth + 1);
                    for (const auto &lc : prev_id.lc)
                    {
                        lcs.push_back(lc);
                    }
                }
                return NodeResult{prev_id.outs, lcs};
            }

            return p;
        };
        visit(method.root(), NodeResult{{ParentEdge{"start"}}, {}}, 0);
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
