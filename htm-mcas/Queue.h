// Adapted from lockfree-mcas/Queue.h

#pragma once

#include "Deque.h"

namespace htm_mcas {

template <typename T>
class Queue : Deque {
 public:
  void push(T const& data) { return Deque::push_back(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace htm_mcas
