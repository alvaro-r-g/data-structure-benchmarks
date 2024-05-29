#pragma once

#include <climits>
#include <iostream>
#include <memory>
#include <mutex>
#include "mcas.h"

namespace lockfree_mcas {

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

  bool insert_before(Node *next, Node *node) {
    if (next == head) {
      return insert_after(next, node);
    }

    if ((next->prev == nullptr) ||
        ((next->next == nullptr) && next != tail)) {
      return false;
    }

    Node *prev = next->prev;

    node->next = next;
    node->prev = prev;
    if (qcas((uint64_t *)&node->prev->next, (uint64_t)node->next, (uint64_t)node,
             (uint64_t *)&next->prev, (uint64_t)node->prev, (uint64_t)node,
             (uint64_t *)&node->next, (uint64_t)node->next, (uint64_t)next,
             (uint64_t *)&node->prev, (uint64_t)node->prev, (uint64_t)prev
             )
        ) {
      return true;
    } else {
      return false;
    }
  }

  bool insert_after(Node *prev, Node *node) {
    if (prev == tail) {
      return insert_before(prev, node);
    }

    if ((prev->next == nullptr) ||
        ((prev->prev == nullptr) && prev != head)) {
      return false;
    }

    Node *next = prev->next;

    node->prev = prev;
    node->next = next;
    if (qcas((uint64_t *)&prev->next, (uint64_t)node->next, (uint64_t)node,
             (uint64_t *)&node->next->prev, (uint64_t)node->prev, (uint64_t)node,
             (uint64_t *)&node->next, (uint64_t)node->next, (uint64_t)next,
             (uint64_t *)&node->prev, (uint64_t)node->prev, (uint64_t)prev
             )
        ) {
      return true;
    } else {
      return false;
    }
  }

  bool delete_node(Node *node) {
      if (node == head || node == tail) {
        return true;
      }
      Node *prev = node->prev;
      Node *next = node->next;
      Node *tmp = nullptr;

      if ((prev == nullptr) && (next == nullptr)) return false; // was already deleted

      if(qcas((uint64_t*)&prev->next, (uint64_t)node, (uint64_t)next,
              (uint64_t*)&next->prev, (uint64_t)node, (uint64_t)prev,
              (uint64_t*)&node->next, (uint64_t)next, (uint64_t)tmp,
              (uint64_t*)&node->prev, (uint64_t)prev, (uint64_t)tmp)
          ) {
        return true;
      } else {
        return false;
      }
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

    while (true) {
    retry:
      new_node->next = nullptr;
      new_node->prev = nullptr;

      Node *curr = head->next;

      while (curr != nullptr && curr != tail && curr->data < data) {
        curr = curr->next;
      }

      if (curr == nullptr) goto retry;
      if ((curr->next == nullptr) && (curr->prev == nullptr)) goto retry; //node was deleted

      if (insert_before(curr, new_node)) return;

    }
  }

  void remove(int data) {
    while (true) {
      retry:
      Node *curr = head->next;

      while (curr != nullptr && curr != tail && curr->data < data) {
        curr = curr->next;
      }

      if (curr == nullptr) goto retry;
      if (curr == tail) return;
      if (curr->data != data) return;
      if ((curr->next == nullptr) || (curr->prev == nullptr)) goto retry;

      if (delete_node(curr)) return;
    }
  }

  int count(int val) {
    while(true) {
      int n_val = 0;
      auto curr = head->next;
      while (curr != tail && curr != nullptr) {
        if (curr->data == val) n_val++;
        curr = curr->next;
      }
      if (curr != nullptr) return n_val;
    }
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

}  // namespace lockfree_mcas
