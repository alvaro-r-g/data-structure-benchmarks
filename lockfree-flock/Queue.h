// Adapted from lockbased/Queue.h

#pragma once

#include "Deque.h"

namespace lockfree_flock {

template <typename T>
class Queue : Deque {
 public:
  void push(T const& data) { return Deque::push_back(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace lockfree_flock
