#pragma once
#include "lab/graph.hpp"

namespace lab {
struct SsaResult {
    std::optional<Graph> graph;
    std::vector<Diagnostic> diagnostics;
};

// Precondition: method parsed successfully and cfg is the corresponding CFG.
// Graph is currently a presentation boundary, not a semantic SSA representation.
// The student implementation may introduce blocks, symbols, and versions later.
// Success: an owned graph and no diagnostics. Failure: nullopt and diagnostics.
SsaResult build_ssa(const ParsedMethod& method, const Graph& cfg);
} // namespace lab
