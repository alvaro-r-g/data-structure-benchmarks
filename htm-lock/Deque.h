// Adapted from lockbased/Deque.h

#pragma once

#include "tm.h"

namespace htm_lock {

class Deque {
 private:
  struct Node {
    int data;
    Node *prev;
    Node *next;
    Node() = default;
  };

  Node *head;
  Node *tail;

 public:
  Deque() {
    head = new Node();
    tail = new Node();

    head->next = tail;
    tail->prev = head;
  }

  virtual ~Deque() {
    Node *curr = head;
    while (curr != tail) {
      Node *tmp = curr;
      curr = curr->next;
      delete tmp;
    }
    delete tail;
  }

  // push_left
  void push_front(int const& data) {
    Node *new_node = new Node();
    new_node->data = data;

    TM_BEGIN(0);

    new_node->next = head->next;
    new_node->prev = head;

    head->next->prev = new_node;
    head->next = new_node;

    TM_END(0);
  }

  // push_right
  void push_back(int const& data) {
    Node *new_node = new Node();
    new_node->data = data;

    TM_BEGIN(1);

    new_node->next = tail;
    new_node->prev = tail->prev;

    tail->prev->next = new_node;
    tail->prev = new_node;

    TM_END(1);
  }

  // pop_left
  int pop_front() {
    int data = -1;
    bool found = false;
    Node *tmp;

    TM_BEGIN(2);

    if (head->next != tail) {
      data = head->next->data;
      tmp = head->next;
      head->next = head->next->next;
      head->next->prev = head;
      found = true;
    }

    TM_END(2);

    if (found) delete tmp;
    return data;
  }

  // pop_right
  int pop_back() {
    int data = -1;
    bool found = false;
    Node *tmp;
    
    TM_BEGIN(3);

    if (tail->prev != head) {
      data = tail->prev->data;
      tmp = tail->prev;
      tail->prev = tail->prev->prev;
      tail->prev->next = tail;
      found = true;
    }

    TM_END(3);

    if (found) delete tmp;
    return data;
  }
};

}  // namespace htm_lock
