#pragma once

#include "Deque.h"

namespace lockbased {


class Stack : Deque {
 public:
  void push(int const& data) { return Deque::push_front(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace lockbased
