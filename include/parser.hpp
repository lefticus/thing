#ifndef THING_PARSER_HPP
#define THING_PARSER_HPP

#include <array>
#include <iostream>
#include <optional>
#include <utility>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <ctre.hpp>

#include "lexer.hpp"

// references
// http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
// https://github.com/munificent/bantam/tree/master/src/com/stuffwithstuff/bantam
// https://github.com/MattDiesel/cpp-pratt
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
// https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
// https://stackoverflow.com/questions/380455/looking-for-a-clear-definition-of-what-a-tokenizer-parser-and-lexers-are


namespace thing {
// Helper function for concatenating two different ct11::fixed_strings


struct Parser
{

  struct parse_node
  {
    enum struct error_type { no_error, wrong_token_type, unexpected_prefix_token, unexpected_infix_token };

    lexer::lex_item item;
    std::vector<parse_node> children;

    error_type error{ error_type::no_error };
    lexer::Token_Type expected_token{};

    [[nodiscard]] constexpr bool is_error() const noexcept { return error != error_type::no_error; }
    explicit parse_node(lexer::lex_item item_, std::initializer_list<parse_node> children_ = {})
      : item{ std::move(item_) }, children{ children_ }
    {}

    parse_node(lexer::lex_item item_, std::vector<parse_node> &&children_)
      : item{ std::move(item_) }, children{ std::move(children_) }
    {}

    parse_node(lexer::lex_item item_, error_type error_, lexer::Token_Type expected_ = {})
      : item{ std::move(item_) }, error{ error_ }, expected_token{ expected_ }
    {}
  };

  lexer::lex_item token;


  auto parse(std::string_view v)
  {
    token = next_token(v);
    return expression();
  }

  constexpr bool peek(const lexer::Token_Type type, const std::string_view value = "") const
  {
    return token.type == type && (value.empty() || token.match == value);
  }

  // if a match is not possible, an error node is returned.
  [[nodiscard]] parse_node consume_match(const lexer::Token_Type type)
  {
    if (token.type != type) {
      return parse_node{ std::exchange(token, next()), parse_node::error_type::wrong_token_type, type };
    }
    return parse_node{ std::exchange(token, next()), {} };
  }

  parse_node list(bool consumeOpener, lexer::Token_Type opener, lexer::Token_Type closer, lexer::Token_Type delimiter)
  {
    auto result = [&]() {
      if (consumeOpener) {
        return consume_match(opener);
      } else {
        return parse_node{ lexer::lex_item{ lexer::Token_Type::unknown, {}, {} } };
      }
    }();

    while (!peek(closer)) {
      result.children.push_back(expression());
      if (peek(delimiter)) {
        // can discard, we know it will consume_match, we peeked
        [[maybe_unused]] const auto match_result = consume_match(delimiter);
      } else {
        break;
      }
    }

    if (const auto match_result = consume_match(closer); match_result.is_error()) {
      result.children.push_back(match_result);
    }

    return result;
  }

  parse_node control_block(const lexer::lex_item &item)
  {
    parse_node result{ item,
      { list(true, lexer::Token_Type::left_paren, lexer::Token_Type::right_paren, lexer::Token_Type::semicolon), statement() } };
    if (peek(lexer::Token_Type::keyword, "else")) {
      result.children.push_back(parse_node{ consume_match(lexer::Token_Type::keyword).item, { statement() } });
    }
    return result;
  }

  // prefix nodes
  parse_node null_denotation(const lexer::lex_item &item)
  {
    constexpr auto prefix_precedence = static_cast<int>(Precedence::prefix);

    switch (item.type) {
    case lexer::Token_Type::identifier:
    case lexer::Token_Type::number:
    case lexer::Token_Type::string:
      return parse_node{ item };
    case lexer::Token_Type::keyword:
      if (item.match == "if" || item.match == "for" || item.match == "while") {
        return control_block(item);
      } else {
        return parse_node{ item, { parse_node{ expression(prefix_precedence) } } };
      }
    case lexer::Token_Type::plus:
    case lexer::Token_Type::minus:
      return parse_node{ item, { parse_node{ expression(prefix_precedence) } } };
    case lexer::Token_Type::left_paren: {
      const auto result = expression();
      if (const auto match_result = consume_match(lexer::Token_Type::right_paren); match_result.is_error()) {
        return match_result;
      }

      return result;
    }
    default:
      return parse_node{ item, parse_node::error_type::unexpected_prefix_token };
    };
  }

  // infix processing
  [[nodiscard]] parse_node left_denotation(const lexer::lex_item &item, const parse_node &left)
  {
    switch (item.type) {
    case lexer::Token_Type::plus:
    case lexer::Token_Type::minus:
    case lexer::Token_Type::asterisk:
    case lexer::Token_Type::slash:
    case lexer::Token_Type::less_than:
    case lexer::Token_Type::less_than_or_equal:
    case lexer::Token_Type::greater_than:
    case lexer::Token_Type::greater_than_or_equal:
    case lexer::Token_Type::logical_and:
    case lexer::Token_Type::logical_or:
    case lexer::Token_Type::equals:
    case lexer::Token_Type::not_equals:
      return parse_node{ item, { left, expression(lbp(item.type)) } };
    case lexer::Token_Type::bang:
      return parse_node{ item, { left } };
    case lexer::Token_Type::left_paren: {
      auto result = left;
      result.children.emplace_back(
        item, list(false, lexer::Token_Type::left_paren, lexer::Token_Type::right_paren, lexer::Token_Type::comma).children);
      return result;
    }
    case lexer::Token_Type::left_brace: {
      auto result = left;
      result.children.push_back(parse_node{ item, { expression() } });
      if (const auto match_result = consume_match(lexer::Token_Type::right_brace); match_result.is_error()) {
        return match_result;
      }
      return result;
    }
    case lexer::Token_Type::caret:
      // caret, as an infix, is right-associative, so we decrease its
      // precedence slightly
      // https://github.com/munificent/bantam/blob/8b0b06a1543b7d9e84ba2bb8d916979459971b2d/src/com/stuffwithstuff/bantam/parselets/BinaryOperatorParselet.java#L20-L27
      // note that this https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
      // example disagrees slightly, it provides a gap in precedence, where
      // the bantam example notches it down to share with other levels
      return parse_node{ item, { left, expression(lbp(item.type) - 1) } };
    default:
      return parse_node{ item, parse_node::error_type::unexpected_infix_token };
    }
  }

  enum struct Precedence {
    none = 0,
    // comma = 1,
    // assignment = 2,
    logical_or = 1,
    logical_and = 2,
    equality_comparison = 3,
    relational_comparison = 4,
    sum = 5,
    product = 6,
    exponent = 7,
    prefix = 8,
    postfix = 9,
    call = 10,
    keyword = 11
  };

  // this is for infix precedence values
  // lbp = left binding power
  [[nodiscard]] static constexpr int lbp(const lexer::Token_Type type) noexcept
  {
    const auto precedence = [type]() -> Precedence {
      switch (type) {
      case lexer::Token_Type::plus:
      case lexer::Token_Type::minus:
        return Precedence::sum;
      case lexer::Token_Type::asterisk:
      case lexer::Token_Type::slash:
        return Precedence::product;
      case lexer::Token_Type::caret:
        return Precedence::exponent;
      case lexer::Token_Type::bang:
        return Precedence::postfix;
      case lexer::Token_Type::left_brace:
      case lexer::Token_Type::left_paren:
        return Precedence::call;
      case lexer::Token_Type::right_paren:
        return Precedence::none;// right_paren will be parsed but not consumed when matching
      case lexer::Token_Type::end_of_file:
        return Precedence::none;
      case lexer::Token_Type::keyword:
        return Precedence::keyword;
      case lexer::Token_Type::less_than_or_equal:
      case lexer::Token_Type::less_than:
      case lexer::Token_Type::greater_than_or_equal:
      case lexer::Token_Type::greater_than:
        return Precedence::relational_comparison;
      case lexer::Token_Type::logical_and:
        return Precedence::logical_and;
      case lexer::Token_Type::logical_or:
        return Precedence::logical_or;
      case lexer::Token_Type::equals:
      case lexer::Token_Type::not_equals:
        return Precedence::equality_comparison;
      default:
        return Precedence::none;
        //        throw std::runtime_error(fmt::format("Unhandled value lbp {}", static_cast<int>(type)));
      }
    }();

    return static_cast<int>(precedence);
  }

  [[nodiscard]] parse_node statement()
  {
    if (peek(lexer::Token_Type::left_brace)) { return compound_statement(); }
    auto result = expression();

    if (result.item.type == lexer::Token_Type::left_brace
        || (!result.children.empty() && result.children.back().item.type == lexer::Token_Type::left_brace)) {
      // our last matched item was a compound statement of some sort, so we won't require a closing semicolon
    } else {
      if (const auto match_result = consume_match(lexer::Token_Type::semicolon); match_result.is_error()) {
        return match_result;
      }
    }
    return result;
  }

  [[nodiscard]] parse_node compound_statement()
  {
    auto result = consume_match(lexer::Token_Type::left_brace);
    if (result.is_error()) { return result; }
    while (!peek(lexer::Token_Type::right_brace)) { result.children.emplace_back(statement()); }
    // we peeked, it'll consume_match
    [[maybe_unused]] const auto right_brace = consume_match(lexer::Token_Type::right_brace);
    return result;
  }

  [[nodiscard]] parse_node expression(const int rbp = 0)
  {
    // parse prefix token
    const auto prefix = token;
    token = next();
    auto left = null_denotation(prefix);

    // parse infix / postfix tokens
    while (rbp < lbp(token.type)) {
      const auto t = token;
      token = next();
      left = left_denotation(t, left);
    }

    return left;
  }

  [[nodiscard]] constexpr lexer::lex_item next() const noexcept { return next_token(token.remainder); }

  static constexpr lexer::lex_item next_token(std::string_view v) noexcept
  {
    while (true) {
      if (const auto item = lexer::lexer(v); item) {
        if (item->type != lexer::Token_Type::whitespace) {
          return *item;
        } else {
          v = item->remainder;
        }
      } else {
        return lexer::lex_item{ lexer::Token_Type::unknown, v, v };
      }
    }
  }


};
}// namespace parser_test

#endif
