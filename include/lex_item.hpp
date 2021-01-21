#ifndef THING_LEX_ITEM_HPP
#define THING_LEX_ITEM_HPP

namespace thing::lexing {
enum struct token_type {
  unknown = 0,
  identifier,
  number,
  increment,
  decrement,
  left_paren,
  right_paren,
  left_brace,
  right_brace,
  left_bracket,
  right_bracket,
  comma,
  assign,
  plus,
  minus,
  asterisk,
  percent,
  slash,
  caret,
  tilde,
  bang,
  question,
  colon,
  string,
  keyword,
  whitespace,
  semicolon,
  equals,
  greater_than,
  less_than,
  greater_than_or_equal,
  less_than_or_equal,
  not_equals,
  end_of_file,
  logical_and,
  logical_or
};

constexpr static std::string_view to_string(const token_type type) noexcept
{
  switch (type) {
  case token_type::unknown:
    return "<unknown>";
  case token_type::identifier:
    return "<identifier>";
  case token_type::number:
    return "<number literal>";
  case token_type::increment:
    return "++";
  case token_type::decrement:
    return "--";
  case token_type::left_paren:
    return "(";
  case token_type::right_paren:
    return ")";
  case token_type::left_brace:
    return "{";
  case token_type::right_brace:
    return "}";
  case token_type::left_bracket:
    return "[";
  case token_type::right_bracket:
    return "]";
  case token_type::comma:
    return ",";
  case token_type::assign:
    return "=";
  case token_type::plus:
    return "+";
  case token_type::minus:
    return "-";
  case token_type::asterisk:
    return "*";
  case token_type::percent:
    return "%";
  case token_type::slash:
    return "/";
  case token_type::caret:
    return "^";
  case token_type::tilde:
    return "~";
  case token_type::bang:
    return "!";
  case token_type::question:
    return "?";
  case token_type::colon:
    return ":";
  case token_type::string:
    return "<string-literal>";
  case token_type::keyword:
    return "<keyword>";
  case token_type::whitespace:
    return "<whitespace>";
  case token_type::semicolon:
    return ";";
  case token_type::equals:
    return "==";
  case token_type::greater_than:
    return ">";
  case token_type::less_than:
    return "<";
  case token_type::greater_than_or_equal:
    return ">=";
  case token_type::less_than_or_equal:
    return "<=";
  case token_type::not_equals:
    return "!=";
  case token_type::end_of_file:
    return "<end-of-file>";
  case token_type::logical_and:
    return "&&";
  case token_type::logical_or:
    return "||";
  }

  return "<unknown>";
}

struct lex_item
{
  token_type type{ token_type::unknown };
  std::string_view match;
  std::string_view remainder;
};

}// namespace thing::lexing

#endif
