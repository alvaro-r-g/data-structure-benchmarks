// Original code from Patel et al. 2017 paper
// A hardware implementation of the MCAS synchronization primitive

#pragma once

namespace lockfree {

class Queue {
  class Node {
   public:
    int value;
    Node *next;
    Node(int value) {
      this->value = value;
      this->next = nullptr;
    }
  };

 private:
  Node *head;
  Node *tail;

  static size_t compareAndExchange( volatile size_t* addr, size_t oldval, size_t
  newval ){
    size_t ret;
    __asm__ volatile( "lock cmpxchg %2, %1\n\t":"=a"(ret), "+m"(*addr):
    "r"(newval), "0"(oldval): "memory" );
    return ret;
  }

 public:
  Queue() {
    Node *sentinel = new Node(-1);
    this->head = sentinel;
    this->tail = sentinel;
  }

 public:
  void push(int item) {
    Node *node = new Node(item);
    Node *last, *next;
    while (true) {
      last = tail;  // read tail
      next = last->next;
      if (last == tail) {
        if (next == nullptr) {
          if (reinterpret_cast<Node *>(compareAndExchange(
                  reinterpret_cast<volatile size_t *>(&last->next),
                  reinterpret_cast<size_t>(next),
                  reinterpret_cast<size_t>(node))) == next) {
            compareAndExchange(reinterpret_cast<volatile size_t *>(&tail),
                               reinterpret_cast<size_t>(last),
                               reinterpret_cast<size_t>(node));
            return;
          }
        } else {
          compareAndExchange(reinterpret_cast<volatile size_t *>(&tail),
                             reinterpret_cast<size_t>(last),
                             reinterpret_cast<size_t>(next));
        }
      }
    }
  }

  int pop() {
    while (true) {
      Node *first = head;
      Node *last = tail;
      Node *next = first->next;
      if (first == head) {    // are they consistent?
        if (first == last) {  // is queue empty or tail falling behind?
          if (next == NULL || head->value == -1) {  // is queue empty?
            // std::cout << "\nqueue is empty";
            return -1;
          }
          compareAndExchange(reinterpret_cast<volatile size_t *>(&tail),
                             reinterpret_cast<size_t>(last),
                             reinterpret_cast<size_t>(next));
        } else {
          int value = next->value;
          if (reinterpret_cast<Node *>(
                  compareAndExchange(reinterpret_cast<volatile size_t *>(&head),
                                     reinterpret_cast<size_t>(first),
                                     reinterpret_cast<size_t>(next))) == first)
            return value;
        }
      }
    }
  }
};

}  // namespace lockfree