#ifndef PARSER_TEST_ALGORITHMS_HPP
#define PARSER_TEST_ALGORITHMS_HPP

#include <iterator>
#include <algorithm>
#include <utility>
#include <string_view>

namespace parser_test {

template<typename T>
constexpr bool BiDirIterator = std::is_convertible_v<typename std::iterator_traits<T>::iterator_category, std::bidirectional_iterator_tag>;


template<typename Iterator, typename UnaryPredicate>
[[nodiscard]] constexpr auto count_if_to_last(Iterator begin, Iterator end, UnaryPredicate predicate) requires BiDirIterator<Iterator>
{
  std::reverse_iterator rbegin{ end };
  const std::reverse_iterator rend{ begin };

  const auto last_item = std::find_if(rbegin, rend, predicate);
  const auto count = std::count_if(last_item, rend, predicate);

  return std::pair{ count, std::prev(last_item.base()) };
}

template<typename Iterator, typename Value>
[[nodiscard]] constexpr auto count_to_last(Iterator begin, Iterator end, const Value &value)
{
  return count_if_to_last(begin, end, [&](const auto &input) { return input == value; });
}


}// namespace parser_test

#endif
