#ifndef THING_LEXER_HPP
#define THING_LEXER_HPP


template<std::size_t N1, std::size_t N2>
constexpr auto operator+(const ctll::fixed_string<N1> &lhs, const ctll::fixed_string<N2> &rhs)
{
  char32_t result[N1 + N2 - 1]{};

  auto iter = std::begin(result);
  iter = std::copy(lhs.begin(), lhs.end(), iter);
  std::copy(rhs.begin(), rhs.end(), iter);
  return ctll::fixed_string(result);
}

namespace thing::lexer {

enum class Token_Type {
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

constexpr static std::string_view to_string(const Token_Type type)
{
  switch (type) {
  case Token_Type::unknown:
    return "<unknown>";
  case Token_Type::identifier:
    return "<identifier>";
  case Token_Type::number:
    return "<number literal>";
  case Token_Type::increment:
    return "++";
  case Token_Type::decrement:
    return "--";
  case Token_Type::left_paren:
    return "(";
  case Token_Type::right_paren:
    return ")";
  case Token_Type::left_brace:
    return "{";
  case Token_Type::right_brace:
    return "}";
  case Token_Type::left_bracket:
    return "[";
  case Token_Type::right_bracket:
    return "]";
  case Token_Type::comma:
    return ",";
  case Token_Type::assign:
    return "=";
  case Token_Type::plus:
    return "+";
  case Token_Type::minus:
    return "-";
  case Token_Type::asterisk:
    return "*";
  case Token_Type::percent:
    return "%";
  case Token_Type::slash:
    return "/";
  case Token_Type::caret:
    return "^";
  case Token_Type::tilde:
    return "~";
  case Token_Type::bang:
    return "!";
  case Token_Type::question:
    return "?";
  case Token_Type::colon:
    return ":";
  case Token_Type::string:
    return "<string-literal>";
  case Token_Type::keyword:
    return "<keyword>";
  case Token_Type::whitespace:
    return "<whitespace>";
  case Token_Type::semicolon:
    return ";";
  case Token_Type::equals:
    return "==";
  case Token_Type::greater_than:
    return ">";
  case Token_Type::less_than:
    return "<";
  case Token_Type::greater_than_or_equal:
    return ">=";
  case Token_Type::less_than_or_equal:
    return "<=";
  case Token_Type::not_equals:
    return "!=";
  case Token_Type::end_of_file:
    return "<end-of-file>";
  case Token_Type::logical_and:
    return "&&";
  case Token_Type::logical_or:
    return "||";
  }

  return "<unknown>";
}

struct lex_item
{
  Token_Type type{ Token_Type::unknown };
  std::string_view match;
  std::string_view remainder;
};


static constexpr std::optional<lex_item> lexer(std::string_view v) noexcept
{

  if (v.empty()) { return lex_item{ Token_Type::end_of_file, v, v }; }

  // prefix with ^ so that the regex search only matches the start of the
  // string. Hana assures me this is efficient ;) (at runtime that is)
  constexpr auto make_token = [](const auto &s) { return ctll::fixed_string{ "^" } + ctll::fixed_string{ s }; };

  const auto ret = [v](const Token_Type type, std::string_view found) -> lex_item {
    return { type, v.substr(0, found.size()), v.substr(found.size()) };
  };

  constexpr auto identifier{ make_token("[_a-zA-Z]+[_0-9a-zA-Z]*") };
  constexpr auto quoted_string{ make_token(R"("([^"\\]|\\.)*")") };
  constexpr auto floatingpoint_number{ make_token("[0-9]+[.][0-9]*([eEpP][0-9]+)?[lLfF]?") };
  // if it starts with an int, it's an int
  // parsing will happen later to determine if it's valid
  constexpr auto integral_number{ make_token("[0-9][_a-zA-Z0-9]*") };
  constexpr auto whitespace{ make_token("\\s+") };

  using token = std::pair<std::string_view, Token_Type>;
  constexpr std::array tokens{ token{ "++", Token_Type::increment },
    token{ "--", Token_Type::decrement },
    token{ ">=", Token_Type::greater_than_or_equal },
    token{ "<=", Token_Type::less_than_or_equal },
    token{ "!=", Token_Type::not_equals },
    token{ "==", Token_Type::equals },
    token{ "||", Token_Type::logical_or },
    token{ "&&", Token_Type::logical_and },
    token{ "<", Token_Type::less_than },
    token{ ">", Token_Type::greater_than },
    token{ ":", Token_Type::colon },
    token{ ",", Token_Type::comma },
    token{ "=", Token_Type::assign },
    token{ "+", Token_Type::plus },
    token{ "-", Token_Type::minus },
    token{ "*", Token_Type::asterisk },
    token{ "/", Token_Type::slash },
    token{ "^", Token_Type::caret },
    token{ "~", Token_Type::tilde },
    token{ "%", Token_Type::percent },
    token{ "!", Token_Type::bang },
    token{ "?", Token_Type::question },
    token{ "(", Token_Type::left_paren },
    token{ ")", Token_Type::right_paren },
    token{ "{", Token_Type::left_brace },
    token{ "}", Token_Type::right_brace },
    token{ "[", Token_Type::left_bracket },
    token{ "]", Token_Type::right_bracket },
    token{ ";", Token_Type::semicolon } };

  constexpr std::array keywords{
    std::string_view{ "auto" }, std::string_view{ "for" }, std::string_view{ "if" }, std::string_view{ "else" }
  };

  if (auto result = ctre::search<whitespace>(v); result) { return ret(Token_Type::whitespace, result); }

  for (const auto &oper : tokens) {
    if (v.starts_with(oper.first)) { return ret(oper.second, oper.first); }
  }

  if (auto id_result = ctre::search<identifier>(v); id_result) {
    const auto is_keyword = std::any_of(
      begin(keywords), end(keywords), [id = std::string_view{ id_result }](const auto &rhs) { return id == rhs; });
    return ret(is_keyword ? Token_Type::keyword : Token_Type::identifier, id_result);
  } else if (auto string_result = ctre::search<quoted_string>(v); string_result) {
    return ret(Token_Type::string, string_result);
  } else if (auto float_result = ctre::search<floatingpoint_number>(v); float_result) {
    return ret(Token_Type::number, float_result);
  } else if (auto int_result = ctre::search<integral_number>(v); int_result) {
    return ret(Token_Type::number, int_result);
  }

  return std::nullopt;
}

}// namespace thing::lexer

#endif