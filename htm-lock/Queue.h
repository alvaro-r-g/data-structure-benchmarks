// Adapted from lockbased/Queue.h

#pragma once

#include "Deque.h"

namespace htm_lock {

class Queue : Deque {
 public:
  void push(int const& data) { return Deque::push_back(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace htm_lock
