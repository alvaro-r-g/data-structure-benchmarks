// Adapted from lockbased/Deque.h

#pragma once

#include "flock/flock.h"

namespace lockfree_flock {

class Deque {
 private:
  struct Node {
    int data;
    flck::atomic<Node *> prev;
    flck::atomic<Node *> next;
    Node() = default;
    Node(int data) : data(data), prev(nullptr), next(nullptr) {};
  };

  Node *head;
  Node *tail;

  flck::lock deque_lock;
  flck::memory_pool<Node> node_pool;

 public:
  Deque() {
    head = node_pool.new_obj();
    tail = node_pool.new_obj();

    head->next = tail;
    tail->prev = head;
  }

  virtual ~Deque() {
    Node *curr = head;
    while (curr != tail) {
      Node *tmp = curr;
      curr = (curr->next).load();
      node_pool.retire(tmp);
    }
    node_pool.retire(tail);
  }

  // push_left
  void push_front(int const& data) {
    flck::with_epoch([=] {
      deque_lock.with_lock([=] {
        Node *new_node = node_pool.new_obj(data);
        Node *hn = (head->next).load();

        new_node->next = hn;
        new_node->prev = head;

        hn->prev = new_node;
        head->next = new_node;
        return true;
      });
    });
  }

  // push_right
  void push_back(int const& data) {
    flck::with_epoch([=] {
      deque_lock.with_lock([=] {
        Node *new_node = node_pool.new_obj(data);
        Node *tp = (tail->prev).load();

        new_node->next = tail;
        new_node->prev = tp;

        tp->next = new_node;
        tail->prev = new_node;
        return true;
      });
    });
  }

  // pop_left
  int pop_front() {
    return flck::with_epoch([=] {
      return deque_lock.with_lock([=] {
        int data = -1;
        Node *hn = (head->next).load();

        if (hn != tail) {
          data = hn->data;
          Node *hnn = (hn->next).load();

          hnn->prev = head;
          head->next = hnn;
          node_pool.retire(hn);
        }

        return data;
      });
    });
  }

  // pop_right
  int pop_back() {
    return flck::with_epoch([=] {
      return deque_lock.with_lock([=] {
        int data = -1;
        Node *tp = (tail->prev).load();

        if (tp != head) {
          data = tp->data;
          Node *tpp = (tp->prev).load();

          tpp->next = tail;
          tail->prev = tpp;
          node_pool.retire(tp);
        }

        return data;
      });
    });
  }

  void print() {
    Node *tmp = head;
    while (tmp)
    {
      std::cout << tmp->data << ",";
      tmp = (tmp->next).load();
    }
    std::cout << std::endl;
  }
};

}  // namespace lockfree_flock
