#include <catch2/catch.hpp>
#include <thing.hpp>

constexpr std::uint64_t run_summation(std::uint64_t start, std::uint64_t end)
{
  using Stack_Type =
    Stack<std::monostate, RelativeStackReference, std::uint64_t, std::int64_t, Function, bool>;

  Stack_Type stack;
  Operations<Stack_Type> ops;
  using PushLiteral = PushLiteral<Stack_Type::value_type>;

  // setup variables
  ops.push_back(PushLiteral{ start });// count variable
  ops.push_back(PushLiteral{ end + 1 });// end value
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

  while (ops.next(stack)) {
    // run while we can
  }

  auto *result = peek<std::uint64_t>(stack, 0);
  if (result != nullptr) {
    return *result;
  } else {
    return 0;
  }
}


TEST_CASE("Thing can evaluate at constexpr time")
{
  STATIC_REQUIRE(run_summation(1, 10) == 55);
}
