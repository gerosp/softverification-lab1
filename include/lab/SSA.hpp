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

        bool operator==(const Variable &other) const
        {
            return this->name == other.name && this->version == other.version;
        }
    };
    class VariableRegistry
    {
    private:
    public:
        std::unordered_map<std::string, Variable> variables;
        bool has_variable(std::string name);

        Variable get_variable(std::string name);
        std::size_t get_version(std::string name);
        void set_version(std::string name, std::size_t version, std::string node);
        Variable create_variable(std::string name, std::string node, std::string type = "");
        Variable get_and_inc_variable(std::string name, std::string node);
        std::vector<std::pair<Variable, Variable>> get_conflicting_variables(VariableRegistry &other);
    };

    struct PendingPhi
    {
        // 1. Our vr
        VariableRegistry vr;
        // Variable being merged
        std::string var;
        // 2. Parent for the phi node
        NodeResult p;
        // 3. Successor for the phi node (do ourselves from g.edges)
        // 4. This graph node
        GraphNode n;
        std::unordered_map<std::string, Variable> deps;
    };

    struct Phi
    {
        bool complete;
        std::string id;
        Variable out_var;
        std::vector<Variable> ins;
        bool operator==(const Phi &other) const
        {
            return this->id == other.id;
        }
    };

} // namespace lab

namespace std
{
    template <>
    struct hash<lab::Variable>
    {
        std::size_t operator()(const lab::Variable &v) const
        {
            return std::hash<int>{}(v.version) ^ (std::hash<std::string>{}(v.name) << 1);
        }
    };
    template <>
    struct hash<lab::Phi>
    {
        std::size_t operator()(const lab::Phi &p) const
        {
            return (std::hash<std::string>{}(p.id) << 1);
        }
    };
}