// Adapted from lockbased/SortedList.h

#pragma once

#include "tm.h"

namespace htm_lock {

class SortedList {
 private:
  struct Node {
    int data;
    Node *next;
    Node *prev;
    Node() = default;
  };

  Node *head;
  Node *tail;

 public:
  SortedList() {
    head = new Node();
    tail = new Node();
    head->next = tail;
    tail->prev = head;
  }

  virtual ~SortedList() {
      auto curr = head;

      while (curr != tail) {
        Node *tmp = curr;
        curr = curr->next;
        delete tmp;
      }
      delete tail;
  }

  void insert(int const &data) {
    Node *new_node = new Node();
    new_node->data = data;
    auto parent = head;

    TM_BEGIN(0);
    auto curr = head->next;

    while (curr != tail && curr->data < data) {
      parent = curr;
      curr = curr->next;
    }

    new_node->next = curr;
    new_node->prev = parent;

    parent->next = new_node;
    curr->prev = new_node;
    TM_END(0);

    return;
  }

  void remove(int const& data) {
    Node *tmp;

    TM_BEGIN(1);
    
    Node *curr = head->next;
    while (curr != tail && curr->data < data) {
      curr = curr->next;
    }

    if (curr == tail || curr->data != data) {
      TM_END(1);
      return;
    }

    tmp = curr;
    curr->next->prev = curr->prev;
    curr->prev->next = curr->next;
    TM_END(1);

    delete tmp;

    return;
  }

  int count(int val) {
    int n_val = 0;

    TM_BEGIN(2);
    auto curr = head->next;

    while (curr != tail) {
      if (curr->data == val) n_val++;
      curr = curr->next;
    }

    TM_END(2);
    return n_val;
  }

  // for testing, not safe
  void print_all() {
    auto curr = head->next;

    while (curr != tail) {
      std::cout << curr->data << " ";
      curr = curr->next;
    }
    std::cout << std::endl;
  }
};

}  // namespace htm_lock
