#pragma once
#include "lab/graph.hpp"

namespace lab
{
    // Presentation boundary for the AST -> CFG pass. Keep semantic blocks and
    // instructions separately when implementing the pass / adding SSA.
    struct CfgResult
    {
        std::optional<Graph> graph;
        std::vector<Diagnostic> diagnostics;
    };

    enum class IfState
    {
        CONDITION,
        CONSEQUENCE,
        ALTERNATIVE
    };

    struct IfBlock
    {
        const SyntaxNode& condition;
        const SyntaxNode& consequence;
        const SyntaxNode& alternative;
        const GraphNode phi;
        GraphNode end;
        IfState state;
    };
    // Precondition: method comes from a ParseResult with ok() == true.
    // Success: graph has a value and diagnostics is empty.
    // Failure: graph is nullopt and diagnostics explains unsupported input, etc.
    // Return owned strings/ranges; do not retain SyntaxNode/string_view references.
    CfgResult build_cfg(const ParsedMethod &method);
} // namespace lab
