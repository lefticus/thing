#ifndef THING_AST_HPP
#define THING_AST_HPP

#include <variant>
#include <vector>
#include <string_view>
#include "lex_item.hpp"
#include "parse_node.hpp"

namespace thing::ast {

// todo: I think we can remove this class altogether and use
// function argument deduction for all of the ast_building functions
template<template<class> class Container_Type> struct basic_ast_builder
{
  struct variable_declaration;
  struct variable_definition;
  struct function_definition;
  struct function_call;
  struct prefix_operator;
  struct infix_operator;

  struct if_statement;
  struct for_statement;
  struct while_statement;

  //  using allocator_type = typename container_type::allocator_type;

  using parse_node = parsing::basic_parse_node<Container_Type>;
  using allocator_type = typename parse_node::allocator_type;

  struct parse_error
  {
    std::string_view error_description;
    std::reference_wrapper<const parse_node> error_location;
  };

  struct compound_statement;
  struct literal_value
  {
    lexing::lex_item value;
  };

  struct identifier
  {
    lexing::lex_item id;
  };

  using expression = std::variant<function_call, prefix_operator, infix_operator, literal_value, identifier>;

  struct function_call
  {
    std::unique_ptr<expression> function;
    Container_Type<expression> parameters;
  };

  struct prefix_operator
  {
    lexing::lex_item op;
    std::unique_ptr<expression> operand;
  };

  struct infix_operator
  {
    std::unique_ptr<expression> lhs_operand;
    lexing::lex_item op;
    std::unique_ptr<expression> rhs_operand;
  };

  template<typename... Types> struct merge_types
  {
    using type = std::variant<Types...>;
  };

  template<typename... RHS> struct merge_types<parse_error, std::variant<RHS...>>
  {
    using type = std::variant<parse_error, RHS...>;
  };

  template<typename... LHS, typename... RHS> struct merge_types<std::variant<LHS...>, std::variant<RHS...>>
  {
    using type = std::variant<LHS..., RHS...>;
  };

  template<typename... LHS, typename... RHS>
  struct merge_types<std::variant<parse_error, LHS...>, std::variant<parse_error, RHS...>>
  {
    using type = std::variant<parse_error, LHS..., RHS...>;
  };

  template<typename... LHS, typename... RHS> struct merge_types<std::variant<LHS...>, std::variant<parse_error, RHS...>>
  {
    using type = std::variant<parse_error, LHS..., RHS...>;
  };

  template<typename... LHS, typename... RHS> struct merge_types<std::variant<parse_error, LHS...>, std::variant<RHS...>>
  {
    using type = std::variant<parse_error, LHS..., RHS...>;
  };

  template<typename... LHS, typename... RHS> struct merge_types<std::variant<parse_error, LHS...>, RHS...>
  {
    using type = std::variant<parse_error, LHS..., RHS...>;
  };

  template<typename... LHS, typename... RHS> struct merge_types<std::variant<LHS...>, RHS...>
  {
    using type = std::variant<LHS..., RHS...>;
  };


  template<typename... Types> using merge_types_t = typename merge_types<Types...>::type;

  [[nodiscard]] static constexpr bool is_parse_error(const auto &value) noexcept
  {
    return std::holds_alternative<parse_error>(value);
  }

  template<typename RetType, typename Value> static constexpr auto return_current_value(Value &&value) -> RetType
  {
    return std::visit(
      []<typename Inner>(Inner &&inner) -> RetType { return std::forward<Inner>(inner); }, std::forward<Value>(value));
  }


  template<typename Func> struct parser
  {
    Func func;

    [[nodiscard]] constexpr auto parse(const parse_node &node, allocator_type alloc) const { return func(node, alloc); }

    template<typename RHS> consteval auto operator|(const parser<RHS> &rhs)
    {
      using return_type = merge_types_t<decltype(parse(std::declval<parse_node>(), std::declval<allocator_type>())),
        decltype(rhs.parse(std::declval<parse_node>(), std::declval<allocator_type>()))>;

      auto combined = [lhs = *this, rhs](const parse_node &node, allocator_type alloc) -> return_type {
        auto lhs_result = lhs.parse(node, alloc);
        if (is_parse_error(lhs_result)) {
          auto rhs_result = rhs.parse(node, alloc);
          if (is_parse_error(rhs_result)) {
            return parse_error{ "Unexpected expression", node };
          } else {
            return return_current_value<return_type>(std::move(rhs_result));
          }
        } else {
          return return_current_value<return_type>(std::move(lhs_result));
        }
      };

      return make_parser(combined);
    }
  };

  template<typename Func> [[nodiscard]] static constexpr auto make_parser(Func &&f) noexcept
  {
    return parser<std::decay_t<Func>>{ std::move(f) };
  }

  // we should probably remove this one and only use the visit_non_error_values version, tbd

  template<typename Value> static constexpr void visit_non_error_value(Value &&value, auto &&visitor)
  {
    std::visit(
      [&]<typename Inner>(Inner &&inner) -> void {
        if constexpr (!std::is_same_v<Inner, parse_error>) { visitor(inner); }
      },
      std::forward<Value>(value));
  }

  template<typename First, typename... Remainder>
  static constexpr parse_error return_first_error(const First &first, const Remainder &...remainder)
  {
    if constexpr (std::is_same_v<parse_error, std::decay_t<First>>) {
      return first;
    } else {
      return return_first_error(remainder...);
    }
  }

  template<typename ReturnType, typename... Value>
  static constexpr ReturnType visit_non_error_values(auto &&visitor, Value &&...value)
  {
    return std::visit(
      [&]<typename... Inner>(Inner && ...inner)->ReturnType {
        if constexpr (!(std::is_same_v<std::decay_t<Inner>, parse_error> || ...)) {
          return visitor(std::forward<Inner>(inner)...);
        } else {
          return return_first_error(inner...);
        }
      },
      std::forward<Value>(value)...);
  }


  using statement =
    merge_types_t<expression, variable_definition, if_statement, for_statement, while_statement, compound_statement>;
  using init_statement = merge_types_t<expression, variable_definition>;

  struct compound_statement
  {
    std::vector<statement> statements;
  };

  struct if_statement
  {
    std::unique_ptr<init_statement> init;
    expression condition;
    std::unique_ptr<statement> true_case;
    std::unique_ptr<statement> false_case;
  };

  struct for_statement
  {
    std::unique_ptr<init_statement> init;
    expression condition;
    expression loop_expression;
    std::unique_ptr<statement> body;
  };

  struct while_statement
  {
    expression condition;
    std::unique_ptr<statement> body;
  };

  struct function_definition
  {
    lexing::lex_item name;
    Container_Type<variable_declaration> parameters;
    std::unique_ptr<statement> body;
  };

  struct variable_declaration
  {
    lexing::lex_item name;
  };

  struct variable_definition
  {
    lexing::lex_item name;
    expression initial_value;
  };


  [[nodiscard]] static constexpr merge_types_t<parse_error, variable_declaration>
    build_variable_decl(const parse_node &node, [[maybe_unused]] allocator_type alloc)
  {
    if (node.item.type != lexing::token_type::keyword || node.item.match != "auto" || node.children.size() != 1
        || node.children[0].item.type != lexing::token_type::identifier || !node.children[0].children.empty()) {
      return parse_error{ "Expected variable declaration in the form of `auto <identifier>`", node };
    }

    return variable_declaration{ node.item };
  }


  [[nodiscard]] static constexpr merge_types_t<parse_error, Container_Type<variable_declaration>>
    build_variable_list(const parse_node &node, allocator_type alloc)
  {
    if (node.item.type != lexing::token_type::left_paren) { return parse_error{ "expected parenthesized list", node }; }

    Container_Type<variable_declaration> retval(alloc);
    retval.reserve(node.children.size());

    for (const auto &child : node.children) {
      auto decl = build_variable_decl(child, alloc);
      if (is_parse_error(decl)) { return std::get<parse_error>(decl); }
      retval.push_back(std::get<variable_declaration>(decl));
    }

    return retval;
  }


  [[nodiscard]] static constexpr merge_types_t<parse_error, variable_definition>
    build_variable_definition(const parse_node &node, allocator_type alloc)
  {
    if (node.item.type == lexing::token_type::keyword && node.item.match == "auto" && node.children.size() == 1
        && node.children[0].item.type == lexing::token_type::identifier && node.children[0].children.size() == 1
        && node.children[0].children[0].item.type == lexing::token_type::left_brace
        && node.children[0].children[0].children.size() == 1) {
      const auto name = node.children[0].item;
      auto initial_expression = build_expression(node.children[0].children[0].children[0], alloc);
      return visit_non_error_values<std::variant<parse_error, variable_definition>>(
        [&name](auto &&initial) {
          return variable_definition{ name, std::move(initial) };
        },
        initial_expression);
    } else {
      return parse_error{
        "Expected variable definition in the form of `auto <identifier> {<initial value expression>};", node
      };
    }
  }

  [[nodiscard]] static constexpr merge_types_t<parse_error, function_call> build_function_call(const parse_node &node,
    [[maybe_unused]] allocator_type alloc)
  {
    return parse_error{ "Function call parsing not yet implemented", node };
  }
  [[nodiscard]] static constexpr merge_types_t<parse_error, prefix_operator>
    build_prefix_operator(const parse_node &node, [[maybe_unused]] allocator_type alloc)
  {
    return parse_error{ "Prefix operator parsing not yet implemented", node };
  }
  [[nodiscard]] static constexpr merge_types_t<parse_error, infix_operator> build_infix_operator(const parse_node &node,
    [[maybe_unused]] allocator_type alloc)
  {
    return parse_error{ "Infix operator parsing not yet implemented", node };
  }
  [[nodiscard]] static constexpr merge_types_t<parse_error, literal_value> build_literal_value(const parse_node &node,
    [[maybe_unused]] allocator_type alloc)
  {
    return parse_error{ "Literal value parsing not yet implemented", node };
  }
  [[nodiscard]] static constexpr merge_types_t<parse_error, identifier> build_identifier(const parse_node &node,
    [[maybe_unused]] allocator_type alloc)
  {
    return parse_error{ "Identifier parsing not yet implemented", node };
  }
  [[nodiscard]] static constexpr merge_types_t<parse_error, while_statement>
    build_while_statement(const parse_node &node, [[maybe_unused]] allocator_type alloc)
  {
    return parse_error{ "While statement parsing not yet implemented", node };
  }
  [[nodiscard]] static constexpr merge_types_t<parse_error, for_statement> build_for_statement(const parse_node &node,
    [[maybe_unused]] allocator_type alloc)
  {
    return parse_error{ "For statement parsing not yet implemented", node };
  }
  [[nodiscard]] static constexpr merge_types_t<parse_error, if_statement> build_if_statement(const parse_node &node,
    [[maybe_unused]] allocator_type alloc)
  {
    return parse_error{ "If statement parsing not yet implemented", node };
  }

  [[nodiscard]] static constexpr merge_types_t<parse_error, expression> build_expression(const parse_node &node,
    [[maybe_unused]] allocator_type alloc)
  {
    return (make_parser(build_function_call) | make_parser(build_prefix_operator) | make_parser(build_infix_operator)
            | make_parser(build_literal_value) | make_parser(build_identifier))
      .parse(node, alloc);
  }

  [[nodiscard]] static constexpr merge_types_t<parse_error, statement> build_statement(const parse_node &node,
    allocator_type alloc)
  {
    return (make_parser(build_expression) | make_parser(build_variable_definition) | make_parser(build_if_statement)
            | make_parser(build_for_statement) | make_parser(build_while_statement)
            | make_parser(build_compound_statement))
      .parse(node, alloc);
  }

  [[nodiscard]] static constexpr merge_types_t<parse_error, compound_statement>
    build_compound_statement(const parse_node &node, allocator_type alloc)
  {
    if (node.item.type != lexing::token_type::left_brace) {
      return parse_error{ "Expected compound statement syntax: `{ /* list of statements */ }`", node };
    }

    Container_Type<statement> return_value(alloc);

    for (const auto &child : node.children) {
      if (auto s = build_statement(child, alloc); !is_parse_error(s)) {
        visit_non_error_value(std::move(s), [&](auto &&value) { return_value.push_back(std::move(value)); });
      } else {
        return std::get<parse_error>(std::move(s));
      }
    }

    return compound_statement{ std::move(return_value) };
  }

  [[nodiscard]] static constexpr merge_types_t<parse_error, function_definition>
    build_function_ast(const parse_node &node, allocator_type alloc)
  {
    if (!(node.item.type == lexing::token_type::keyword && node.item.match == "auto" && node.children.size() == 1
          && node.children.front().item.type == lexing::token_type::identifier)) {
      return parse_error{
        "Expected function definition syntax: `auto <function_name> (<parameter list...>) <compound_statement>", node
      };
    }

    auto parameter_list = build_variable_list(node.children[0].children[0], alloc);
    auto function_body = build_compound_statement(node.children[0].children[1], alloc);

    //  const auto retval = function_definition{
    //    node.children[0].item,
    //  };

    return function_definition{};
  }

  //  [[nodiscard]] static constexpr auto build_ast(const parse_node &node, allocator_type alloc = {})
  //  {
  //    return
  //  }
};
}// namespace thing::ast

#endif
