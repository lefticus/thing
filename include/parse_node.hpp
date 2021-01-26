#ifndef THING_PARSE_NODE_HPP
#define THING_PARSE_NODE_HPP

#include "lex_item.hpp"
#include <vector>


namespace thing::parsing {

template<template<class> class Container_Type> struct basic_parse_node
{
  enum struct error_type { no_error, wrong_token_type, unexpected_prefix_token, unexpected_infix_token };

  using container_type = Container_Type<basic_parse_node>;

  using allocator_type = typename container_type::allocator_type;

  lexing::lex_item item;

  container_type children;

  error_type error{ error_type::no_error };
  lexing::token_type expected_token{};

  [[nodiscard]] constexpr bool is_error() const noexcept { return error != error_type::no_error; }

  [[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return children.get_allocator(); }

  constexpr explicit basic_parse_node(lexing::lex_item item_,
    container_type &&children_ = {},
    allocator_type alloc = {})
    : item{ std::move(item_) }, children{ std::move(children_), alloc }
  {}

  constexpr basic_parse_node(lexing::lex_item item_,
    error_type error_,
    lexing::token_type expected_ = {},
    allocator_type alloc = {})
    : item{ std::move(item_) }, children{ alloc }, error{ error_ }, expected_token{ expected_ }
  {}

  constexpr basic_parse_node() : basic_parse_node(allocator_type{}) {}
  constexpr explicit basic_parse_node(allocator_type alloc) : children(alloc) {}
  constexpr basic_parse_node(basic_parse_node &&) noexcept = default;
  constexpr basic_parse_node(const basic_parse_node &other, allocator_type alloc = {}) : children( other.children, alloc ) {}
  ~basic_parse_node() = default;
  constexpr basic_parse_node(basic_parse_node &&other, allocator_type alloc) : children( std::move(other.children), alloc ) {}
  basic_parse_node &operator=(basic_parse_node &&) noexcept = default;
  basic_parse_node &operator=(const basic_parse_node &) = default;
};


}// namespace thing::parsing
#endif// MYPROJECT_PARSE_NODE_HPP
