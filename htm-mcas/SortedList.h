// Adapted from lockfree-mcas/SortedList.h

#pragma once

#include <climits>
#include <iostream>
#include <memory>
#include "tm.h"

namespace htm_mcas {

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

  void insert_before(Node *next, Node *node) {
    if (next == head) {
      insert_after(next, node);
    }

    Node *prev = next->prev;

    node->next = next;
    node->prev = prev;

    node->prev->next = node;
    next->prev = node;
    node->next = next;
    node->prev = prev;
  }

  void insert_after(Node *prev, Node *node) {
    if (prev == tail) {
      insert_before(prev, node);
    }

    Node *next = prev->next;

    node->prev = prev;
    node->next = next;

    prev->next = node;
    node->next->prev = node;
    node->next = next;
    node->prev = prev;
  }

  void delete_node(Node *node) {
    if (node == head || node == tail) {
      return;
    }

    Node *prev = node->prev;
    Node *next = node->next;

    prev->next = next;
    next->prev = prev;
  }

 public:
  SortedList() {
    head = new Node();
    tail = new Node();
    head->next = tail;
    tail->prev = head;
  }

  void insert(int data) {
    Node *new_node = new Node();
    new_node->data = data;
    new_node->next = nullptr;
    new_node->prev = nullptr;

    TM_BEGIN(0);

    Node *curr = head->next;

    while (curr != nullptr && curr != tail && curr->data < data) {
      curr = curr->next;
    }

    insert_before(curr, new_node);

    TM_END(0);
  }

  void remove(int data) {
    TM_BEGIN(1);

    Node *curr = head->next;

    while (curr != nullptr && curr != tail && curr->data < data) {
      curr = curr->next;
    }

    if (curr == tail || curr->data != data) {
      TM_END(1);
      return;
    }

    delete_node(curr);

    TM_END(1);
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

  void print_all() {
    auto curr = head->next;

    while (curr != tail) {
      std::cout << curr->data << " ";
      curr = curr->next;
    }
    std::cout << std::endl;
  }
};

}  // namespace htm_mcas
