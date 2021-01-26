
add_executable(intro main.cpp ../include/lex_item.hpp ../include/parse_node.hpp ../include/lexer.hpp ../include/parser.hpp ../include/thing.hpp ../include/algorithms.hpp ../include/ast.hpp ../include/containers.hpp)
target_link_libraries(
  intro
  PRIVATE project_options
          project_warnings
          CONAN_PKG::docopt.cpp
          CONAN_PKG::fmt
          CONAN_PKG::spdlog
          CONAN_PKG::ctre)
target_include_directories(intro PRIVATE "${CMAKE_SOURCE_DIR}/include")
