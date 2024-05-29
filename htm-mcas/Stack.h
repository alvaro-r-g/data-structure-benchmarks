// Adapted from lockfree-mcas/Stack.h

#pragma once

#include "Deque.h"

namespace htm_mcas {

template <typename T>
class Stack : Deque {
 public:
  void push(T const& data) { return Deque::push_front(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace htm_mcas
