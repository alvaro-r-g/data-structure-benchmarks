#pragma once

#include "Deque.h"

namespace flock_mcas {

template <typename T>
class Queue : Deque {
 public:
  void push(T const& data) { return Deque::push_back(data); }
  int pop() { return Deque::pop_front(); }
};

}  // namespace flock_mcas
