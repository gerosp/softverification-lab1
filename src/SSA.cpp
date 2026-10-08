#include "lab/SSA.hpp"

namespace lab
{
    SsaResult build_ssa(const ParsedMethod &method, const Graph &cfg)
    {
        SsaResult ssa;

        GraphNode start;
        for (const auto &node : cfg.nodes)
        {
            if (node.id == "start")
            {
                start = node;
                break;
            }
        }

        // ssa.graph = cfg;
        // TODO: implement SSA on top of the method's control-flow graph.

        // return ssa;
        return {std::nullopt, {{"ssa_not_implemented", "SSA construction is not implemented yet.", {0, 0}}}};
    }
} // namespace lab
