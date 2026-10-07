#include "lab/SSA.hpp"

namespace lab {
SsaResult build_ssa(const ParsedMethod&, const Graph&) {
    // Student TODO: implement SSA on top of the method's control-flow graph.
    return {std::nullopt, {{"ssa_not_implemented", "SSA construction is not implemented yet.", {0, 0}}}};
}
} // namespace lab
