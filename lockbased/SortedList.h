#pragma once

#include <memory>
#include <mutex>

namespace lockbased {


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
  std::mutex list_lock = {};

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

  void insert(int const& data) {
    Node *new_node = new Node();
    new_node->data = data;
    {
      std::lock_guard<std::mutex> lock(list_lock);

      auto parent = head;
      auto curr = head->next;

      while (curr != tail && curr->data < data) {
	parent = curr;
	curr = curr->next;
      }

      new_node->next = curr;
      new_node->prev = parent;
    
      parent->next = new_node;
      curr->prev = new_node;
    }
    return;
  }

  void remove(int const& data) {
    Node *tmp;
    {
      std::lock_guard<std::mutex> lock(list_lock);

      Node *curr = head->next;
      while (curr != tail && curr->data < data) {
	curr = curr->next;
      }

      if (curr == tail) return;
      if (curr->data != data) return;

      tmp = curr;
      curr->next->prev = curr->prev;
      curr->prev->next = curr->next;
    }
    delete tmp;

    return;
  }

  int count(int val) {
    std::lock_guard<std::mutex> lock(list_lock);
    
    int n_val = 0;
    auto curr = head->next;
    n_val = 0;

    while (curr != tail) {
      if (curr->data == val) n_val++;
      curr = curr->next;
    }

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

}  // namespace lockbased
