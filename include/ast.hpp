#ifndef THING_AST_HPP
#define THING_AST_HPP

#include <variant>
#include <vector>
#include <string_view>
#include "lex_item.hpp"
#include "parse_node.hpp"

namespace thing::ast {
struct variable_declaration;
struct function_declaration;
struct function_call;
struct prefix_operator;
struct infix_operator;

struct if_statement;
struct for_statement;
struct compound_statement;
struct literal_value
{
  lexing::lex_item value;
} struct identifier
{
  lexing::lex_item id;
};

using expression = std::variant<function_call, prefix_operator, infix_operator, literal_value, identifier>;
using statement = std::variant<variable_declaration, expression, if_statement, for_statement, compound_statement>;
using init_statement = std::variant<expression, variable_declaration>;

struct prefix_operator
{
  lexing::lex_item op;
  expression operand;
};

struct infix_operator
{
  expression lhs_operand;
  lexing::lex_item op;
  expression rhs_operand;
};

struct compound_statement
{
  std::vector<statement> statements;
};

struct if_statement
{
  init_statement init;
  expression condition;
  statement true_case;
  statement false_case;
};

struct for_statement
{
  init_statement init;
  expression condition;
  expression loop_expression;
  statement body;
};

struct while_statement
{
  expression condition;
  statement body;
};

struct function_declaration
{
  lexing::lex_item name;
  std::vector<variable_declaration> parameters;
  statement body;
};

struct variable_declaration
{
  lexing::lex_item name;
  expression initial_value;
};

struct function_call
{
  expression function;
  std::vector<expression> parameters;
};

function_declaration build_function_ast(const parsing::parse_node &node)
{
  //    if (node.)
  return function_declaration{};
}
}// namespace thing::ast

#endif// MYPROJECT_AST_HPP
