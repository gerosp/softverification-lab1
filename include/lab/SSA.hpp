#pragma once
#include <unordered_map>
#include "lab/graph.hpp"
#include "lab/cfg.hpp"

namespace lab
{
    struct SsaResult
    {
        std::optional<Graph> graph;
        std::vector<Diagnostic> diagnostics;
    };

    // Precondition: method parsed successfully and cfg is the corresponding CFG.
    // Graph is currently a presentation boundary, not a semantic SSA representation.
    // The student implementation may introduce blocks, symbols, and versions later.
    // Success: an owned graph and no diagnostics. Failure: nullopt and diagnostics.
    SsaResult build_ssa(const ParsedMethod &method, const Graph &cfg);

    class Variable
    {
    public:
        std::size_t version;
        std::string name;
        std::string type;
        std::string created_at;
        std::string to_string()
        {
            return name + "." + std::to_string(version);
        }
    };
    class VariableRegistry
    {
    private:
        std::unordered_map<std::string, Variable> variables;

    public:
        Variable get_variable(std::string name);
        std::size_t get_version(std::string name);
        void set_version(std::string name, std::size_t version);
        Variable create_variable(std::string name, std::string node, std::string type = "");
        Variable get_and_inc_variable(std::string name, std::string node);
        std::vector<std::pair<Variable, Variable>> get_conflicting_variables(VariableRegistry &other);
    };

    struct PendingPhi
    {
        // 1. Our vr
        VariableRegistry vr;

        // 2. Parent for the phi node
        NodeResult p;
        // 3. Successor for the phi node (do ourselves from g.edges)
        // 4. This graph node
        GraphNode n;
    };

} // namespace lab