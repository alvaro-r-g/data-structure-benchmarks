#pragma once

#include "Deque.h"

namespace lockbased {


class Queue : Deque {
 public:
  void push(int const& data) { return Deque::push_back(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace lockbased
