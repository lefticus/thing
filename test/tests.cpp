#include <catch2/catch.hpp>
#include "../include/parser.hpp"
#include "../include/algorithms.hpp"

TEST_CASE("Can parse expressions with precendence")
{
  constexpr std::string_view str = "5 * 2 + 4 / 3";
  thing::parsing::basic_parser<std::vector> parser;
  const auto result = parser.parse(str);

  CHECK(result.item.match == "+");
  REQUIRE(result.children.size() == 2);
  CHECK(result.children[0].item.match == "*");
  REQUIRE(result.children[0].children.size() == 2);
  CHECK(result.children[0].children[0].item.match == "5");
  CHECK(result.children[0].children[1].item.match == "2");
  CHECK(result.children[1].item.match == "/");
  REQUIRE(result.children[1].children.size() == 2);
  CHECK(result.children[1].children[0].item.match == "4");
  CHECK(result.children[1].children[1].item.match == "3");
}
