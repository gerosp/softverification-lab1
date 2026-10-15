#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <tree_sitter/api.h>

namespace lab
{
    inline constexpr std::size_t max_source_bytes = 256 * 1024;
    inline constexpr unsigned max_tree_depth = 256;
    struct SourceRange
    {
        std::uint32_t start;
        std::uint32_t end;
    };
    struct Diagnostic
    {
        std::string code;
        std::string message;
        SourceRange range;
    };
    class ParsedMethod;
    // Non-owning view. The ParsedMethod must outlive all views (and must not move).
    class SyntaxNode
    {
    public:
        bool valid() const;
        std::string_view kind() const;
        bool named() const;
        bool missing() const;
        bool error() const;
        SourceRange range() const;
        std::string_view text() const;
        std::uint32_t child_count() const;
        SyntaxNode child(std::uint32_t index) const;
        SyntaxNode field(std::string_view name) const;
        std::string_view child_field(std::uint32_t index) const;
        std::vector<SyntaxNode> children(bool named_only = false) const;
        SyntaxNode(TSNode node, const ParsedMethod *owner) : node_(node), owner_(owner) {}

    private:
        friend class ParsedMethod;
        TSNode node_{};
        const ParsedMethod *owner_{};
    };
    class ParsedMethod
    {
    public:
        ParsedMethod(std::string source, std::string wrapped, TSTree *tree, TSNode root, std::uint32_t prefix);
        ~ParsedMethod();
        ParsedMethod(const ParsedMethod &) = delete;
        ParsedMethod &operator=(const ParsedMethod &) = delete;
        ParsedMethod(ParsedMethod &&) = delete;
        ParsedMethod &operator=(ParsedMethod &&) = delete;
        SyntaxNode root() const;
        std::string_view source() const { return source_; }
        SourceRange user_range(TSNode node) const;

    private:
        std::string source_;
        std::string wrapped_;
        TSTree *tree_;
        TSNode root_;
        std::uint32_t prefix_;
    };
    struct ParseResult
    {
        std::unique_ptr<ParsedMethod> method;
        std::vector<Diagnostic> diagnostics;
        bool ok() const { return diagnostics.empty() && method != nullptr; }
    };
    ParseResult parse_method(std::string source);
} // namespace lab
