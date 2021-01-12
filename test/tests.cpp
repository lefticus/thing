#include <catch2/catch.hpp>
#include "../include/parser.hpp"
#include "../include/algorithms.hpp"
#include <fmt/format.h>

TEST_CASE("Can parse expressions with precendence")
{
  constexpr std::string_view str = "5 * 2 + 4 / 3";
  auto string_to_parse = str;

  parser_test::Parser parser;
  while (!string_to_parse.empty()) {
    auto parsed = parser.next_token(string_to_parse);
    if (parsed.type == parser_test::Parser::Type::unknown) {
      const auto [line, location] = parser_test::count_to_last(str.begin(), parsed.remainder.begin(), '\n');
      // skip last matched newline, and find next newline after
      const auto errored_line = std::string_view{ std::next(location), std::find(std::next(location), parsed.remainder.end(), '\n') };
      // count column from location to beginning of unmatched string
      const auto column = std::distance(std::next(location), parsed.remainder.begin());

      std::cout << fmt::format("Error parsing string ({},{})\n\n", line + 1, column + 1);
      std::cout << errored_line;
      std::cout << fmt::format("\n{:>{}}\n\n", '^', column + 1);
      return EXIT_FAILURE;
    }
    std::cout << '\'' << parsed.match << "' '" << parsed.remainder << "'\n";
    string_to_parse = parsed.remainder;
  }
}
