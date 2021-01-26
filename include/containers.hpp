#ifndef THING_CONTAINERS_HPP
#define THING_CONTAINERS_HPP


#include <variant>

namespace thing {

/*
template<typename Type>
struct ref
{
  std::size_t index;
  void *container;

  Type *(*lookup)(void *, std::size_t);

  Type *operator->() {
    return lookup(container, index);
  }
};

template<typename ... Type>
struct allocator
{
  using contained = std::variant<Type...>;

  std::vector<contained> data;

  template<typename ... Param>
  ref<RequestedType> create(Param && ... param) {
    data.emplace_back(std::forward<Param>(param)...);
    return ref<RequestedType>{data.size() - 1, this, []()};
  }
};

template<typename Type>
struct list
{
  struct list_node
  {
    Type obj;
    list_node *next;
  };
};
*/

}

#endif
