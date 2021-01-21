#ifndef THING_LEXER_HPP
#define THING_LEXER_HPP

#include "lex_item.hpp"
#include <ctre.hpp>

// Helper function for concatenating two different ct11::fixed_strings
template<std::size_t N1, std::size_t N2>
constexpr auto operator+(const ctll::fixed_string<N1> &lhs, const ctll::fixed_string<N2> &rhs)
{
  char32_t result[N1 + N2 - 1]{};

  auto iter = std::begin(result);
  iter = std::copy(lhs.begin(), lhs.end(), iter);
  std::copy(rhs.begin(), rhs.end(), iter);
  return ctll::fixed_string(result);
}

namespace thing::lexing {

[[nodiscard]] static constexpr lex_item lexer(std::string_view v) noexcept
{
  if (v.empty()) { return lex_item{ token_type::end_of_file, v, v }; }

  // prefix with ^ so that the regex search only matches the start of the
  // string. Hana assures me this is efficient ;) (at runtime that is)
  constexpr auto make_token = [](const auto &s) { return ctll::fixed_string{ "^" } + ctll::fixed_string{ s }; };

  const auto ret = [v](const token_type type, std::string_view found) -> lex_item {
    return { type, v.substr(0, found.size()), v.substr(found.size()) };
  };

  constexpr auto identifier{ make_token("[_a-zA-Z]+[_0-9a-zA-Z]*") };
  constexpr auto quoted_string{ make_token(R"("([^"\\]|\\.)*")") };
  constexpr auto floatingpoint_number{ make_token("[0-9]+[.][0-9]*([eEpP][0-9]+)?[lLfF]?") };
  // if it starts with an int, it's an int
  // parsing will happen later to determine if it's valid
  constexpr auto integral_number{ make_token("[0-9][_a-zA-Z0-9]*") };
  constexpr auto whitespace{ make_token("\\s+") };

  using token = std::pair<std::string_view, token_type>;
  constexpr std::array tokens{ token{ "++", token_type::increment },
    token{ "--", token_type::decrement },
    token{ ">=", token_type::greater_than_or_equal },
    token{ "<=", token_type::less_than_or_equal },
    token{ "!=", token_type::not_equals },
    token{ "==", token_type::equals },
    token{ "||", token_type::logical_or },
    token{ "&&", token_type::logical_and },
    token{ "<", token_type::less_than },
    token{ ">", token_type::greater_than },
    token{ ":", token_type::colon },
    token{ ",", token_type::comma },
    token{ "=", token_type::assign },
    token{ "+", token_type::plus },
    token{ "-", token_type::minus },
    token{ "*", token_type::asterisk },
    token{ "/", token_type::slash },
    token{ "^", token_type::caret },
    token{ "~", token_type::tilde },
    token{ "%", token_type::percent },
    token{ "!", token_type::bang },
    token{ "?", token_type::question },
    token{ "(", token_type::left_paren },
    token{ ")", token_type::right_paren },
    token{ "{", token_type::left_brace },
    token{ "}", token_type::right_brace },
    token{ "[", token_type::left_bracket },
    token{ "]", token_type::right_bracket },
    token{ ";", token_type::semicolon } };

  constexpr std::array keywords{
    std::string_view{ "auto" }, std::string_view{ "for" }, std::string_view{ "if" }, std::string_view{ "else" }
  };

  if (auto result = ctre::search<whitespace>(v); result) { return ret(token_type::whitespace, result); }

  for (const auto &oper : tokens) {
    if (v.starts_with(oper.first)) { return ret(oper.second, oper.first); }
  }

  if (auto id_result = ctre::search<identifier>(v); id_result) {
    const auto is_keyword = std::any_of(
      begin(keywords), end(keywords), [id = std::string_view{ id_result }](const auto &rhs) { return id == rhs; });
    return ret(is_keyword ? token_type::keyword : token_type::identifier, id_result);
  } else if (auto string_result = ctre::search<quoted_string>(v); string_result) {
    return ret(token_type::string, string_result);
  } else if (auto float_result = ctre::search<floatingpoint_number>(v); float_result) {
    return ret(token_type::number, float_result);
  } else if (auto int_result = ctre::search<integral_number>(v); int_result) {
    return ret(token_type::number, int_result);
  }

  return lex_item{token_type::unknown, v, v.substr(v.size())};
}

}// namespace thing::lexing

#endif