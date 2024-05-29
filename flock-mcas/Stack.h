#pragma once

#include "Deque.h"

namespace flock_mcas {

template <typename T>
class Stack : Deque {
 public:
  void push(T const& data) { return Deque::push_front(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace flock_mcas
