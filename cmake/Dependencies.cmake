include(FetchContent)
# Release assets include generated C sources. No tree-sitter CLI/Rust is needed.
FetchContent_Declare(tree_sitter
  URL https://codeload.github.com/tree-sitter/tree-sitter/tar.gz/refs/tags/v0.25.10
  URL_HASH SHA256=ad5040537537012b16ef6e1210a572b927c7cdc2b99d1ee88d44a7dcdc3ff44c
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_SUBDIR unused)
FetchContent_Declare(csharp
  URL https://github.com/tree-sitter/tree-sitter-c-sharp/releases/download/v0.23.1/tree-sitter-c-sharp.tar.xz
  URL_HASH SHA256=091b700c852ec39c9253ad22ea50198567ede167afddedbcc6a8080a7148090b
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_SUBDIR unused)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tree_sitter csharp json)
add_library(tree_sitter_runtime STATIC "${tree_sitter_SOURCE_DIR}/lib/src/lib.c")
target_include_directories(tree_sitter_runtime PUBLIC "${tree_sitter_SOURCE_DIR}/lib/include"
  PRIVATE "${tree_sitter_SOURCE_DIR}/lib/src")
set_target_properties(tree_sitter_runtime PROPERTIES C_STANDARD 11)
if(NOT WIN32)
  target_compile_definitions(tree_sitter_runtime PRIVATE _POSIX_C_SOURCE=200112L _DEFAULT_SOURCE)
endif()
add_library(csharp_grammar STATIC "${csharp_SOURCE_DIR}/src/parser.c" "${csharp_SOURCE_DIR}/src/scanner.c")
target_include_directories(csharp_grammar PRIVATE "${csharp_SOURCE_DIR}/src")
set_target_properties(csharp_grammar PROPERTIES C_STANDARD 11)
