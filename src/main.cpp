#include <functional>
#include <iostream>

#include <spdlog/spdlog.h>


#include <docopt/docopt.h>

#include <iostream>

#include <thing.hpp>

static constexpr auto USAGE =
  R"(Naval Fate.

    Usage:
          naval_fate ship new <name>...
          naval_fate ship <name> move <x> <y> [--speed=<kn>]
          naval_fate ship shoot <x> <y>
          naval_fate mine (set|remove) <x> <y> [--moored | --drifting]
          naval_fate (-h | --help)
          naval_fate --version
 Options:
          -h --help     Show this screen.
          --version     Show version.
          --speed=<kn>  Speed in knots [default: 10].
          --moored      Moored (anchored) mine.
          --drifting    Drifting mine.
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

int main([[maybe_unused]] int argc, [[maybe_unused]] const char **argv)
{
  //std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
  //  { std::next(argv), std::next(argv, argc) },
  //  true,// show help if requested
  //  "Naval Fate 2.0");// version string

  //Use the default logger (stdout, multi-threaded, colored)
  //spdlog::info("Hello, {}!", "World");

  run_summation();

}
