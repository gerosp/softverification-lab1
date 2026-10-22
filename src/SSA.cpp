#include "lab/SSA.hpp"
#include "lab/cfg.hpp"
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <deque>
namespace lab
{
    // std::string get_next_id(const Graph &g) { return "n" + std::to_string(g.nodes.size()); }

    Variable get_replacement(Variable v, const std::unordered_map<Variable, Variable> &m)
    {
        while (m.contains(v))
        {
            v = m.at(v);
        }
        return v;
    }
    std::string render_node_label(std::string id, std::unordered_map<std::string, std::vector<std::variant<std::string, Variable>>> labels, std::unordered_map<Variable, Variable> r, std::string fallback)
    {
        auto node = labels.find(id);
        if (node == labels.end())
        {
            return fallback;
        }
        std::string out;
        for (const auto &p : node->second)
        {
            if (const auto *s = std::get_if<std::string>(&p))
            {
                out += *s;
            }
            else if (const auto *v = std::get_if<Variable>(&p))
            {
                out += get_replacement(*v, r).to_string();
            }
        }
        return out;
    }
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

    void build_label_from_identifiers(VariableRegistry &vr, const SyntaxNode &n, const std::string id, std::unordered_map<std::string, std::vector<std::variant<std::string, Variable>>> &labels)
    {
        std::string out;
        if (n.kind() == "binary_expression")
        {
            build_label_from_identifiers(vr, n.field("left"), id, labels);
            labels[id].push_back(" ");
            build_label_from_identifiers(vr, n.field("operator"), id, labels);
            labels[id].push_back(" ");
            build_label_from_identifiers(vr, n.field("right"), id, labels);
        }
        else if (n.kind() == "identifier")
        {
            labels[id].push_back(vr.get_variable(std::string(n.text())));
            // return vr.get_variable(std::string(n.text())).to_string();
        }
        else if (n.kind() == "prefix_unary_expression" || n.kind() == "postfix_unary_expression")
        {
            build_label_from_identifiers(vr, n.child(0), id, labels);
            build_label_from_identifiers(vr, n.child(1), id, labels);
            // return + // TODO: better to use a loop here
        }
        else
        {
            labels[id].push_back(std::string(n.text()));
        }
    }
    std::string build_phi_label(Variable out, std::vector<Variable> ins, std::unordered_map<Variable, Variable> r)
    {

        std::string res = get_replacement(out, r).to_string() + " = (";
        for (size_t i = 0; i < ins.size(); i++)
        {
            res += get_replacement(ins[i], r).to_string();
            if (i != ins.size() - 1)
            {
                res += ", ";
            }
        }
        res += ")";
        return res;
    }
    void parse_arguments(VariableRegistry &vr, const SyntaxNode &n)
    {
        for (const auto &param : n.field("parameters").children())
        {
            if (param.field("name").kind() == "identifier")
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

    void update_phi_node(Graph &g, std::string id, PendingPhi &phi)
    {
        for (auto &node : g.nodes)
        {
            if (node.id == id)
            {
                std::string text = phi.vr.get_variable(phi.var).to_string() + " = phi (";

                for (auto &[dep, var] : phi.deps)
                {
                    text += var.to_string();
                    {
                        text += ", ";
                    }
                }

                // TODO: remove the last symbol (its the ",")
                node.label = text + ")";
            }
        }
    }
    NodeResult create_phi_node(VariableRegistry &gvr, VariableRegistry &lvr, Graph &g, std::vector<Variable> &v, GraphNode &n, NodeResult p, std::string name)
    {
        std::string id = get_next_id(g);
        Variable newvar = gvr.get_and_inc_variable(name, id);
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
            g.edges.push_back(GraphEdge{edge.id, id, edge.comment, edge.color});
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
        std::unordered_set<Phi> removed_phis;

        std::unordered_set<std::string> visited;
        std::unordered_map<std::string, std::vector<GraphNode>> preds;
        std::unordered_map<std::string, std::vector<GraphNode>> succs;
        std::unordered_map<std::string, VariableRegistry> vars;
        std::unordered_map<std::string, std::string> cfg2ssa;
        std::unordered_map<std::string, std::vector<std::string>> phi_watchers;
        std::unordered_map<std::string, std::vector<ParentEdge>> pending_edges; // CFG id to SSA id
        std::unordered_map<std::string, PendingPhi> pending_phis;
        std::unordered_map<std::string, std::vector<std::variant<std::string, Variable>>> labels;
        std::vector<Phi> phis;
        std::unordered_map<Variable, std::unordered_set<Phi>> var2phis;
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
            if (n.kind == "End")
            {
                return NodeResult{};
            }

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
                bool is_pending = false;
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
                        // pending_phi_ids.push_back(pred.id);
                        is_pending = true;
                        break;

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
                        // visited.erase(n.id); // Will break on self-edges and loops(?)
                        // if (pending_edges.contains(n.id))
                        // {
                        //     pending_edges[n.id].push_back(p.outs[0]);
                        // }
                        // else
                        // {
                        //     pending_edges[n.id] = p.outs;
                        // }
                        // return NodeResult{};

                        // BUT: if there is an actual loop in the cfg... like... a loop.. you know
                        // We will never return there (at least with visited preds)
                        // And we do not know what versions or variables to put phi, as the loop is not visited yet.
                        // ssa.diagnostics.push_back(Diagnostic{"ssa_dbg", "Encountered missing vars", n.range.value_or(SourceRange{})});
                    }
                }
                if (!is_pending)
                {

                    for (auto &[name, conflicts] : diff_vars)
                    {
                        // ssa.diagnostics.push_back(Diagnostic{"ssa_dbg", "Conflict size: " + std::to_string(conflicts.size()), n.range.value_or(SourceRange{})});

                        // Chain phi nodes one after the other
                        p = create_phi_node(vr, lvr, g, conflicts, n, p, name);
                        if (created_node_id == "")
                            created_node_id = p.outs[0].id;

                        phis.push_back(Phi{
                            !is_pending,
                            p.outs[0].id,
                            lvr.get_variable(name), conflicts});
                        for (const auto &c : conflicts)
                        {
                            var2phis[c].insert(Phi{
                                !is_pending,
                                p.outs[0].id,
                                lvr.get_variable(name), conflicts});
                        }
                    }
                }
                else
                {
                    for (auto &[name, var] : lvr.variables)
                    {
                        std::unordered_map<std::string, Variable> deps;
                        bool everything_is_broken_stop_the_count = false;
                        std::vector<Variable> conflicts;
                        std::vector<Variable> ready_ins;
                        for (const auto &pred : preds[n.id])
                        {
                            if (vars.contains(pred.id))
                            {
                                if (!vars[pred.id].has_variable(name))
                                {
                                    everything_is_broken_stop_the_count = true;
                                    break;
                                }
                                conflicts.push_back(vars[pred.id].get_variable(name));
                                ready_ins.push_back(vars[pred.id].get_variable(name));

                                deps[pred.id] = vars[pred.id].get_variable(name);
                            }
                            else
                            {
                                Variable v{1, "temp", "", ""};
                                conflicts.push_back(v); // TODO: proper variable
                                deps[pred.id] = v;
                            }
                        }
                        if (everything_is_broken_stop_the_count)
                        {
                            continue;
                        }
                        p = create_phi_node(vr, lvr, g, conflicts, n, p, name);
                        if (created_node_id == "")
                            created_node_id = p.outs[0].id;

                        pending_phis[p.outs[0].id] = PendingPhi{
                            lvr,
                            name,
                            p,
                            n,
                            deps};
                        phis.push_back(Phi{
                            !is_pending,
                            p.outs[0].id,
                            lvr.get_variable(name), ready_ins});
                        for (const auto &c : ready_ins)
                        {
                            var2phis[c].insert(Phi{
                                !is_pending,
                                p.outs[0].id,
                                lvr.get_variable(name), ready_ins});
                        }
                        for (const auto &pred : preds[n.id])
                        {
                            if (!vars.contains(pred.id))
                                phi_watchers[pred.id].push_back(p.outs[0].id);
                        }
                    }
                }

                // for (const auto &id : pending_phi_ids)
                // {
                //     if (pending_phis.contains(id))
                //     {
                //         // pending_phis[id].push_back(PendingPhi{lvr, p, n});
                //     }
                //     else
                //     {
                //         // pending_phis[id] = {PendingPhi{lvr, p, n}};
                //     }
                // }

                // if (pending_edges.contains(n.id))
                // {
                //     for (const auto &e : pending_edges[n.id])
                //     {
                //         if (created_node_id != "")
                //         {
                //             g.edges.push_back(GraphEdge{e.id, created_node_id, e.comment, e.color});
                //         }
                //     }
                // }

                // P is the created phi here
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

                // The NEW label creation process
                labels[id] = {var, " = "};
                build_label_from_identifiers(lvr, rest, id, labels);
                if (rest.text() == "")
                {
                    labels[id].push_back("*uninitialized*");
                }

                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, edge.comment, edge.color});
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
                    text = replace_identifiers(lvr, postfix_expression.value());
                    labels[id] = {""};
                    build_label_from_identifiers(lvr, postfix_expression.value(), id, labels);
                }
                else // It is an assignment
                {
                    // Assumes left is identifier
                    std::string dest = std::string(assignment_expression.value().field("left").text());
                    std::string right = replace_identifiers(lvr, assignment_expression.value().field("right"));
                    labels[id] = {};
                    build_label_from_identifiers(lvr, assignment_expression.value().field("right"), id, labels);

                    const auto &v = vr.get_and_inc_variable(dest, n.id);
                    lvr.set_version(v.name, v.version, n.id);
                    text = lvr.get_variable(dest).to_string() + "=" + right;
                    // labels[id] = {lvr.get_variable(dest), "="};
                    labels[id].insert(labels[id].begin(), "=");
                    labels[id].insert(labels[id].begin(), lvr.get_variable(dest));
                }
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, edge.comment, edge.color});
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
                labels[id] = {};
                build_label_from_identifiers(lvr, assignment_expression.field("right"), id, labels);

                const auto v = vr.get_and_inc_variable(dest, id);
                lvr.set_version(dest, v.version, id);
                text = lvr.get_variable(dest).to_string() + "=" + right;
                // labels[id] = {lvr.get_variable(dest), "="};
                labels[id].insert(labels[id].begin(), "=");
                labels[id].insert(labels[id].begin(), lvr.get_variable(dest));

                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, edge.comment, edge.color});
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
                labels[id] = {""};

                for (const auto &child : n.syntax_node.value().children())
                {
                    if (child.text() != ";")
                    {
                        text += replace_identifiers(lvr, child) + " ";
                        build_label_from_identifiers(lvr, child, id, labels);
                        labels[id].push_back(" ");
                    }
                }
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, edge.comment, edge.color});
                }
                new_parent = NodeResult{{ParentEdge{id}}, {}};
                if (created_node_id == "")
                    created_node_id = id;
            }
            else if (n.kind == "postfix_unary_expression")
            {
                if (!n.syntax_node.has_value())
                {
                    ssa.diagnostics.push_back(Diagnostic{"ssa_no_syntax", "Found postfix unary expression without syntax node", n.range.value_or(SourceRange{})});
                }

                std::string dest = std::string(n.syntax_node.value().child(0).text());
                std::string text;
                std::string operation = std::string(n.syntax_node.value().child(1).text());
                std::string id = "n" + std::to_string(g.nodes.size());
                const auto v = vr.get_and_inc_variable(dest, id);
                lvr.set_version(dest, v.version, id);
                text = lvr.get_variable(dest).to_string() + operation;

                labels[id] = {lvr.get_variable(dest), operation};
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, edge.comment, edge.color});
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
                labels[id] = {""};
                build_label_from_identifiers(lvr, n.syntax_node.value(), id, labels);
                g.nodes.push_back(GraphNode{id, n.kind, text, n.range, n.syntax_node});
                for (const auto &edge : p.outs)
                {
                    g.edges.push_back(GraphEdge{edge.id, id, edge.comment, edge.color});
                }
                new_parent = NodeResult{{ParentEdge{id}}, {}}; // TODO: Not necessarily right
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
            if (phi_watchers.contains(n.id))
            {
                for (const auto &phi_id : phi_watchers[n.id])
                {
                    PendingPhi &phi = pending_phis[phi_id];
                    if (lvr.has_variable(phi.var)) // If not, this phi is actually broken and should be nuked :(
                    {

                        phi.deps[n.id] = lvr.get_variable(phi.var);
                        // TODO: implement function which would re-generate phi's text based on phi.deps
                        update_phi_node(g, phi_id, phi);

                        // Update phi ins
                        for (auto &existing_phi : phis)
                        {
                            if (existing_phi.id == phi_id)
                            {
                                existing_phi.ins.push_back(lvr.get_variable(phi.var));
                                var2phis[lvr.get_variable(phi.var)].insert(existing_phi);
                            }
                        }
                    }
                    else
                    {
                        // This phi is invalid and merges a variable from a context where it does not exist
                        for (auto &existing_phi : phis)
                        {
                            if (existing_phi.id == phi_id)
                            {
                                removed_phis.insert(existing_phi);
                            }
                        }
                    }
                }
            }

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
                        {
                            NodeResult n_p;
                            for (const auto &o : new_parent.outs)
                            {
                                n_p.outs.push_back(ParentEdge{o.id, edge.label, edge.color});
                            }
                            n_p.lc = new_parent.lc;
                            visit(to.value(), n_p, depth + 1, lvr);
                        }
                        else
                        {
                            if (cfg2ssa.contains(edge.to))
                            {
                                for (const auto &out : new_parent.outs)
                                {
                                    g.edges.push_back(GraphEdge{out.id, cfg2ssa[edge.to], edge.label, edge.color});
                                }
                                // g.edges.push_back(GraphEdge{cfg2ssa[edge.from], cfg2ssa[edge.to], edge.label, edge.color});
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

        // Remove redundant phi nodes
        std::unordered_map<Variable, Variable> replacement;
        std::deque<Phi> q(phis.begin(), phis.end());
        while (!q.empty())
        {
            Phi phi = q.front();
            q.pop_front();
            if (removed_phis.contains(phi))
            {
                continue;
            }
            std::unordered_set<Variable> phi_variables;
            for (const auto &v : phi.ins)
            {
                if (get_replacement(v, replacement) != get_replacement(phi.out_var, replacement))
                {
                    phi_variables.insert(get_replacement(v, replacement));
                }
            }
            if (phi_variables.size() == 1)
            { // Redundant phi
                removed_phis.insert(phi);
                replacement[phi.out_var] = *phi_variables.begin();
                // Copy var2phis from being-phased-out out_var to *phi_variables.begin()
                for (const auto &var : var2phis[phi.out_var])
                {
                    var2phis[*phi_variables.begin()].insert(var);
                }
                for (const auto &nphi : var2phis[phi.out_var])
                {
                    for (const auto &up_to_date_phi : phis)
                    {
                        if (up_to_date_phi.id == nphi.id)
                        {
                            q.push_back(up_to_date_phi);
                        }
                    }
                    // q.push_back(nphi);
                }
            }
            else if (phi_variables.size() == 0)
            {
                // WTF
                // Somehow our only predecessor for this var is ourselves
            }
        }

        // Remove marked phis
        for (const auto &phi : removed_phis)
        {
            std::vector<GraphEdge> to;
            std::vector<GraphEdge> from;
            for (auto &edge : g.edges)
            {
                if (edge.to == phi.id)
                {
                    to.push_back(edge);
                }
                if (edge.from == phi.id)
                {
                    from.push_back(edge);
                }
            }

            // By this point, there should be either only 1 from, only 1 to, or 1 of both
            if (to.size() != 1 && from.size() != 1)
            {
                // Cannot eliminate many-to-many
                ssa.diagnostics.push_back(Diagnostic{"ssa-phi-elim-many-to-many", "Could not eliminate phi node: many to many relationship on a removed node", SourceRange{}});
            }

            for (const auto &t : to)
            {
                for (const auto &f : from)
                {
                    g.edges.push_back(GraphEdge{t.from, f.to, t.label, t.color}); // Losing label and color of phi->node edge, should be fine
                }
            }

            for (const auto &t : to)
            {
                std::vector<int> vec = {1, 2, 3, 4, 5, 6};

                std::erase_if(g.edges, [&](GraphEdge ge)
                              { return ge.to == t.to && ge.from == t.from; });

                // std::erase(g.edges, t);
            }
            for (const auto &f : from)
            {
                std::erase_if(g.edges, [&](GraphEdge ge)
                              { return ge.to == f.to && ge.from == f.from; });

                // std::erase(g.edges, from);
            }
            std::erase_if(g.nodes, [&](GraphNode n)
                          { return n.id == phi.id; });
        }

        // Render all labels

        for (auto &node : g.nodes)
        {
            node.label = render_node_label(node.id, labels, replacement, node.label);
            if (node.kind == "phi")
            {
                auto phi = std::find_if(phis.begin(), phis.end(), [&](Phi p)
                                        { return p.id == node.id; });
                if (phi != phis.end())
                {
                    node.label = build_phi_label(phi->out_var, phi->ins, replacement);
                }
            }
        }

        // ssa.graph = cfg;

        return ssa;
        // return {std::nullopt, {{"ssa_not_implemented", "SSA construction is not implemented yet.", {0, 0}}}};
    }

    Variable
    VariableRegistry::create_variable(std::string name, std::string node, std::string type)
    {
        Variable new_var{0, name, type, node};
        variables[name] = new_var;
        return new_var;
    }
    Variable VariableRegistry::get_variable(std::string name)
    {
        return variables[name];
    }
    bool VariableRegistry::has_variable(std::string name)
    {
        return variables.contains(name);
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
            if (!other.variables.contains(n))
            {
                continue; // Cannot have a conflict if a variable does not exist
            }
            if (v.version != other.get_version(n))
                res.push_back(std::pair{v, other.get_variable(n)});
        }
        return res;
    }

} // namespace lab
