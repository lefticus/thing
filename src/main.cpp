#include <functional>
#include <iostream>

#include <spdlog/spdlog.h>


#include <docopt/docopt.h>


#include <thing.hpp>
#include <parser.hpp>
#include <algorithms.hpp>


static constexpr auto USAGE =
  R"(Parser Test.

    Usage:
          parser_test [options]

 Options:
          -h --help     Show this screen.
          --version     Show version.
)";




void print(std::uint64_t val0) { std::cout << "value0 is: " << val0 << '\n'; }

auto run_summation()
{
  using Stack_Type =
    Stack<std::monostate, RelativeStackReference, std::uint64_t, std::int64_t, std::int32_t, double, Function, bool, std::string>;

  Stack_Type stack;
  Operations<Stack_Type> ops;
  using PushLiteral = PushLiteral<Stack_Type::value_type>;

  // setup variables
  ops.push_back(PushLiteral{ 1ull });// count variable
  ops.push_back(PushLiteral{ 20001ull });// end value
  ops.push_back(PushLiteral{ 0ull });// accumulator

  ops.push_back(PushLiteral{ std::monostate{} });// temporary space for results

  // Test to see if count and end are the same yet
  ops.push_back(PushLiteral{ Function{
    &execBinaryOp<Stack_Type, std::uint64_t, std::int64_t> } });// function to call
  ops.push_back(PushLiteral{
    RelativeStackReference{ 2 } });// reference to results for return value
  ops.push_back(PushLiteral{ static_cast<std::uint64_t>(BinaryOps::Equal_To) });
  ops.push_back(
    PushLiteral{ RelativeStackReference{ 7 } });// reference to count variable
  ops.push_back(
    PushLiteral{ RelativeStackReference{ 7 } });// reference to end value
  ops.push_back(PushLiteral{ 3ull });// arity
  ops.push_back(CallFunction{ true });

  // If they are, jump over loop body
  ops.push_back(RelativeJump{ true, 14 });

  // reuse result space from Equal_To

  // Loop body, accumulate current value into accumulator
  ops.push_back(PushLiteral{ Function{
    &execBinaryOp<Stack_Type, std::uint64_t, std::int64_t> } });// function to call
  ops.push_back(
    PushLiteral{ RelativeStackReference{ 3 } });// reference to accumulator
  ops.push_back(
    PushLiteral{ static_cast<std::uint64_t>(BinaryOps::Addition) });// param 1
  ops.push_back(
    PushLiteral{ RelativeStackReference{ 7 } });// reference to count variable
  ops.push_back(PushLiteral{
    RelativeStackReference{ 6 } });// reference to accumulator variable
  ops.push_back(PushLiteral{ 3ull });// arity
  ops.push_back(CallFunction{ true });

  // Loop incrementer
  ops.push_back(PushLiteral{ Function{
    &execUnaryOp<Stack_Type, std::uint64_t, std::int64_t> } });// function to call
  ops.push_back(
    PushLiteral{ std::monostate{} });// we don't care about return value
  ops.push_back(
    PushLiteral{ static_cast<std::uint64_t>(UnaryOps::Pre_Increment) });
  ops.push_back(
    PushLiteral{ RelativeStackReference{ 7 } });// reference to count variable
  ops.push_back(PushLiteral{ 2ull });// arity
  ops.push_back(CallFunction{ true });

  // jump back to top of loop body
  ops.push_back(RelativeJump{ false, -22 });

  // pop temporary value used for loop test
  ops.push_back(Pop{});

  // print top value
  ops.push_back(
    PushLiteral{ Operations<Stack_Type>::template make_function<print>() });
  ops.push_back(PushLiteral{ std::monostate{} });// return value location
  ops.push_back(
    PushLiteral{ RelativeStackReference{ 3 } });// reference to previous top
  ops.push_back(PushLiteral{ 1ull });// arity
  ops.push_back(CallFunction{ true });

  while (ops.next(stack)) {
    // run while we can
  }
}

void dump(const parser_test::Parser::parse_tree &node, const std::size_t indent=0)
{
  fmt::print("{}'{}'\n", std::string(indent, ' '), node.item.match);

    for (const auto &child : node.children) {
      dump(child, indent+2);
    }
}

int main(int argc, const char **argv)
{
  const auto args = docopt::docopt(USAGE,
    { std::next(argv), std::next(argv, argc) },
    true,// show help if requested
    "Parser Test 0.0");// version string

//  for (auto const &arg : args) {
//    fmt::print("Command line arg: '{}': '{}'\n", arg.first, arg.second);
//  }


//    constexpr std::string_view str{"3 * (2+-4)^4 + 3! - 123.1"};
  constexpr std::string_view str{ "auto func(x,a*(2/z+q),d,b)" };
  auto string_to_parse = str;

  parser_test::Parser parser;
  while (!string_to_parse.empty()) {
    auto parsed = parser.next_token(string_to_parse);
    if (parsed.type == parser_test::Parser::Type::unknown) {
      const auto [line, location] = parser_test::count_to_last(str.begin(), parsed.remainder.begin(), '\n');
      // skip last matched newline, and find next newline after
      const auto errored_line = std::string_view{std::next(location), std::find(std::next(location), parsed.remainder.end(), '\n')};
      // count column from location to beginning of unmatched string
      const auto column = std::distance(std::next(location), parsed.remainder.begin());

      std::cout << fmt::format("Error parsing string ({},{})\n\n", line+1, column+1);
      std::cout << errored_line;
      std::cout << fmt::format("\n{:>{}}\n\n", '^', column+1);
      return EXIT_FAILURE;
    }
    std::cout << '\'' << parsed.match << "' '" << parsed.remainder << "'\n";
    string_to_parse = parsed.remainder;
  }

  //fmt::print(" {} = {} \n", str, parser.parse(str));


  auto result = parser.parse(str);
  dump(result);

  dump(parser.parse("auto x{15}"));
//  constexpr std::string_view str{ "auto func(x,a*(2/z+q),d,b)" };

  fmt::print("\n");


  run_summation();

}
