// Adapted from lockbased/Stack.h

#pragma once

#include "Deque.h"

namespace htm_lock {

class Stack : Deque {
 public:
  void push(int const& data) { return Deque::push_front(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace htm_lock
