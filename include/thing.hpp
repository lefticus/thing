#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
#include <variant>

struct StackInterface {};

template <typename... Contained> struct Stack : StackInterface {
  using value_type = std::variant<Contained...>;

  std::array<value_type, 256> values;
  std::size_t idx = 0;

  constexpr void push(const value_type &value) {
    if (!std::is_constant_evaluated()) {
      assert(idx < values.size());
    }
    values[idx++] = value;
  }

  constexpr void push(value_type &&value) {
    if (!std::is_constant_evaluated()) {
      assert(idx < values.size());
    }
    values[idx++] = std::move(value);
  }

  constexpr void push(auto &&val) {
    push(value_type{std::forward<decltype(val)>(val)});
  }

  [[nodiscard]] constexpr auto size() const { return idx; }

  [[nodiscard]] constexpr value_type &top(const std::size_t n = 0) {
    return values[idx - (1 + n)];
  }

  [[nodiscard]] constexpr const value_type &top(const std::size_t n = 0) const {
    return values[idx - (1 + n)];
  }

  template <typename Result> constexpr Result pop() {
    return std::move(std::get<Result>(values[--idx]));
  }

  constexpr void drop() { values[--idx] = value_type{}; }
};

constexpr void drop(auto &stack, std::uint64_t count = 0) {
  while (count != 0) {
    stack.drop();
    --count;
  }
}

struct Pop {
  constexpr void exec(auto &stack, auto &) const { drop(stack, 1); }
};

struct Nop {
  constexpr void exec(auto &, auto &) const noexcept {}
};

template <typename Type> struct PushLiteral {
  Type value;

  constexpr void exec(auto &stack, auto &) const { stack.push(value); }
};

struct RelativeStackReference {
  std::ptrdiff_t offset{0};
};

template <typename... T>
[[nodiscard]] constexpr auto resolve(Stack<T...> &stack,
                                     const std::size_t n = 0) ->
    typename Stack<T...>::value_type & {
  auto &stackObject = stack.top(n);
  auto *sr = std::get_if<RelativeStackReference>(&stackObject);
  if (sr != nullptr) {
    return resolve(stack, static_cast<std::size_t>(
                              sr->offset + static_cast<std::ptrdiff_t>(n)));
  } else {
    return stackObject;
  }
}

template <typename... T, typename ObjectType>
constexpr void set(Stack<T...> &stack, const std::size_t n,
                   ObjectType &&newObject) {
  resolve(stack, n) =
      typename Stack<T...>::value_type{std::forward<ObjectType>(newObject)};
}

// specific to the known stack type, if you make your own stack type you need
// to make your own peek
template <typename... Result, typename... T>
[[nodiscard]] constexpr auto peek(Stack<T...> &stack, const std::size_t n = 0) {
  if constexpr (sizeof...(Result) == 1) {
    // this is guaranteed to expand to only one element
    return std::get_if<Result...>(&resolve(stack, n));
  } else {
    using ResultType =
        std::variant<std::monostate, std::add_pointer_t<Result>...>;
    ResultType result;
    bool found = false;
    auto merge_result = [n, &result, &stack, &found]<typename Type>() {
      if (!found) {
        auto *ptr = peek<Type>(stack, n);

        if (ptr) {
          result = ResultType(ptr);
          found = true;
        }
      }
    };

    (merge_result.template operator()<Result>(), ...);
    return result;
  }
}

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

enum struct BinaryOps {
  Division,
  Addition,
  Subtraction,
  Multiplication,
  Equal_To,
  Not_Equal_To,
  Less_Than,
  Greater_Than,
  Less_Than_Or_Equal_To,
  Greater_Than_Or_Equal_To,
  Left_Shift,
  Right_Shift,
  Bitwise_And,
  Bitwise_Or,
  And,
  Or,
  Remainder
};

enum struct UnaryOps {
  Pre_Increment,
  Pre_Decrement,
  Post_Increment,
  Post_Decrement,
  Plus,
  Minus,
  Negation,
  Complement
};

template <typename T>
constexpr auto is_integral_arithmetic =
    std::is_integral_v<T> &&std::is_integral_v<T> && !std::is_same_v<T, char> &&
    !std::is_same_v<T, bool>;

template <typename T, typename U>
constexpr auto is_same_integral_signedness = 
    (!std::is_integral_v<T> || !std::is_integral_v<U> || std::is_signed_v<T> == std::is_signed_v<U>);


static_assert(is_same_integral_signedness<int, short>);
static_assert(is_same_integral_signedness<std::string, short>);
static_assert(!is_same_integral_signedness<unsigned, int>);



template <typename T, typename U>
constexpr auto are_both_float_or_not = (std::is_floating_point_v<T> &&
                                        std::is_floating_point_v<U>) ||
                                       (!std::is_floating_point_v<T> &&
                                        !std::is_floating_point_v<U>);

template <typename T, typename U>
constexpr auto use_safe_comparison =
    is_integral_arithmetic<T> &&is_integral_arithmetic<U>;

template <typename T, typename U>
constexpr auto is_desireable_op =
    are_both_float_or_not<T, U> &&is_same_integral_signedness<T, U>;

template <typename T>
constexpr auto is_bool_or_udt =
    std::is_same_v<bool, T> || !std::is_arithmetic_v<T>;

template <typename StackType, typename... Types>
constexpr bool execUnaryOp(const std::uint64_t arity,
                           StackInterface &stack_interface) {
  auto &stack = static_cast<StackType &>(stack_interface);

  if (arity != 2) {
    return false;
  }

  auto peeked_lhs = peek<Types...>(stack, 1);
  auto op_ptr = peek<std::uint64_t>(stack, 2);

  if (op_ptr == nullptr) {
    return false;
  }

  auto op = static_cast<UnaryOps>(*op_ptr);

  // return location is stack - 2

  auto visitor = [&stack, op]<typename LHS>(LHS *lhs_p) -> bool {
    auto &lhs = *lhs_p;

    switch (op) {
    case UnaryOps::Pre_Increment:
      if constexpr (requires { ++lhs; } && !std::is_same_v<LHS, bool>) {
        ++lhs;
        set(stack, 3, RelativeStackReference{-2});
        return true;
      }
      break;
    case UnaryOps::Pre_Decrement:
      if constexpr (requires { --lhs; }) {
        --lhs;
        set(stack, 3, RelativeStackReference{-2});
        return true;
      }
      break;
    case UnaryOps::Post_Increment:
      if constexpr (requires { lhs++; } && !std::is_same_v<LHS, bool>) {
        set(stack, 3, lhs++);
        return true;
      }
      break;
    case UnaryOps::Post_Decrement:
      if constexpr (requires { lhs--; }) {
        set(stack, 3, lhs--);
        return true;
      }
      break;
    case UnaryOps::Plus:
      if constexpr (requires { +lhs; }) {
        set(stack, 3, +lhs);
        return true;
      }
      break;
    case UnaryOps::Minus:
      if constexpr (requires { -lhs; }) {
        set(stack, 3, -lhs);
        return true;
      }
      break;
    case UnaryOps::Negation:
      if constexpr (requires { !lhs; } && is_bool_or_udt<LHS>) {
        set(stack, 3, !lhs);
        return true;
      }
      break;
    case UnaryOps::Complement:
      if constexpr (requires { ~lhs; } && !std::is_same_v<LHS, bool>) {
        set(stack, 3, ~lhs);
        return true;
      }
    }

    return false;
  };

  auto result = std::visit(
      overloaded{visitor, [](std::monostate) -> bool { return false; }}, peeked_lhs);

  return result;
}

template <typename StackType, typename... Types>
constexpr bool execBinaryOp(const std::uint64_t arity,
                            StackInterface &stack_interface) {
  auto &stack = static_cast<StackType &>(stack_interface);

  if (arity != 3) {
    return false;
  }

  auto peeked_rhs = peek<Types...>(stack, 1);
  auto peeked_lhs = peek<Types...>(stack, 2);
  auto op_ptr = peek<std::uint64_t>(stack, 3);

  if (op_ptr == nullptr) {
    return false;
  }

  auto op = static_cast<BinaryOps>(*op_ptr);

  // return location is stack - 2

  auto visitor = [&stack, op]<typename LHS, typename RHS>(LHS *lhs_p,
                                                          RHS *rhs_p) -> bool {
    auto &lhs = *lhs_p;
    auto &rhs = *rhs_p;

    switch (op) {
    case BinaryOps::Division:
      if constexpr (requires { lhs / rhs; } && is_desireable_op<LHS, RHS>) {
        set(stack, 4, lhs / rhs);
        return true;
      }
      break;
    case BinaryOps::Addition:
      if constexpr (requires { lhs + rhs; } && is_desireable_op<LHS, RHS>) {
        set(stack, 4, lhs + rhs);
        return true;
      }
      break;
    case BinaryOps::Subtraction:
      if constexpr (requires { lhs - rhs; } && is_desireable_op<LHS, RHS>) {
        set(stack, 4, lhs - rhs);
        return true;
      }
      break;
    case BinaryOps::Multiplication:
      if constexpr (requires { lhs *rhs; } && is_desireable_op<LHS, RHS>) {
        set(stack, 4, lhs * rhs);
        return true;
      }
      break;
    case BinaryOps::Equal_To:
      if constexpr (requires { lhs == rhs; }) {
        if constexpr (use_safe_comparison<LHS, RHS>) {
          set(stack, 4, std::cmp_equal(lhs, rhs));
          return true;
        } else if constexpr (is_desireable_op<LHS, RHS>) {
          set(stack, 4, lhs == rhs);
          return true;
        }
      }
      break;
    case BinaryOps::Greater_Than:
      if constexpr (requires { lhs > rhs; }) {
        if constexpr (use_safe_comparison<LHS, RHS>) {
          set(stack, 4, std::cmp_greater(lhs, rhs));
          return true;
        } else if constexpr (is_desireable_op<LHS, RHS>) {
          set(stack, 4, lhs > rhs);
          return true;
        }
      }
      break;
    case BinaryOps::Less_Than:
      if constexpr (requires { lhs < rhs; }) {
        if constexpr (use_safe_comparison<LHS, RHS>) {
          set(stack, 4, std::cmp_less(lhs, rhs));
          return true;
        } else if constexpr (is_desireable_op<LHS, RHS>) {
          set(stack, 4, lhs < rhs);
          return true;
        }
      }
      break;
    case BinaryOps::Left_Shift:
      if constexpr (requires { lhs << rhs; }) {
        set(stack, 4, lhs << rhs);
        return true;
      }
      break;
    case BinaryOps::Right_Shift:
      if constexpr (requires { lhs >> rhs; }) {
        set(stack, 4, lhs >> rhs);
        return true;
      }
      break;
    case BinaryOps::Less_Than_Or_Equal_To:
      if constexpr (requires { lhs <= rhs; }) {
        if constexpr (use_safe_comparison<LHS, RHS>) {
          set(stack, 4, std::cmp_less_equal(lhs, rhs));
          return true;
        } else if constexpr (is_desireable_op<LHS, RHS>) {
          set(stack, 4, lhs <= rhs);
          return true;
        }
      }
      break;
    case BinaryOps::Greater_Than_Or_Equal_To:
      if constexpr (requires { lhs >= rhs; }) {
        if constexpr (use_safe_comparison<LHS, RHS>) {
          set(stack, 4, std::cmp_greater_equal(lhs, rhs));
          return true;
        } else if constexpr (is_desireable_op<LHS, RHS>) {
          set(stack, 4, lhs >= rhs);
          return true;
        }
      }
      break;
    case BinaryOps::Not_Equal_To:
      if constexpr (requires { lhs != rhs; }) {
        if constexpr (use_safe_comparison<LHS, RHS>) {
          set(stack, 4, std::cmp_not_equal(lhs, rhs));
          return true;
        } else if constexpr (is_desireable_op<LHS, RHS>) {
          set(stack, 4, lhs != rhs);
          return true;
        }
      }
      break;
    case BinaryOps::Bitwise_And:
      if constexpr (requires { lhs &rhs; } &&
                    is_same_integral_signedness<LHS, RHS>) {
        set(stack, 4, lhs & rhs);
        return true;
      }
      break;
    case BinaryOps::Bitwise_Or:
      if constexpr (requires { lhs | rhs; } &&
                    is_same_integral_signedness<LHS, RHS>) {
        set(stack, 4, lhs | rhs);
        return true;
      }
      break;
    case BinaryOps::And:
      if constexpr (requires { lhs &&rhs; } && is_bool_or_udt<LHS> &&
                    is_bool_or_udt<RHS>) {
        set(stack, 4, lhs && rhs);
        return true;
      }
      break;
    case BinaryOps::Or:
      if constexpr (requires { lhs || rhs; } && is_bool_or_udt<LHS> &&
                    is_bool_or_udt<RHS>) {
        set(stack, 4, lhs || rhs);
        return true;
      }
      break;
    case BinaryOps::Remainder:
      if constexpr (requires { lhs % rhs; } &&
                    is_same_integral_signedness<LHS, RHS>) {
        set(stack, 4, lhs % rhs);
        return true;
      }
      break;
    }

    return false;
  };

  auto result = std::visit(
      overloaded{visitor,
                 [](const auto &, std::monostate) -> bool { return false; },
                 [](std::monostate, const auto &) -> bool { return false; },
                 [](std::monostate, std::monostate) -> bool { return false; }},
      peeked_lhs, peeked_rhs);

  return result;
}

struct Function {
  using exec_func_type = bool (*)(const std::uint64_t, StackInterface &);

  exec_func_type exec_func;

  constexpr void exec(std::uint64_t arity, StackInterface &stack) {
    exec_func(arity, stack);
  }
};

void jump(auto &operations, const std::ptrdiff_t distance) {
  operations.pc = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(operations.pc) + distance);
}

struct RelativeJump {
  bool conditional = false;
  std::ptrdiff_t distance;

  constexpr void exec(auto &stack, auto &ops) const {
    if (conditional) {
      bool *dojump = peek<bool>(stack);
      if (dojump != nullptr && *dojump == true) {
        jump(ops, distance);
      }
    } else {
      jump(ops, distance);
    }
  }
};

struct CallFunction {
  bool popstack{false};
  constexpr void exec(auto &stack, auto &) const {
    auto *arity = peek<std::uint64_t>(stack, 0);

    if (arity != nullptr) {
      auto *function = peek<Function>(stack, *arity + 2);
      if (function != nullptr) {
        function->exec(*arity, stack);

        if (popstack) {
          drop(stack, *arity + 3); // arity + function to call + return value
        }
      }
    }
  }
};

template <typename StackType, typename Ret, bool throws, typename... Param>
constexpr auto exec(Ret (*func)(Param...) noexcept(throws),
                    StackInterface &genericStack) -> Ret {
  const auto exec_ = [&func]<std::size_t... Idx>(std::index_sequence<Idx...>,
                                      StackType &stack)
                   ->decltype(auto) {
    // values are pushed on to the stack in left->right ordering, which means
    // that we need to do something like Idx-2, Idx-1, Idx-0
    //
    const auto pointers = std::tuple{peek<Param>(stack, (sizeof...(Idx)) - Idx)...};

    const bool any_null = ((std::get<Idx>(pointers) == nullptr) || ...);
    if (any_null) { throw "ooops"; }
    return func(*std::get<Idx>(pointers)...);
  };

  auto &actualStack = [&]() -> StackType & {
    if constexpr (std::is_polymorphic_v<StackType>) {
      // if the stack type is polymorphic, then we want to use a proper dynamic
      // cast
      return dynamic_cast<StackType &>(genericStack);
    } else {
      // otherwise we are doing an unchecked up-cast which is possible to have a
      // problem
      return static_cast<StackType &>(genericStack);
    }
  }();

  return (
      exec_(std::make_index_sequence<sizeof...(Param)>(), actualStack));
}

template <typename StackType> struct Operations {
  using Types =
      std::variant<Nop, Pop, PushLiteral<typename StackType::value_type>,
                   RelativeJump, CallFunction>;

  std::array<Types, 256> values;
  std::uint64_t idx{0};
  std::uint64_t pc{0};

  constexpr void push_back(Types op) { values[idx++] = std::move(op); }

  template <auto FunctionPtr> static constexpr Function make_function() {
    // todo use the arity!!
    auto executor = []([[maybe_unused]] const std::uint64_t arity,
                       StackInterface &stack) -> bool {
      exec<StackType>(FunctionPtr, stack);
      return true;
    };

    return {+executor};
  }

  constexpr bool next(StackType &stack) {
    std::visit([&](auto &value) { value.exec(stack, *this); }, values[pc]);
    ++pc;
    return !std::holds_alternative<Nop>(values[pc]);
  }
};
