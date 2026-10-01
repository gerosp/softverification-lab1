#include "lab/protocol.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
namespace {
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
std::string read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot open example.");
    return {std::istreambuf_iterator<char>(stream), {}};
}
void check_ranges(lab::SyntaxNode n, const std::string& source) {
    auto r = n.range();
    check(r.start <= r.end && r.end <= source.size(), "Range outside user source");
    check(n.text() == std::string_view(source).substr(r.start, r.end - r.start), "Range/text mismatch");
    for (auto c : n.children()) check_ranges(c, source);
}
void examples(const std::filesystem::path& dir) {
    const std::vector<std::pair<std::string, std::vector<std::string>>> cases{
        {"01-sequence.cs", {"local_declaration_statement", "assignment_expression", "binary_expression", "+", "*", "-"}},
        {"02-if-else.cs", {"if_statement", "prefix_unary_expression"}},
        {"03-nested-if.cs", {"if_statement", "return_statement", "!"}},
        {"04-while.cs", {"while_statement", "assignment_expression"}},
        {"05-for.cs", {"for_statement", "if_statement", "postfix_unary_expression", "++", "+="}},
        {"06-do-while.cs", {"do_statement", "&&", "||"}},
        {"07-nested-loops.cs", {"for_statement", "while_statement", "break_statement", "continue_statement"}}
    };
    for (const auto& [name, expected] : cases) {
        auto source = read(dir / name);
        auto parsed = lab::parse_method(source);
        if (!parsed.ok()) {
            for (const auto& d : parsed.diagnostics) std::cerr << name << ": " << d.message << '\n';
            throw std::runtime_error("Example did not parse.");
        }
        check(parsed.method->root().kind() == "method_declaration", "Synthetic class leaked");
        check(parsed.method->root().field("body").kind() == "block", "Missing body field");
        check(parsed.method->root().field("name").kind() == "identifier", "Missing name field");
        check_ranges(parsed.method->root(), source);
        auto graph = lab::syntax_graph(*parsed.method);
        std::unordered_set<std::string> kinds;
        for (const auto& node : graph.nodes) kinds.insert(node.kind);
        for (const auto& kind : expected) {
            if (!kinds.contains(kind)) throw std::runtime_error(name + ": missing " + kind);
        }
        check(lab::to_dot(graph) == lab::to_dot(lab::syntax_graph(*parsed.method)), "Non-deterministic DOT");
    }
}
void invalid() {
    for (const auto& source : {"", "int F() {", "int F() { return 1 }", "int F() { return 1; } int G() { return 2; }",
                             "int F() => 1;", "class A { int F() { return 1; } }", "int x;"}) {
        auto parsed = lab::parse_method(source);
        check(!parsed.ok() && !parsed.diagnostics.empty(), "Invalid method accepted");
    }
    auto incomplete = lab::parse_method("int F() { return 1 }");
    check(incomplete.method != nullptr, "No partial method tree");
    bool missing = false;
    for (const auto& d : incomplete.diagnostics) missing |= d.code == "missing_token";
    check(missing, "Missing token not reported");
    auto missing_brace = lab::parse_method("int F() { return 1;");
    check(!missing_brace.ok(), "Synthetic suffix hid a missing brace");
    auto big = lab::parse_method(std::string(lab::max_source_bytes + 1, ' '));
    check(!big.ok(), "Oversized source accepted");
}
void unicode() {
    // Explicit UTF-8 bytes keep this test independent of the compiler source encoding.
    std::string source = "// \xf0\x9f\x98\x80\r\nint \xd0\xa2\xd0\xb5\xd1\x81\xd1\x82(int x)\r\n{\r\n    return x + 1;\r\n}\r\n";
    auto parsed = lab::parse_method(source);
    check(parsed.ok(), "Unicode/CRLF did not parse");
    check_ranges(parsed.method->root(), source);
    auto name = parsed.method->root().field("name");
    check(name.range().start == source.find("\xd0\xa2"), "Offset includes synthetic prefix");
    auto body = parsed.method->root().field("body");
    check(body.range().start == source.find('{'), "CRLF changed offsets");
    auto json = lab::syntax_json(parsed.method->root());
    check(!json.dump().empty(), "Invalid JSON UTF-8");
}
void protocol() {
    auto request = nlohmann::json{{"protocolVersion", 1}, {"requestId", 42}, {"source", "int F() { return 1; }"}, {"view", "syntax"}};
    auto response = lab::handle_request(request);
    check(response["requestId"] == 42 && lab::response_exit_code(response) == 0, "Protocol success failed");
    check(response["nodeRanges"].contains("n0"), "Missing source map");
    request["view"] = "cfg";
    auto cfg = lab::handle_request(request);
    check(cfg["requestId"] == 42 && cfg["status"] == "error", "CFG placeholder response");
    check(cfg["diagnostics"][0]["code"] == "cfg_not_implemented", "CFG pass was not dispatched");
    check(cfg["dot"].is_null() && cfg["nodeRanges"].empty(), "CFG placeholder leaked the AST");
    request["source"] = "int F() { return 1 }";
    auto invalid_cfg = lab::handle_request(request);
    check(invalid_cfg["status"] == "syntax_error" && invalid_cfg["dot"].is_null(), "Invalid AST reached CFG pass");
    check(invalid_cfg["diagnostics"][0]["code"] != "cfg_not_implemented", "Parser diagnostics overwritten by CFG");
    request["view"] = "ssa";
    check(lab::response_exit_code(lab::handle_request(request)) == 2, "Unsupported view accepted");
    request["view"] = "syntax";
    request["source"] = "int F() {";
    check(lab::response_exit_code(lab::handle_request(request)) == 1, "Syntax error exit code");
    check(lab::response_exit_code(lab::handle_request(nullptr)) == 2, "Invalid envelope accepted");
    lab::Graph graph{{{"a", "test", "quote \" backslash \\ newline\nnext", std::nullopt}}, {}};
    auto dot = lab::to_dot(graph);
    check(dot.find("\\\"") != std::string::npos && dot.find("\\\\") != std::string::npos && dot.find("\\n") != std::string::npos, "DOT escaping failed");
    check(lab::to_dot(graph, "CFG").starts_with("digraph \"CFG\""), "CFG DOT name missing");
}
}
int main(int argc, char** argv) {
    try {
        check(argc == 2, "Expected example directory");
        examples(argv[1]); invalid(); unicode(); protocol();
        std::cout << "All core tests passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
