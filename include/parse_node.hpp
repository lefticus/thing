#ifndef THING_PARSE_NODE_HPP
#define THING_PARSE_NODE_HPP

#include "lex_item.hpp"
#include <vector>


namespace thing::parsing {
struct parse_node
{
  enum struct error_type { no_error, wrong_token_type, unexpected_prefix_token, unexpected_infix_token };

  lexing::lex_item item;
  std::vector<parse_node> children;

  error_type error{ error_type::no_error };
  lexing::token_type expected_token{};

  [[nodiscard]] constexpr bool is_error() const noexcept { return error != error_type::no_error; }

  explicit parse_node(lexing::lex_item item_, std::vector<parse_node> &&children_ = {})
    : item{ std::move(item_) }, children{ std::move(children_) }
  {}

 // parse_node(lexing::lex_item item_, std::vector<parse_node> &&children_)
 //   : item{ std::move(item_) }, children{ std::move(children_) }
 // {}

  parse_node(lexing::lex_item item_, error_type error_, lexing::token_type expected_ = {})
    : item{ std::move(item_) }, error{ error_ }, expected_token{ expected_ }
  {}
};

}
#endif// MYPROJECT_PARSE_NODE_HPP
