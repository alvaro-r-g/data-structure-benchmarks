//
// Lock-free Deque implementation using Snark Algorithm
// as Described in Doherty et al. 2004 DCAS is not a silver bullet for
// nonblocking algorithms.
//

#pragma once

#include "mcas.h"

namespace lockfree_mcas {

class Deque {
 private:
  struct Node {
    int data;
    Node *L;
    Node *R;
    Node() = default;
  };

  Node *LeftHat;   // head
  Node *RightHat;  // tail
  Node *dummy;

 public:
  Deque() {
    dummy = new Node();
    dummy->L = dummy;
    dummy->R = dummy;
    LeftHat = dummy;
    RightHat = dummy;
  }

  ~Deque() {
    while (true) {
      int val = pop_back();
      if (val == -1) break;
    }
    delete dummy;
  }

  // push_left
  void push_front(int const& data) {
    Node *new_node = new Node();
    new_node->L = dummy;
    new_node->data = data;

    while (true) {
      Node* lh = LeftHat;
      Node* lhL = lh->L;
      if (lhL == lh) {
        new_node->R = dummy;
        Node* rh = RightHat;
        if (dcas(reinterpret_cast<uint64_t *>(&LeftHat), reinterpret_cast<uint64_t>(lh), reinterpret_cast<uint64_t>(new_node),
                 reinterpret_cast<uint64_t *>(&RightHat), reinterpret_cast<uint64_t>(rh), reinterpret_cast<uint64_t>(new_node))) return;
      } else {
        new_node->R = lh;
        if (dcas(reinterpret_cast<uint64_t *>(&LeftHat), reinterpret_cast<uint64_t>(lh), reinterpret_cast<uint64_t>(new_node),
                 reinterpret_cast<uint64_t *>(&lh->L), reinterpret_cast<uint64_t>(lhL), reinterpret_cast<uint64_t>(new_node))) return;
      }
    }
  }

  // push_right
  void push_back(int const& data) {
    Node *new_node = new Node();
    new_node->R = dummy;
    new_node->data = data;

    while (true) {
      Node* rh = RightHat;
      Node* rhR = rh->R;
      if (rhR == rh) {
        new_node->L = dummy;
        Node* lh = LeftHat;
        if (dcas(reinterpret_cast<uint64_t *>(&RightHat), reinterpret_cast<uint64_t>(rh), reinterpret_cast<uint64_t>(new_node),
                 reinterpret_cast<uint64_t *>(&LeftHat), reinterpret_cast<uint64_t>(lh), reinterpret_cast<uint64_t>(new_node))) return;
      } else {
        new_node->L = rh;
        if (dcas(reinterpret_cast<uint64_t *>(&RightHat), reinterpret_cast<uint64_t>(rh), reinterpret_cast<uint64_t>(new_node),
                 reinterpret_cast<uint64_t *>(&rh->R), reinterpret_cast<uint64_t>(rhR), reinterpret_cast<uint64_t>(new_node))) return;
      }
    }
  }

  // pop_left
  int pop_front() {
    while (true) {
      Node* lh = LeftHat;
      Node* lhL = lh->L;
      Node* lhR = lh->R;

      if (lhL == lh) {
        if (LeftHat == lh) return -1;
      } else {
        if (tcas(reinterpret_cast<uint64_t *>(&LeftHat), reinterpret_cast<uint64_t>(lh), reinterpret_cast<uint64_t>(lhR),
                 reinterpret_cast<uint64_t *>(&lh->R), reinterpret_cast<uint64_t>(lhR), reinterpret_cast<uint64_t>(lh),
                 reinterpret_cast<uint64_t *>(&lh->L), reinterpret_cast<uint64_t>(lhL), reinterpret_cast<uint64_t>(lh))) {
          int result = lh->data;
          return result;
        }
      }
    }
  }

  // pop_right
  int pop_back() {
    while (true) {
      Node* rh = RightHat;
      Node* rhL = rh->L;
      Node* rhR = rh->R;

      if (rhR == rh) {
        if (RightHat == rh) return -1;
      } else {
        if (tcas(reinterpret_cast<uint64_t *>(&RightHat), reinterpret_cast<uint64_t>(rh), reinterpret_cast<uint64_t>(rhL),
                 reinterpret_cast<uint64_t *>(&rh->L), reinterpret_cast<uint64_t>(rhL), reinterpret_cast<uint64_t>(rh),
                 reinterpret_cast<uint64_t *>(&rh->R), reinterpret_cast<uint64_t>(rhR), reinterpret_cast<uint64_t>(rh))) {
          int result = rh->data;
          return result;
        }
      }
    }
  }
};

}  // namespace lockfree_mcas
