#include "lab/SSA.hpp"
#include "lab/cfg.hpp"
#include <cassert>
#include <unordered_set>
#include <unordered_map>
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

            return replace_identifiers(vr, n.field("left")) + " " + replace_identifiers(vr, n.field("operator")) + " " + replace_identifiers(vr, n.field("right"));
        }
        if (n.kind() == "identifier")
        {
            return vr.get_variable(std::string(n.text())).to_string();
        }
        else if (n.kind() == "prefix_unary_expression" || n.kind() == "postfix_unary_expression")
        {
            return replace_identifiers(vr, n.child(0)) + replace_identifiers(vr, n.child(1)); // TODO: better to use a loop here
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
    void build_dependencies(std::unordered_map<std::string, std::vector<GraphNode>> &preds, std::unordered_map<std::string, std::vector<GraphNode>> &succs, const Graph &g)
    {
        for (const auto &e : g.edges)
        {
            auto from = find_node_in_graph(g, e.from);
            auto to = find_node_in_graph(g, e.to);
            if (!from.has_value() || !to.has_value())
            {
                continue;
            }
            if (preds.contains(to.value().id))
            {
                preds[to.value().id].push_back(from.value());
            }
            else
            {
                preds[to.value().id] = {from.value()};
            }

            if (succs.contains(from.value().id))
            {
                succs[from.value().id].push_back(to.value());
            }
            else
            {
                succs[from.value().id] = {to.value()};
            }
        }
    }

    NodeResult create_phi_node(VariableRegistry &gvr, VariableRegistry &lvr, Graph &g, std::vector<Variable> &v, GraphNode &n, NodeResult p)
    {
        std::string id = get_next_id(g);
        Variable newvar = gvr.get_and_inc_variable(v[0].name, id);
        lvr.set_version(newvar.name, newvar.version, newvar.created_at);
        std::string text = newvar.to_string() + " = phi (";
        for (auto &var : v)
        {
            text += var.to_string();
            if (&var != &v.back())
            {
                text += ", ";
            }
        }
        text += ")";
        g.nodes.push_back(GraphNode{id, "phi", text, n.range, n.syntax_node});
        // Ignore control flow dependencies (or not?    )
        for (const auto &edge : p.outs)
        {
            g.edges.push_back(GraphEdge{edge.id, id, ""});
        }

        // Instead, create edges for every incoming variable
        // g.edges.push_back(GraphEdge{v1.created_at, id, "", GraphEdgeColor::Green});
        // g.edges.push_back(GraphEdge{v2.created_at, id, "", GraphEdgeColor::Green});

        p.outs = {ParentEdge{id}};
        return p;
    }

    SsaResult build_ssa(const ParsedMethod &method, const Graph &cfg)
    {
        SsaResult ssa;
        ssa.graph = Graph{};
        Graph &g = ssa.graph.value();
        VariableRegistry vr;
        std::unordered_set<std::string> visited;
        std::unordered_map<std::string, std::vector<GraphNode>> preds;
        std::unordered_map<std::string, std::vector<GraphNode>> succs;
        std::unordered_map<std::string, VariableRegistry> vars;
        std::unordered_map<std::string, std::string> cfg2ssa;
        std::unordered_map<std::string, std::vector<PendingPhi>> pending_phis;
        std::unordered_map<std::string, std::vector<ParentEdge>> pending_edges; // CFG id to SSA id

        build_dependencies(preds, succs, cfg);
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

        std::function<NodeResult(GraphNode, NodeResult, unsigned, VariableRegistry)> visit = [&](GraphNode n, NodeResult p, unsigned depth, VariableRegistry lvr)
        {
            visited.insert(n.id);
            std::string created_node_id = "";

            // If a node has multiple preds
            // It would need a phi node placed (?)
            // But only on the variables that are different in both branches
            if (preds.contains(n.id) && preds[n.id].size() > 1)
            {
                // Find different variables
                std::vector<std::string> pending_phi_ids;
                std::optional<VariableRegistry> pvr;
                if (vars.contains(preds[n.id][0].id))
                {
                    pvr = vars[preds[n.id][0].id];
                }
                std::unordered_map<std::string, std::vector<Variable>> diff_vars;
                for (const auto &pred : preds[n.id])
                {
                    // TODO: It is possible to lose a conflict if the first pvr does not contain the conflicting variable
                    if (vars.contains(pred.id) && pvr.has_value())
                    {
                        const auto variables = pvr.value().get_conflicting_variables(vars[pred.id]);
                        for (const auto &v : variables)
                        {
                            if (!diff_vars.contains(v.first.name))
                            {
                                diff_vars[v.first.name] = {};
                                for (const auto &pr : preds[n.id])
                                {
                                    if (vars.contains(pr.id))
                                    {
                                        diff_vars[v.first.name].push_back(vars[pr.id].get_variable(v.first.name));
                                    }
                                }
                                // diff_vars[v.first.name] = {v.first, v.second};
                            }
                            else
                            {
                                // diff_vars[v.first.name].push_back(v.second);
                            }
                        }
                    }
                    else
                    {
                        pending_phi_ids.push_back(pred.id);
                        // We cannot judge about if there is a conflict now
                        // When this node is actually visited, we want it to:
                        // compare its own varreg with whatever we had here
                        // if a conflict is present, splice the phi chain somewhere
                        // and add a typical phi node

                        // For that, we should give the node:
                        // 1. Our vr
                        // 2. Parent for the phi node (either previous phi or list of what came in)
                        // 3. Successor for the phi node
                        // 4. This graph node

                        // How to find successor?
                        //  We dont really know what connects to this node
                        //  So either look for cfg2ssa[each: succs[n.id]]
                        //  Or for graph edges coming out of the parent node
                        //  We will need to break those  (or change the dest) anyway i think

                        // Alternative idea:

                        // Cannot go further until we come here again, but this time when everything is evaluated
                        visited.erase(n.id); // Will break on self-edges and loops(?)
                        if (pending_edges.contains(n.id))
                        {
                            pending_edges[n.id].push_back(p.outs[0]);
                        }
                        else
                        {
                            pending_edges[n.id] = p.outs;
                        }
                        return NodeResult{};

                        // BUT: if there is an actual loop in the cfg... like... a loop.. you know
                        // We will never return there (at least with visited preds)
                        // And we do not know what versions or variables to put phi, as the loop is not visited yet.
                        // ssa.diagnostics.push_back(Diagnostic{"ssa_dbg", "Encountered missing vars", n.range.value_or(SourceRange{})});
                    }
                }

                for (auto &[name, conflicts] : diff_vars)
                {
                    // ssa.diagnostics.push_back(Diagnostic{"ssa_dbg", "Conflict size: " + std::to_string(conflicts.size()), n.range.value_or(SourceRange{})});

                    // Chain phi nodes one after the other
                    p = create_phi_node(vr, lvr, g, conflicts, n, p);
                    if (created_node_id == "")
                        created_node_id = p.outs[0].id;
                }
                for (const auto &id : pending_phi_ids)
                {
                    if (pending_phis.contains(id))
                    {
                        pending_phis[id].push_back(PendingPhi{pvr.value(), p, n});
                    }
                    else
                    {
                        pending_phis[id] = {PendingPhi{pvr.value(), p, n}};
                    }
                }
                if (pending_edges.contains(n.id))
                {
                    for (const auto &e : pending_edges[n.id])
                    {
                        if (created_node_id != "")
                        {
                            g.edges.push_back(GraphEdge{e.id, created_node_id, e.comment, e.color});
                        }
                    }
                }
            }
            NodeResult new_parent = p;

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
                // For statement produces a "local_declaration_statement" that is actually a variable_declaration
                auto variable_declarator = find_child1(n.syntax_node.value(), "variable_declarator");

                if (!variable_declaration.has_value() && !variable_declarator.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"Found declaration without syntax node", "No syntax node in variable_declaration", n.range.value_or(SourceRange{})});
                    return NodeResult{};
                }
                if (variable_declaration.has_value())
                {
                    type = variable_declaration.value().field("type").text();
                }
                else
                {
                    type = n.syntax_node.value().field("type").text();
                }
                if (!variable_declarator.has_value())
                    variable_declarator = find_child1(variable_declaration.value(), "variable_declarator");

                if (!variable_declarator.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"Found declaration without syntax node", "No syntax node in variable_declarator", n.range.value_or(SourceRange{})});
                    return NodeResult{};
                }
                name = variable_declarator.value().field("name").text();
                auto rest = variable_declarator.value().child(2); // Assume name (0) = (1) *expr* (2)
                std::string id = "n" + std::to_string(g.nodes.size());

                vr.create_variable(name, id, type);
                Variable var = lvr.create_variable(name, id, type);

                // text = var.to_string() + " = " + std::string(rest.text());
                text = var.to_string() + " = " + replace_identifiers(lvr, rest);

                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, ""});
                }
                new_parent = NodeResult{{ParentEdge{id}}, {}};
                if (created_node_id == "")
                    created_node_id = id;
                // for (const auto &edge : cfg.edges)
                // {
                //     if (edge.from == n.id)
                //     {

                //         auto to = find_node_in_graph(cfg, edge.to);
                //         if (to.has_value())
                //         {
                //             if (!visited.contains(to.value().id))
                //                 visit(to.value(), NodeResult{{ParentEdge{id}}, {}}, depth + 1);
                //         }
                //     }
                // }
            }
            else if (n.kind == "expression_statement")
            {
                if (!n.syntax_node.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"ssa_no_syntax", "Found expression without syntax node", n.range.value_or(SourceRange{})});
                    return NodeResult{};
                }
                std::string text;
                std::string id = "n" + std::to_string(g.nodes.size());

                std::optional<SyntaxNode> assignment_expression = find_child1(n.syntax_node.value(), "assignment_expression");
                if (!assignment_expression.has_value())
                {
                    std::optional<SyntaxNode> postfix_expression = find_child1(n.syntax_node.value(), "postfix_unary_expression");
                    if (!postfix_expression.has_value())
                    {
                        ssa.diagnostics.push_back(Diagnostic{"ssa_no_syntax", "Found assignment  without syntax node", n.range.value_or(SourceRange{})});
                        return NodeResult{};
                    }
                    std::string identifier = std::string(postfix_expression.value().child(0).text());
                    const auto &v = vr.get_and_inc_variable(identifier, n.id);
                    lvr.set_version(v.name, v.version, n.id);
                    text = replace_identifiers(lvr, n.syntax_node.value());
                }
                else // It is an assignment
                {
                    // Assumes left is identifier
                    std::string dest = std::string(assignment_expression.value().field("left").text());
                    std::string right = replace_identifiers(lvr, assignment_expression.value().field("right"));
                    const auto &v = vr.get_and_inc_variable(dest, n.id);
                    lvr.set_version(v.name, v.version, n.id);
                    text = lvr.get_variable(dest).to_string() + "=" + right;
                }
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, ""});
                }
                new_parent = NodeResult{{ParentEdge{id}}, {}};
                if (created_node_id == "")
                    created_node_id = id;
                // for (const auto &edge : cfg.edges)
                // {
                //     if (edge.from == n.id)
                //     {

                //         auto to = find_node_in_graph(cfg, edge.to);
                //         if (to.has_value())
                //         {
                //             if (!visited.contains(to.value().id))
                //                 visit(to.value(), NodeResult{{ParentEdge{id}}, {}}, depth + 1);
                //         }
                //     }
                // }
            }
            else if (n.kind == "assignment_expression")
            {
                if (!n.syntax_node.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"ssa_no_syntax", "Found assignment expression without syntax node", n.range.value_or(SourceRange{})});
                }
                SyntaxNode assignment_expression = n.syntax_node.value();
                // Assumes left is identifier
                std::string dest = std::string(assignment_expression.field("left").text());
                std::string text;
                std::string right = replace_identifiers(lvr, assignment_expression.field("right"));
                std::string id = "n" + std::to_string(g.nodes.size());
                const auto v = vr.get_and_inc_variable(dest, id);
                lvr.set_version(dest, v.version, id);
                text = lvr.get_variable(dest).to_string() + "=" + right;
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, ""});
                }
                new_parent = NodeResult{{ParentEdge{id}}, {}};
                if (created_node_id == "")
                    created_node_id = id;
            }
            else if (n.kind == "return_statement")
            {
                std::string id = get_next_id(g);
                if (!n.syntax_node)
                {
                    ssa.diagnostics.push_back(Diagnostic{"missing_syntax_node", "Return node is missing a SyntaxNode", n.range.value_or(SourceRange{})});

                    return NodeResult{{}, {}};
                }
                std::string text = " ";
                for (const auto &child : n.syntax_node.value().children())
                {
                    text += replace_identifiers(lvr, child) + " ";
                }
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, ""});
                }
                new_parent = NodeResult{{ParentEdge{id}}, {}};
                if (created_node_id == "")
                    created_node_id = id;
            }
            else if (n.kind == "condition")
            {
                std::string id = get_next_id(g);
                if (!n.syntax_node.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"missing_syntax_node", "Condition node is missing a SyntaxNode", n.range.value_or(SourceRange{})});

                    return NodeResult{{}, {}};
                }
                std::string text = replace_identifiers(lvr, n.syntax_node.value());
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, ""});
                }
                new_parent = NodeResult{{ParentEdge{id}}, {}};
                if (created_node_id == "")
                    created_node_id = id;
            }
            else // Non terminal nodes
            {

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
            vars[n.id] = lvr;

            // if (!p.outs.empty())
            //     cfg2ssa[n.id] = p.outs[0].id; // Ideally that would be just newly created node id

            // This prevents nodes from overriding the cfg2ssa, however now some n.id might miss ther SSA link
            // IDK if we need SSA link for every CFG node...
            if (created_node_id != "")
                cfg2ssa[n.id] = created_node_id;

            // if (pending_phis.contains(n.id))
            // { // If some phi was waiting on this node
            //     for (auto &phi : pending_phis[n.id])
            //     {
            //         auto new_p = phi.p;
            //         for (const auto &[v1, v2] : phi.vr.get_conflicting_variables(vr))
            //         {
            //             // Chain phi nodes one after the other
            //             new_p = create_phi_node(vr, g, v1, v2, phi.n, new_p);

            //             // Now lets fix the graph

            //             for (auto &edge : cfg.edges)
            //             {
            //                 if (edge.to==){

            //                 }
            //             }
            //         }
            //     }
            // }
            for (const auto &edge : cfg.edges)
            {
                if (edge.from == n.id)
                {

                    auto to = find_node_in_graph(cfg, edge.to);
                    if (to.has_value())
                    {
                        // ssa.diagnostics.push_back(Diagnostic{"Found next node:", to.value().kind + "", n.range.value_or(SourceRange{})});
                        if (!visited.contains(to.value().id))
                            visit(to.value(), new_parent, depth + 1, lvr);
                        else
                        {
                            if (cfg2ssa.contains(edge.to))
                            {
                                g.edges.push_back(GraphEdge{cfg2ssa[edge.from], cfg2ssa[edge.to], edge.label, edge.color});
                            }
                            else
                            {
                                // ssa.diagnostics.push_back(Diagnostic{"ssa_dbg", "Do not know the next node!", n.range.value_or(SourceRange{})});

                                // Oopsie, have not visited "to" node (or "start"/"end")
                            }
                        }
                    }
                }

                // if (edge.to == n.id && created_node_id != "")
                // { // Sometimes we may forget about edges TO this node (if visited.contains fails)
                //     // TODO: edge.from might not be visited yet
                //     g.edges.push_back(GraphEdge{cfg2ssa[edge.from], created_node_id, ""});
                // }
            }

            return p;
            // return NodeResult{};
        };

        visit(start, NodeResult{}, 0, vr);
        // ssa.diagnostics.push_back(Diagnostic{"", "Graph Size:" + std::to_string(ssa.graph.value().nodes.size()), SourceRange{}});

        // ssa.graph = cfg;
        // TODO: implement SSA on top of the method's control-flow graph.

        return ssa;
        // return {std::nullopt, {{"ssa_not_implemented", "SSA construction is not implemented yet.", {0, 0}}}};
    }

    Variable VariableRegistry::create_variable(std::string name, std::string node, std::string type)
    {
        Variable new_var{0, name, type, node};
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
    Variable VariableRegistry::get_and_inc_variable(std::string name, std::string node)
    {
        variables[name].version++;
        variables[name].created_at = node;
        return variables[name];
    }
    void VariableRegistry::set_version(std::string name, std::size_t version, std::string node)
    {
        variables[name].version = version;
        variables[name].created_at = node;
    }
    std::vector<std::pair<Variable, Variable>> VariableRegistry::get_conflicting_variables(VariableRegistry &other)
    {
        std::vector<std::pair<Variable, Variable>> res;
        for (const auto &[n, v] : this->variables)
        {
            if (v.version != other.get_version(n))
                res.push_back(std::pair{v, other.get_variable(n)});
        }
        return res;
    }

} // namespace lab
