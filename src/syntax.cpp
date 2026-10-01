#include "lab/syntax.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

extern "C" const TSLanguage* tree_sitter_c_sharp();
namespace lab {
bool SyntaxNode::valid() const { return !ts_node_is_null(node_); }
std::string_view SyntaxNode::kind() const { return valid() ? ts_node_type(node_) : ""; }
bool SyntaxNode::named() const { return valid() && ts_node_is_named(node_); }
bool SyntaxNode::missing() const { return valid() && ts_node_is_missing(node_); }
bool SyntaxNode::error() const { return valid() && ts_node_is_error(node_); }
SourceRange SyntaxNode::range() const { return valid() ? owner_->user_range(node_) : SourceRange{0, 0}; }
std::string_view SyntaxNode::text() const {
    auto r = range();
    return owner_->source().substr(r.start, r.end - r.start);
}
std::uint32_t SyntaxNode::child_count() const { return valid() ? ts_node_child_count(node_) : 0; }
SyntaxNode SyntaxNode::child(std::uint32_t index) const {
    return SyntaxNode(valid() ? ts_node_child(node_, index) : TSNode{}, owner_);
}
SyntaxNode SyntaxNode::field(std::string_view name) const {
    return SyntaxNode(valid() ? ts_node_child_by_field_name(node_, name.data(), static_cast<std::uint32_t>(name.size())) : TSNode{}, owner_);
}
std::string_view SyntaxNode::child_field(std::uint32_t index) const {
    auto f = valid() ? ts_node_field_name_for_child(node_, index) : nullptr;
    return f ? f : "";
}
std::vector<SyntaxNode> SyntaxNode::children(bool named_only) const {
    std::vector<SyntaxNode> result;
    for (std::uint32_t i = 0; i < child_count(); ++i) {
        auto n = child(i);
        if (!named_only || n.named()) result.push_back(n);
    }
    return result;
}
ParsedMethod::ParsedMethod(std::string source, std::string wrapped, TSTree* tree, TSNode root, std::uint32_t prefix)
    : source_(std::move(source)), wrapped_(std::move(wrapped)), tree_(tree), root_(root), prefix_(prefix) {}
ParsedMethod::~ParsedMethod() { ts_tree_delete(tree_); }
SyntaxNode ParsedMethod::root() const { return SyntaxNode(root_, this); }
SourceRange ParsedMethod::user_range(TSNode node) const {
    auto map = [this](std::uint32_t pos) {
        return static_cast<std::uint32_t>(std::min<std::size_t>(pos > prefix_ ? pos - prefix_ : 0, source_.size()));
    };
    return {map(ts_node_start_byte(node)), map(ts_node_end_byte(node))};
}
ParseResult parse_method(std::string source) {
    ParseResult result;
    if (source.size() > max_source_bytes) {
        result.diagnostics.push_back({"source_too_large", "Source exceeds 256 KiB.", {0, 0}});
        return result;
    }
    const std::string prefix = "class __LabInput__ {\n";
    auto wrapped = prefix + source + "\n}";
    auto parser = std::unique_ptr<TSParser, decltype(&ts_parser_delete)>(ts_parser_new(), ts_parser_delete);
    if (!parser || !ts_parser_set_language(parser.get(), tree_sitter_c_sharp()))
        throw std::runtime_error("Cannot initialize C# parser (check Tree-sitter ABI).");
    auto tree = std::unique_ptr<TSTree, decltype(&ts_tree_delete)>(
        ts_parser_parse_string(parser.get(), nullptr, wrapped.data(), static_cast<std::uint32_t>(wrapped.size())), ts_tree_delete);
    if (!tree) throw std::runtime_error("Parser returned no syntax tree.");
    auto file = ts_tree_root_node(tree.get());
    TSNode container{};
    for (std::uint32_t i = 0; i < ts_node_named_child_count(file); ++i) {
        auto n = ts_node_named_child(file, i);
        if (std::string_view(ts_node_type(n)) == "class_declaration") { container = n; break; }
    }
    TSNode method{};
    unsigned method_count = 0;
    bool other_member = false;
    if (!ts_node_is_null(container)) {
        auto body = ts_node_child_by_field_name(container, "body", 4);
        for (std::uint32_t i = 0; i < ts_node_named_child_count(body); ++i) {
            auto n = ts_node_named_child(body, i);
            std::string_view type = ts_node_type(n);
            if (type == "method_declaration") {
                ++method_count;
                if (ts_node_is_null(method)) method = n;
            } else if (type != "comment") other_member = true;
        }
    }
    // Only user content is exported. Never expose the synthetic class as a method.
    if (!ts_node_is_null(method)) {
        result.method = std::make_unique<ParsedMethod>(source, wrapped, tree.get(), method, static_cast<std::uint32_t>(prefix.size()));
        tree.release();
    }
    auto range = [&](TSNode n) -> SourceRange {
        auto map = [&](std::uint32_t p) { return static_cast<std::uint32_t>(std::min<std::size_t>(p > prefix.size() ? p - prefix.size() : 0, source.size())); };
        return {map(ts_node_start_byte(n)), map(ts_node_end_byte(n))};
    };
    if (method_count != 1 || other_member)
        result.diagnostics.push_back({"expected_one_method", "Enter exactly one method with a signature and block body (no class or other members).", {0, static_cast<std::uint32_t>(source.size())}});
    if (!ts_node_is_null(method)) {
        auto body = ts_node_child_by_field_name(method, "body", 4);
        if (ts_node_is_null(body) || std::string_view(ts_node_type(body)) != "block")
            result.diagnostics.push_back({"expected_block_body", "The method must have a block body: { ... }.", range(method)});
    }
    // Walk the complete parse, including the synthetic suffix: a missing user '}'
    // can otherwise be hidden by the closing brace supplied by the wrapper.
    std::vector<std::pair<TSNode, unsigned>> stack{{file, 0}};
    while (!stack.empty()) {
        auto [n, depth] = stack.back(); stack.pop_back();
        if (depth > max_tree_depth) {
            result.diagnostics.push_back({"tree_too_deep", "Syntax nesting exceeds the viewer limit of 256 levels.", range(n)});
            break;
        }
        if (ts_node_is_error(n)) result.diagnostics.push_back({"syntax_error", "Unexpected or incomplete syntax.", range(n)});
        if (ts_node_is_missing(n)) result.diagnostics.push_back({"missing_token", std::string("Expected ") + ts_node_type(n) + ".", range(n)});
        for (auto i = ts_node_child_count(n); i > 0; --i) stack.emplace_back(ts_node_child(n, i - 1), depth + 1);
    }
    // Reject valid-looking text which escaped the synthetic class with extra braces.
    for (std::uint32_t i = 0; i < ts_node_named_child_count(file); ++i) {
        auto n = ts_node_named_child(file, i);
        if (!ts_node_eq(n, container) && std::string_view(ts_node_type(n)) != "comment")
            result.diagnostics.push_back({"outside_method", "Unexpected content outside the method.", range(n)});
    }
    return result;
}
} // namespace lab
