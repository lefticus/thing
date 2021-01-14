#ifndef PARSER_TEST_PARSER_HPP
#define PARSER_TEST_PARSER_HPP

#include <array>
#include <iostream>
#include <optional>
#include <utility>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <ctre.hpp>

// references
// http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
// https://github.com/munificent/bantam/tree/master/src/com/stuffwithstuff/bantam
// https://github.com/MattDiesel/cpp-pratt
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
// https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
// https://stackoverflow.com/questions/380455/looking-for-a-clear-definition-of-what-a-tokenizer-parser-and-lexers-are


namespace parser_test {
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


struct Parser
{
  enum class Type {
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

  struct lex_item
  {
    Type type{ Type::unknown };
    std::string_view match;
    std::string_view remainder;
  };

  struct parse_node
  {
    lex_item item;
    std::vector<parse_node> children;

    parse_node(lex_item item_, std::initializer_list<parse_node> children_ = {})
      : item{ std::move(item_) }, children{ children_ }
    {}

    parse_node(lex_item item_, std::vector<parse_node> &&children_)
      : item{ std::move(item_) }, children{ std::move(children_) }
    {}
  };

  lex_item token;


  auto parse(std::string_view v)
  {
    token = next_token(v);
    return expression();
  }

  constexpr bool peek(const Type type, const std::string_view value="") const {
    return token.type == type && (value.empty() || token.match == value);
  }

  constexpr lex_item match(const Type type)
  {
    if (token.type != type) {
      throw token;
    }
    return std::exchange(token, next());
  }

  parse_node list(bool consumeOpener, Type opener, Type closer, Type delimiter)
  {
    auto result = [&]() {
      if (consumeOpener) {
        return parse_node{ match(opener) };
      } else {
        return parse_node{ lex_item{ Type::unknown, {}, {} } };
      }
    }();

    while (!peek(closer)) {
      result.children.push_back(expression());
      if (peek(delimiter)) {
        match(delimiter);
      } else {
        break;
      }
    }

    match(closer);

    return result;
  }

  parse_node control_block(const lex_item &item)
  {
    parse_node result{ item, { list(true, Type::left_paren, Type::right_paren, Type::semicolon), statement() } };
    if (peek(Type::keyword, "else")) {
      result.children.push_back(parse_node{match(Type::keyword), {statement()}});
    }
    return result;
  }

  parse_node nud(const lex_item &item)
  {
    constexpr auto prefix_precedence = static_cast<int>(Precedence::prefix);

    switch (item.type) {
    case Type::identifier:
    case Type::number:
    case Type::string:
      return parse_node{ item };
    case Type::keyword:
      if (item.match == "if" || item.match == "for" || item.match == "while") {
        return control_block(item);
      } else {
        return parse_node{ item, { parse_node{ expression(prefix_precedence) } } };
      }
    case Type::plus:
    case Type::minus:
      return parse_node{ item, { parse_node{ expression(prefix_precedence) } } };
    case Type::left_paren: {
      const auto result = expression();
      match(Type::right_paren);

      return result;
    }
    default:
      throw item;
    };
  }

  parse_node led(const lex_item &item, const parse_node &left)
  {
    switch (item.type) {
    case Type::plus:
    case Type::minus:
    case Type::asterisk:
    case Type::slash:
    case Type::less_than:
    case Type::less_than_or_equal:
    case Type::greater_than:
    case Type::greater_than_or_equal:
    case Type::logical_and:
    case Type::logical_or:
    case Type::equals:
    case Type::not_equals:
      return parse_node{ item, { left, expression(lbp(item.type)) } };
    case Type::bang:
      return parse_node{ item, { left } };
    case Type::left_paren: {
      auto result = left;
      result.children.emplace_back(item, list(false, Type::left_paren, Type::right_paren, Type::comma).children);
      return result;
    }
    case Type::left_brace: {
      auto result = left;
      result.children.push_back(parse_node{ item, { expression() } });
      match(Type::right_brace);
      return result;
    }
    case Type::caret:
      // caret, as an infix, is right-associative, so we decrease its
      // precedence slightly
      // https://github.com/munificent/bantam/blob/8b0b06a1543b7d9e84ba2bb8d916979459971b2d/src/com/stuffwithstuff/bantam/parselets/BinaryOperatorParselet.java#L20-L27
      // note that this https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
      // example disagrees slightly, it provides a gap in precedence, where
      // the bantam example notches it down to share with other levels
      return parse_node{ item, { left, expression(lbp(item.type) - 1) } };
    default:
      throw item;
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
  static constexpr int lbp(const Type type)
  {
    const auto precedence = [type]() -> Precedence {
      switch (type) {
      case Type::plus:
      case Type::minus:
        return Precedence::sum;
      case Type::asterisk:
      case Type::slash:
        return Precedence::product;
      case Type::caret:
        return Precedence::exponent;
      case Type::bang:
        return Precedence::postfix;
      case Type::left_brace:
      case Type::left_paren:
        return Precedence::call;
      case Type::right_paren:
        return Precedence::none;// right_paren will be parsed but not consumed when matching
      case Type::end_of_file:
        return Precedence::none;
      case Type::keyword:
        return Precedence::keyword;
      case Type::less_than_or_equal:
      case Type::less_than:
      case Type::greater_than_or_equal:
      case Type::greater_than:
        return Precedence::relational_comparison;
      case Type::logical_and:
        return Precedence::logical_and;
      case Type::logical_or:
        return Precedence::logical_or;
      case Type::equals:
      case Type::not_equals:
        return Precedence::equality_comparison;
      default:
        return Precedence::none;
        //        throw std::runtime_error(fmt::format("Unhandled value lbp {}", static_cast<int>(type)));
      }
    }();

    return static_cast<int>(precedence);
  }

  parse_node statement()
  {
    if (peek(Type::left_brace)) { return compound_statement(); }
    auto result = expression();

    if (result.item.type == Type::left_brace
        || (!result.children.empty() && result.children.back().item.type == Type::left_brace)) {
      // our last matched item was a compound statement of some sort, so we won't require a closing semicolon
    } else {
      match(Type::semicolon);
    }
    return result;
  }

  parse_node compound_statement()
  {
    auto result = parse_node{ match(Type::left_brace), {} };
    while (!peek(Type::right_brace)) { result.children.emplace_back(statement()); }
    match(Type::right_brace);
    return result;
  }

  parse_node expression(const int rbp = 0)
  {
    // parse prefix token
    const auto prefix = token;
    token = next();
    auto left = nud(prefix);

    // parse infix / postfix tokens
    while (rbp < lbp(token.type)) {
      const auto t = token;
      token = next();
      left = led(t, left);
    }

    return left;
  }

  constexpr lex_item next() noexcept { return next_token(token.remainder); }

  static constexpr lex_item next_token(std::string_view v) noexcept
  {
    while (true) {
      if (const auto item = lexer(v); item) {
        if (item->type != Type::whitespace) {
          return *item;
        } else {
          v = item->remainder;
        }
      } else {
        return lex_item{ Type::unknown, v, v };
      }
    }
  }

  static constexpr std::optional<lex_item> lexer(std::string_view v) noexcept
  {

    if (v.empty()) { return lex_item{ Type::end_of_file, v, v }; }

    // prefix with ^ so that the regex search only matches the start of the
    // string. Hana assures me this is efficient ;) (at runtime that is)
    constexpr auto make_token = [](const auto &s) { return ctll::fixed_string{ "^" } + ctll::fixed_string{ s }; };

    const auto ret = [v](const Type type, std::string_view found) -> lex_item {
      return { type, v.substr(0, found.size()), v.substr(found.size()) };
    };

    constexpr auto identifier{ make_token("[_a-zA-Z]+[_0-9a-zA-Z]*") };
    constexpr auto quoted_string{ make_token(R"("([^"\\]|\\.)*")") };
    constexpr auto floatingpoint_number{ make_token("[0-9]+[.][0-9]*([eEpP][0-9]+)?[lLfF]?") };
    constexpr auto integral_number{ make_token("([1-9][0-9]*|0[xX][0-9A-Fa-f]+|0[bB][01]+|0[0-7]+)") };
    //    constexpr auto integral_number{ make_token("[1-9][0-9]+") };
    constexpr auto whitespace{ make_token("\\s+") };

    using token = std::pair<std::string_view, Type>;
    constexpr std::array tokens{ token{ "++", Type::increment },
      token{ "--", Type::decrement },
      token{ ">=", Type::greater_than_or_equal },
      token{ "<=", Type::less_than_or_equal },
      token{ "!=", Type::not_equals },
      token{ "==", Type::equals },
      token{ "||", Type::logical_or },
      token{ "&&", Type::logical_and },
      token{ "<", Type::less_than },
      token{ ">", Type::greater_than },
      token{ ":", Type::colon },
      token{ ",", Type::comma },
      token{ "=", Type::assign },
      token{ "+", Type::plus },
      token{ "-", Type::minus },
      token{ "*", Type::asterisk },
      token{ "/", Type::slash },
      token{ "^", Type::caret },
      token{ "~", Type::tilde },
      token{ "%", Type::percent },
      token{ "!", Type::bang },
      token{ "?", Type::question },
      token{ "(", Type::left_paren },
      token{ ")", Type::right_paren },
      token{ "{", Type::left_brace },
      token{ "}", Type::right_brace },
      token{ "[", Type::left_bracket },
      token{ "]", Type::right_bracket },
      token{ ";", Type::semicolon } };

    constexpr std::array keywords{
      std::string_view{ "auto" }, std::string_view{ "for" }, std::string_view{ "if" }, std::string_view{ "else" }
    };

    if (auto result = ctre::search<whitespace>(v); result) { return ret(Type::whitespace, result); }

    for (const auto &oper : tokens) {
      if (v.starts_with(oper.first)) { return ret(oper.second, oper.first); }
    }

    if (auto id_result = ctre::search<identifier>(v); id_result) {
      const auto is_keyword = std::any_of(
        begin(keywords), end(keywords), [id = std::string_view{ id_result }](const auto &rhs) { return id == rhs; });
      return ret(is_keyword ? Type::keyword : Type::identifier, id_result);
    } else if (auto string_result = ctre::search<quoted_string>(v); string_result) {
      return ret(Type::string, string_result);
    } else if (auto float_result = ctre::search<floatingpoint_number>(v); float_result) {
      return ret(Type::number, float_result);
    } else if (auto int_result = ctre::search<integral_number>(v); int_result) {
      return ret(Type::number, int_result);
    }

    return std::nullopt;
  }
};
}// namespace parser_test

#endif
