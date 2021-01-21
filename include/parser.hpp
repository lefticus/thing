#ifndef THING_PARSER_HPP
#define THING_PARSER_HPP

#include <array>
#include <iostream>
#include <optional>
#include <utility>
#include <cstdint>
#include <iterator>
#include <string_view>

#include "parse_node.hpp"
#include "lexer.hpp"

// We are attempting to implement a "Pratt Parser" here, using
// these references:
//
// http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
// https://github.com/munificent/bantam/tree/master/src/com/stuffwithstuff/bantam
// https://github.com/MattDiesel/cpp-pratt
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
// https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
// https://stackoverflow.com/questions/380455/looking-for-a-clear-definition-of-what-a-tokenizer-parser-and-lexings-are


namespace thing::parsing {


struct parser
{

  lexing::lex_item next_lexed_token;


  auto parse(std::string_view v)
  {
    next_lexed_token = next_token(v);
    return expression();
  }

  [[nodiscard]] constexpr bool peek(const lexing::token_type type, const std::string_view value = "") const noexcept
  {
    return next_lexed_token.type == type && (value.empty() || next_lexed_token.match == value);
  }

  [[nodiscard]] constexpr bool next_token_is_valid() const noexcept {
    switch (next_lexed_token.type) {
    case lexing::token_type::end_of_file:
    case lexing::token_type::unknown:
      return false;
    default:
      return true;
    }
  }

  // if a match is not possible, an error node is returned.
  [[nodiscard]] parse_node consume_match(const lexing::token_type type)
  {
    if (next_lexed_token.type != type) {
      return parse_node{ std::exchange(next_lexed_token, next()), parse_node::error_type::wrong_token_type, type };
    }
    return parse_node{ std::exchange(next_lexed_token, next()), {} };
  }

  [[nodiscard]] parse_node list(const bool consume_opener,
    const lexing::token_type opener,
    const lexing::token_type closer,
    const lexing::token_type delimiter)
  {
    auto result = [&]() {
      if (consume_opener) {
        return consume_match(opener);
      } else {
        return parse_node{ lexing::lex_item{ lexing::token_type::unknown, {}, {} } };
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

    // a failed match is useful information, so we save it for later reporting
    if (const auto match_result = consume_match(closer); match_result.is_error()) {
      result.children.push_back(match_result);
    }

    return result;
  }

  [[nodiscard]] parse_node control_block(const lexing::lex_item &item)
  {
    parse_node result{ item,
      { list(true, lexing::token_type::left_paren, lexing::token_type::right_paren, lexing::token_type::semicolon),
        statement() } };
    if (peek(lexing::token_type::keyword, "else")) {
      result.children.push_back(parse_node{ consume_match(lexing::token_type::keyword).item, { statement() } });
    }
    return result;
  }

  // prefix nodes
  [[nodiscard]] parse_node null_denotation(const lexing::lex_item &item)
  {
    constexpr auto prefix_precedence = static_cast<int>(precedence::prefix);

    switch (item.type) {
    case lexing::token_type::identifier:
    case lexing::token_type::number:
    case lexing::token_type::string:
      return parse_node{ item };
    case lexing::token_type::keyword:
      if (item.match == "if" || item.match == "for" || item.match == "while") {
        return control_block(item);
      } else {
        return parse_node{ item, { parse_node{ expression(prefix_precedence) } } };
      }
    case lexing::token_type::plus:
    case lexing::token_type::minus:
      return parse_node{ item, { parse_node{ expression(prefix_precedence) } } };
    case lexing::token_type::left_paren: {
      // not const because of two different return paths, we don't want to
      // disable automatic moves
      auto result = expression();
      if (auto match_result = consume_match(lexing::token_type::right_paren); match_result.is_error()) {
        return match_result;
      }

      return result;
    }
    default:
      return parse_node{ item, parse_node::error_type::unexpected_prefix_token };
    };
  }

  // infix processing
  [[nodiscard]] parse_node left_denotation(const lexing::lex_item &item, const parse_node &left)
  {
    switch (item.type) {
    case lexing::token_type::plus:
    case lexing::token_type::minus:
    case lexing::token_type::asterisk:
    case lexing::token_type::slash:
    case lexing::token_type::less_than:
    case lexing::token_type::less_than_or_equal:
    case lexing::token_type::greater_than:
    case lexing::token_type::greater_than_or_equal:
    case lexing::token_type::logical_and:
    case lexing::token_type::logical_or:
    case lexing::token_type::equals:
    case lexing::token_type::not_equals:
      return parse_node{ item, { left, expression(lbp(item.type)) } };
    case lexing::token_type::bang:
      return parse_node{ item, { left } };
    case lexing::token_type::left_paren: {
      auto result = left;
      result.children.emplace_back(item,
        list(false, lexing::token_type::left_paren, lexing::token_type::right_paren, lexing::token_type::comma)
          .children);
      return result;
    }
    case lexing::token_type::left_brace: {
      auto result = left;
      result.children.push_back(parse_node{ item, { expression() } });
      if (auto match_result = consume_match(lexing::token_type::right_brace); match_result.is_error()) {
        return match_result;
      }
      return result;
    }
    case lexing::token_type::caret:
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

  enum struct precedence {
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
  [[nodiscard]] static constexpr int lbp(const lexing::token_type type) noexcept
  {
    return static_cast<int>([type]() {
      switch (type) {
      case lexing::token_type::plus:
      case lexing::token_type::minus:
        return precedence::sum;

      case lexing::token_type::asterisk:
      case lexing::token_type::slash:
        return precedence::product;

      case lexing::token_type::caret:
        return precedence::exponent;

      case lexing::token_type::bang:
        return precedence::postfix;

      case lexing::token_type::left_brace:
      case lexing::token_type::left_paren:
        return precedence::call;

      case lexing::token_type::keyword:
        return precedence::keyword;

      case lexing::token_type::less_than_or_equal:
      case lexing::token_type::less_than:
      case lexing::token_type::greater_than_or_equal:
      case lexing::token_type::greater_than:
        return precedence::relational_comparison;

      case lexing::token_type::logical_and:
        return precedence::logical_and;

      case lexing::token_type::logical_or:
        return precedence::logical_or;

      case lexing::token_type::equals:
      case lexing::token_type::not_equals:
        return precedence::equality_comparison;

        // all of the remaining do not have a left binding power
        // we are leaving them here as explicit cases so that we know if we add
        // a new token_type that should have an LBP but forget to add it
      case lexing::token_type::right_paren:// right_paren will be parsed but not consumed when matching
      case lexing::token_type::end_of_file:
      case lexing::token_type::unknown:
      case lexing::token_type::identifier:
      case lexing::token_type::number:
      case lexing::token_type::increment:
      case lexing::token_type::decrement:
      case lexing::token_type::right_brace:
      case lexing::token_type::left_bracket:
      case lexing::token_type::right_bracket:
      case lexing::token_type::comma:
      case lexing::token_type::assign:
      case lexing::token_type::percent:
      case lexing::token_type::tilde:
      case lexing::token_type::question:
      case lexing::token_type::colon:
      case lexing::token_type::string:
      case lexing::token_type::whitespace:
      case lexing::token_type::semicolon:
        return precedence::none;
      }
      return precedence::none;
    }());
  }

  [[nodiscard]] parse_node statement()
  {
    if (peek(lexing::token_type::left_brace)) { return compound_statement(); }
    auto result = expression();

    if (result.item.type == lexing::token_type::left_brace
        || (!result.children.empty() && result.children.back().item.type == lexing::token_type::left_brace)) {
      // our last matched item was a compound statement of some sort, so we won't require a closing semicolon
    } else {
      if (auto match_result = consume_match(lexing::token_type::semicolon); match_result.is_error()) {
        result.children.push_back(match_result);
      }
    }
    return result;
  }

  [[nodiscard]] parse_node compound_statement()
  {
    auto result = consume_match(lexing::token_type::left_brace);
    if (result.is_error()) { return result; }

    while (!peek(lexing::token_type::right_brace) && next_token_is_valid()) { result.children.emplace_back(statement()); }

    if (auto right_brace = consume_match(lexing::token_type::right_brace); right_brace.is_error()) {
      result.children.push_back(right_brace);
    }
    return result;
  }

  [[nodiscard]] parse_node expression(const int rbp = 0)
  {
    // parse prefix next_lexed_token
    const auto prefix = next_lexed_token;
    next_lexed_token = next();
    auto left = null_denotation(prefix);

    // parse infix / postfix tokens
    while (rbp < lbp(next_lexed_token.type)) {
      const auto t = next_lexed_token;
      next_lexed_token = next();
      left = left_denotation(t, left);
    }

    return left;
  }

  [[nodiscard]] constexpr lexing::lex_item next() const noexcept { return next_token(next_lexed_token.remainder); }

  [[nodiscard]] static constexpr lexing::lex_item next_token(std::string_view v) noexcept
  {
    while (true) {
      if (const auto item = lexing::lexer(v); item) {
        if (item->type != lexing::token_type::whitespace) {
          return *item;
        } else {
          v = item->remainder;
        }
      } else {
        return lexing::lex_item{ lexing::token_type::unknown, v, v };
      }
    }
  }
};
}// namespace thing::parsing

#endif
