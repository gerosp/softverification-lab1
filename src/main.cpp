#include "lab/protocol.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
namespace {
std::string read_all(std::istream& input, std::size_t limit) {
    std::string data;
    char buffer[4096];
    while (input.read(buffer, sizeof(buffer)) || input.gcount()) {
        data.append(buffer, static_cast<std::size_t>(input.gcount()));
        if (data.size() > limit) throw std::runtime_error("Input exceeds size limit.");
    }
    if (input.bad()) throw std::runtime_error("Cannot read input.");
    return data;
}
}
int main(int argc, char** argv) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    bool request_mode = argc > 1 && std::string_view(argv[1]) == "--request-json";
    try {
        if (request_mode) {
            if (argc != 2) throw std::runtime_error("--request-json takes no other arguments.");
            auto response = lab::handle_request(nlohmann::json::parse(read_all(std::cin, 2 * 1024 * 1024)));
            std::cout << response.dump() << '\n';
            return lab::response_exit_code(response);
        }
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout << "lab-model parse --input <file|-> --format <json|dot|sexp> [--view syntax|cfg]\n"
                         "  Default view: syntax (AST). sexp is only available for syntax.\nlab-model --request-json\n";
            return 0;
        }
        if (argc < 2 || std::string_view(argv[1]) != "parse") throw std::runtime_error("Use --help for usage.");
        std::string input, format = "json", view = "syntax";
        for (int i = 2; i < argc; i += 2) {
            if (i + 1 >= argc) throw std::runtime_error("Missing option value.");
            std::string_view arg = argv[i];
            if (arg == "--input") input = argv[i + 1];
            else if (arg == "--format") format = argv[i + 1];
            else if (arg == "--view") view = argv[i + 1];
            else throw std::runtime_error("Unknown option: " + std::string(arg));
        }
        if (input.empty()) throw std::runtime_error("--input is required.");
        if (format != "json" && format != "dot" && format != "sexp") throw std::runtime_error("Unknown format: " + format);
        if (view != "syntax" && view != "cfg") throw std::runtime_error("Unknown view: " + view);
        if (view == "cfg" && format == "sexp") throw std::runtime_error("sexp is only available for view 'syntax'; use json or dot for CFG.");
        std::string source;
        if (input == "-") source = read_all(std::cin, lab::max_source_bytes);
        else {
            std::ifstream file(input, std::ios::binary);
            if (!file) throw std::runtime_error("Cannot open input file: " + input);
            source = read_all(file, lab::max_source_bytes);
        }
        if (format == "json" || format == "dot") {
            auto response = lab::analyze_response(std::move(source), nullptr, format == "json", view);
            if (format == "json") std::cout << response.dump(2) << '\n';
            else {
                if (response["dot"].is_string()) std::cout << response["dot"].get<std::string>();
                for (const auto& d : response["diagnostics"])
                    std::cerr << d["code"].get<std::string>() << " [" << d["range"]["start"] << "," << d["range"]["end"] << "): "
                              << d["message"].get<std::string>() << '\n';
            }
            return lab::response_exit_code(response);
        }
        auto result = lab::parse_method(std::move(source));
        if (result.method) {
            std::cout << lab::syntax_sexp(result.method->root()) << '\n';
        }
        for (const auto& d : result.diagnostics)
            std::cerr << d.code << " [" << d.range.start << "," << d.range.end << "): " << d.message << '\n';
        return result.ok() ? 0 : 1;
    } catch (const std::exception& e) {
        if (request_mode) std::cout << lab::error_response(nullptr, "request_error", e.what()).dump() << '\n';
        std::cerr << e.what() << '\n';
        return 2;
    }
}
