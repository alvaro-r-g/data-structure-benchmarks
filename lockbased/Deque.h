#pragma once

#include <memory>
#include <mutex>

namespace lockbased {


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
  std::mutex deque_lock = {};

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
    {
      std::lock_guard<std::mutex> lock(deque_lock);
    
      new_node->next = head->next;
      new_node->prev = head;

      head->next->prev = new_node;
      head->next = new_node;
    }
  }

  // push_right
  void push_back(int const& data) {
    Node *new_node = new Node();
    new_node->data = data;
    {
      std::lock_guard<std::mutex> lock(deque_lock);

      new_node->next = tail;
      new_node->prev = tail->prev;

      tail->prev->next = new_node;
      tail->prev = new_node;
    }
  }

  // pop_left
  int pop_front() {
    int data = -1;
    bool found = false;
    Node *tmp;
    {
      std::lock_guard<std::mutex> lock(deque_lock);
      if (head->next != tail) {
	data = head->next->data;
	tmp = head->next;
	head->next = head->next->next;
	head->next->prev = head;
	found = true;
      }
    }
    if (found) delete tmp;
    return data;
  }

  // pop_right
  int pop_back() {
    int data = -1;
    bool found = false;
    Node *tmp;
    {
      std::lock_guard<std::mutex> lock(deque_lock);
      if (tail->prev != head) {
	data = tail->prev->data;
	tmp = tail->prev;
	tail->prev = tail->prev->prev;
	tail->prev->next = tail;
	found = true;
      }
    }
    if (found) delete tmp;
    return data;
  }

  void print() {
    Node *tmp = head;
    while (tmp)
    {
      std::cout << tmp->data << ",";
      tmp = tmp->next;
    }
    std::cout << std::endl;
  }
};

}  // namespace lockbased
