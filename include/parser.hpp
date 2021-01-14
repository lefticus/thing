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

  struct parse_node
  {
    enum struct error_type { no_error, wrong_token_type, unexpected_prefix_token, unexpected_infix_token };

    lex_item item;
    std::vector<parse_node> children;

    error_type error{ error_type::no_error };
    Token_Type expected_token{};

    [[nodiscard]] constexpr bool is_error() const noexcept { return error != error_type::no_error; }
    explicit parse_node(lex_item item_, std::initializer_list<parse_node> children_ = {})
      : item{ std::move(item_) }, children{ children_ }
    {}

    parse_node(lex_item item_, std::vector<parse_node> &&children_)
      : item{ std::move(item_) }, children{ std::move(children_) }
    {}

    parse_node(lex_item item_, error_type error_, Token_Type expected_ = {})
      : item{ std::move(item_) }, error{ error_ }, expected_token{ expected_ }
    {}
  };

  lex_item token;


  auto parse(std::string_view v)
  {
    token = next_token(v);
    return expression();
  }

  constexpr bool peek(const Token_Type type, const std::string_view value = "") const
  {
    return token.type == type && (value.empty() || token.match == value);
  }

  // if a match is not possible, an error node is returned.
  [[nodiscard]] parse_node consume_match(const Token_Type type)
  {
    if (token.type != type) {
      return parse_node{ std::exchange(token, next()), parse_node::error_type::wrong_token_type, type };
    }
    return parse_node{ std::exchange(token, next()), {} };
  }

  parse_node list(bool consumeOpener, Token_Type opener, Token_Type closer, Token_Type delimiter)
  {
    auto result = [&]() {
      if (consumeOpener) {
        return consume_match(opener);
      } else {
        return parse_node{ lex_item{ Token_Type::unknown, {}, {} } };
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

  parse_node control_block(const lex_item &item)
  {
    parse_node result{ item,
      { list(true, Token_Type::left_paren, Token_Type::right_paren, Token_Type::semicolon), statement() } };
    if (peek(Token_Type::keyword, "else")) {
      result.children.push_back(parse_node{ consume_match(Token_Type::keyword).item, { statement() } });
    }
    return result;
  }

  // prefix nodes
  parse_node null_denotation(const lex_item &item)
  {
    constexpr auto prefix_precedence = static_cast<int>(Precedence::prefix);

    switch (item.type) {
    case Token_Type::identifier:
    case Token_Type::number:
    case Token_Type::string:
      return parse_node{ item };
    case Token_Type::keyword:
      if (item.match == "if" || item.match == "for" || item.match == "while") {
        return control_block(item);
      } else {
        return parse_node{ item, { parse_node{ expression(prefix_precedence) } } };
      }
    case Token_Type::plus:
    case Token_Type::minus:
      return parse_node{ item, { parse_node{ expression(prefix_precedence) } } };
    case Token_Type::left_paren: {
      const auto result = expression();
      if (const auto match_result = consume_match(Token_Type::right_paren); match_result.is_error()) {
        return match_result;
      }

      return result;
    }
    default:
      return parse_node{ item, parse_node::error_type::unexpected_prefix_token };
    };
  }

  // infix processing
  [[nodiscard]] parse_node left_denotation(const lex_item &item, const parse_node &left)
  {
    switch (item.type) {
    case Token_Type::plus:
    case Token_Type::minus:
    case Token_Type::asterisk:
    case Token_Type::slash:
    case Token_Type::less_than:
    case Token_Type::less_than_or_equal:
    case Token_Type::greater_than:
    case Token_Type::greater_than_or_equal:
    case Token_Type::logical_and:
    case Token_Type::logical_or:
    case Token_Type::equals:
    case Token_Type::not_equals:
      return parse_node{ item, { left, expression(lbp(item.type)) } };
    case Token_Type::bang:
      return parse_node{ item, { left } };
    case Token_Type::left_paren: {
      auto result = left;
      result.children.emplace_back(
        item, list(false, Token_Type::left_paren, Token_Type::right_paren, Token_Type::comma).children);
      return result;
    }
    case Token_Type::left_brace: {
      auto result = left;
      result.children.push_back(parse_node{ item, { expression() } });
      if (const auto match_result = consume_match(Token_Type::right_brace); match_result.is_error()) {
        return match_result;
      }
      return result;
    }
    case Token_Type::caret:
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
  [[nodiscard]] static constexpr int lbp(const Token_Type type) noexcept
  {
    const auto precedence = [type]() -> Precedence {
      switch (type) {
      case Token_Type::plus:
      case Token_Type::minus:
        return Precedence::sum;
      case Token_Type::asterisk:
      case Token_Type::slash:
        return Precedence::product;
      case Token_Type::caret:
        return Precedence::exponent;
      case Token_Type::bang:
        return Precedence::postfix;
      case Token_Type::left_brace:
      case Token_Type::left_paren:
        return Precedence::call;
      case Token_Type::right_paren:
        return Precedence::none;// right_paren will be parsed but not consumed when matching
      case Token_Type::end_of_file:
        return Precedence::none;
      case Token_Type::keyword:
        return Precedence::keyword;
      case Token_Type::less_than_or_equal:
      case Token_Type::less_than:
      case Token_Type::greater_than_or_equal:
      case Token_Type::greater_than:
        return Precedence::relational_comparison;
      case Token_Type::logical_and:
        return Precedence::logical_and;
      case Token_Type::logical_or:
        return Precedence::logical_or;
      case Token_Type::equals:
      case Token_Type::not_equals:
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
    if (peek(Token_Type::left_brace)) { return compound_statement(); }
    auto result = expression();

    if (result.item.type == Token_Type::left_brace
        || (!result.children.empty() && result.children.back().item.type == Token_Type::left_brace)) {
      // our last matched item was a compound statement of some sort, so we won't require a closing semicolon
    } else {
      if (const auto match_result = consume_match(Token_Type::semicolon); match_result.is_error()) {
        return match_result;
      }
    }
    return result;
  }

  [[nodiscard]] parse_node compound_statement()
  {
    auto result = consume_match(Token_Type::left_brace);
    if (result.is_error()) { return result; }
    while (!peek(Token_Type::right_brace)) { result.children.emplace_back(statement()); }
    // we peeked, it'll consume_match
    [[maybe_unused]] const auto right_brace = consume_match(Token_Type::right_brace);
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

  [[nodiscard]] constexpr lex_item next() const noexcept { return next_token(token.remainder); }

  static constexpr lex_item next_token(std::string_view v) noexcept
  {
    while (true) {
      if (const auto item = lexer(v); item) {
        if (item->type != Token_Type::whitespace) {
          return *item;
        } else {
          v = item->remainder;
        }
      } else {
        return lex_item{ Token_Type::unknown, v, v };
      }
    }
  }

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
    constexpr auto integral_number{ make_token("([0-9]+|0[xX][0-9A-Fa-f]+|0[bB][01]+|0[oO][0-7]+)") };
    //    constexpr auto integral_number{ make_token("[1-9][0-9]+") };
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
};
}// namespace parser_test

#endif
