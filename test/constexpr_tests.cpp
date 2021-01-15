#include <catch2/catch.hpp>
#include <thing.hpp>
#include <parser.hpp>
#include <string_view>


// we want constexpr either fully on or fully off, so we can debug when the build fails
#ifdef CATCH_CONFIG_RUNTIME_STATIC_REQUIRE
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

CONSTEXPR std::uint64_t run_summation(std::uint64_t start, std::uint64_t end)
{
  using Stack_Type = Stack<std::monostate, RelativeStackReference, std::uint64_t, std::int64_t, Function, bool>;

  Stack_Type stack;
  Operations<Stack_Type> ops;
  using PushLiteral = PushLiteral<Stack_Type::value_type>;

  // setup variables
  ops.push_back(PushLiteral{ start });// count variable
  ops.push_back(PushLiteral{ end + 1 });// end value
  ops.push_back(PushLiteral{ 0ull });// accumulator

  ops.push_back(PushLiteral{ std::monostate{} });// temporary space for results

  // Test to see if count and end are the same yet
  ops.push_back(PushLiteral{ Function{ &execBinaryOp<Stack_Type, std::uint64_t, std::int64_t> } });// function to call
  ops.push_back(PushLiteral{ RelativeStackReference{ 2 } });// reference to results for return value
  ops.push_back(PushLiteral{ static_cast<std::uint64_t>(BinaryOps::Equal_To) });
  ops.push_back(PushLiteral{ RelativeStackReference{ 7 } });// reference to count variable
  ops.push_back(PushLiteral{ RelativeStackReference{ 7 } });// reference to end value
  ops.push_back(PushLiteral{ 3ull });// arity
  ops.push_back(CallFunction{ true });

  // If they are, jump over loop body
  ops.push_back(RelativeJump{ true, 14 });

  // reuse result space from Equal_To

  // Loop body, accumulate current value into accumulator
  ops.push_back(PushLiteral{ Function{ &execBinaryOp<Stack_Type, std::uint64_t, std::int64_t> } });// function to call
  ops.push_back(PushLiteral{ RelativeStackReference{ 3 } });// reference to accumulator
  ops.push_back(PushLiteral{ static_cast<std::uint64_t>(BinaryOps::Addition) });// param 1
  ops.push_back(PushLiteral{ RelativeStackReference{ 7 } });// reference to count variable
  ops.push_back(PushLiteral{ RelativeStackReference{ 6 } });// reference to accumulator variable
  ops.push_back(PushLiteral{ 3ull });// arity
  ops.push_back(CallFunction{ true });

  // Loop incrementer
  ops.push_back(PushLiteral{ Function{ &execUnaryOp<Stack_Type, std::uint64_t, std::int64_t> } });// function to call
  ops.push_back(PushLiteral{ std::monostate{} });// we don't care about return value
  ops.push_back(PushLiteral{ static_cast<std::uint64_t>(UnaryOps::Pre_Increment) });
  ops.push_back(PushLiteral{ RelativeStackReference{ 7 } });// reference to count variable
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


TEST_CASE("Thing can evaluate at CONSTEXPR time") { STATIC_REQUIRE(run_summation(1, 10) == 55); }


TEST_CASE("Can lex arithmetic expressions")
{
  CONSTEXPR std::string_view expression = "4*23.45+15    %79.131231234123^some_iDent1fier";

  CONSTEXPR auto result_1 = thing::lexer::lexer(expression);
  STATIC_REQUIRE(result_1);
  STATIC_REQUIRE(result_1->match == "4");
  STATIC_REQUIRE(result_1->remainder == "*23.45+15    %79.131231234123^some_iDent1fier");
  STATIC_REQUIRE(result_1->type == thing::lexer::Token_Type::number);

  CONSTEXPR auto result_2 = thing::lexer::lexer(result_1->remainder);
  STATIC_REQUIRE(result_2);
  STATIC_REQUIRE(result_2->match == "*");
  STATIC_REQUIRE(result_2->remainder == "23.45+15    %79.131231234123^some_iDent1fier");
  STATIC_REQUIRE(result_2->type == thing::lexer::Token_Type::asterisk);

  CONSTEXPR auto result_3 = thing::lexer::lexer(result_2->remainder);
  STATIC_REQUIRE(result_3);
  STATIC_REQUIRE(result_3->match == "23.45");
  STATIC_REQUIRE(result_3->remainder == "+15    %79.131231234123^some_iDent1fier");
  STATIC_REQUIRE(result_3->type == thing::lexer::Token_Type::number);

  CONSTEXPR auto result_4 = thing::lexer::lexer(result_3->remainder);
  STATIC_REQUIRE(result_4);
  STATIC_REQUIRE(result_4->match == "+");
  STATIC_REQUIRE(result_4->remainder == "15    %79.131231234123^some_iDent1fier");
  STATIC_REQUIRE(result_4->type == thing::lexer::Token_Type::plus);


  CONSTEXPR auto result_5 = thing::lexer::lexer(result_4->remainder);
  STATIC_REQUIRE(result_5);
  STATIC_REQUIRE(result_5->match == "15");
  STATIC_REQUIRE(result_5->remainder == "    %79.131231234123^some_iDent1fier");
  STATIC_REQUIRE(result_5->type == thing::lexer::Token_Type::number);

  CONSTEXPR auto result_6 = thing::lexer::lexer(result_5->remainder);
  STATIC_REQUIRE(result_6);
  STATIC_REQUIRE(result_6->match == "    ");
  STATIC_REQUIRE(result_6->remainder == "%79.131231234123^some_iDent1fier");
  STATIC_REQUIRE(result_6->type == thing::lexer::Token_Type::whitespace);

  CONSTEXPR auto result_7 = thing::lexer::lexer(result_6->remainder);
  STATIC_REQUIRE(result_7);
  STATIC_REQUIRE(result_7->match == "%");
  STATIC_REQUIRE(result_7->remainder == "79.131231234123^some_iDent1fier");
  STATIC_REQUIRE(result_7->type == thing::lexer::Token_Type::percent);

  CONSTEXPR auto result_8 = thing::lexer::lexer(result_7->remainder);
  STATIC_REQUIRE(result_8);
  STATIC_REQUIRE(result_8->match == "79.131231234123");
  STATIC_REQUIRE(result_8->remainder == "^some_iDent1fier");
  STATIC_REQUIRE(result_8->type == thing::lexer::Token_Type::number);

  CONSTEXPR auto result_9 = thing::lexer::lexer(result_8->remainder);
  STATIC_REQUIRE(result_9);
  STATIC_REQUIRE(result_9->match == "^");
  STATIC_REQUIRE(result_9->remainder == "some_iDent1fier");
  STATIC_REQUIRE(result_9->type == thing::lexer::Token_Type::caret);

  CONSTEXPR auto result_10 = thing::lexer::lexer(result_9->remainder);
  STATIC_REQUIRE(result_10);
  STATIC_REQUIRE(result_10->match == "some_iDent1fier");
  STATIC_REQUIRE(result_10->remainder == "");
  STATIC_REQUIRE(result_10->type == thing::lexer::Token_Type::identifier);
}

TEST_CASE("Can lex integer literals")
{
  CONSTEXPR auto integer = std::string_view{ "12345" };
  CONSTEXPR auto lexed_int = thing::lexer::lexer(integer);
  STATIC_REQUIRE(lexed_int->match == "12345");
  STATIC_REQUIRE(lexed_int->remainder == "");
  STATIC_REQUIRE(lexed_int->type == thing::lexer::Token_Type::number);

  CONSTEXPR auto hex = std::string_view{ "0xAF12345" };
  CONSTEXPR auto lexed_hex = thing::lexer::lexer(hex);
  STATIC_REQUIRE(lexed_hex->match == "0xAF12345");
  STATIC_REQUIRE(lexed_hex->remainder == "");
  STATIC_REQUIRE(lexed_hex->type == thing::lexer::Token_Type::number);

  CONSTEXPR auto bits = std::string_view{ "0B1010101" };
  CONSTEXPR auto lexed_bits = thing::lexer::lexer(bits);
  STATIC_REQUIRE(lexed_bits->match == "0B1010101");
  STATIC_REQUIRE(lexed_bits->remainder == "");
  STATIC_REQUIRE(lexed_bits->type == thing::lexer::Token_Type::number);
}

// TEST_CASE("Can handle lexing of bad literals")
//{
//  CONSTEXPR auto bits = std::string_view{ "0B2010101" };
//  CONSTEXPR auto lexed_bits = thing::lexer::lexer(bits);
//  STATIC_REQUIRE(!lexed_bits);
//}


TEST_CASE("Can lex floating point literals")
{
  CONSTEXPR auto fp = std::string_view("123.42E1l");
  CONSTEXPR auto lexed_fp = thing::lexer::lexer(fp);
  STATIC_REQUIRE(lexed_fp->match == "123.42E1l");
  STATIC_REQUIRE(lexed_fp->remainder == "");
  STATIC_REQUIRE(lexed_fp->type == thing::lexer::Token_Type::number);
}
