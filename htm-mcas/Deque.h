// Adapted from lockfree-mcas/Deque.h

#pragma once

#include "tm.h"

namespace htm_mcas {

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

    TM_BEGIN(0);

    Node* lh = LeftHat;
    Node* lhL = lh->L;
    if (lhL == lh) {
      new_node->R = dummy;
      LeftHat = new_node;
      RightHat = new_node;
    } else {
      new_node->R = lh;
      LeftHat = new_node;
      lh->L = new_node;
    }

    TM_END(0);
  }

  // push_right
  void push_back(int const& data) {
    Node *new_node = new Node();
    new_node->R = dummy;
    new_node->data = data;

    TM_BEGIN(1);

    Node* rh = RightHat;
    Node* rhR = rh->R;
    if (rhR == rh) {
      new_node->L = dummy;
      RightHat = new_node;
      LeftHat = new_node;
    } else {
      new_node->L = rh;
      RightHat = new_node;
      rh->R =  new_node;
    }

    TM_END(1);
  }

  // pop_left
  int pop_front() {
    int result;

    TM_BEGIN(2);

    Node* lh = LeftHat;
    Node* lhL = lh->L;
    Node* lhR = lh->R;

    if (lhL == lh) {
      result = -1;
    } else {
      LeftHat = lhR;
      lh->R = lh;
      lh->L = lh;
      result = lh->data;
    }

    TM_END(2);

    return result;
  }

  // pop_right
  int pop_back() {
    int result;

    TM_BEGIN(3);

    Node* rh = RightHat;
    Node* rhL = rh->L;
    Node* rhR = rh->R;

    if (rhR == rh) {
      result = -1;
    } else {
      RightHat = rhL;
      rh->L = rh;
      rh->R = rh;
      result = rh->data;
    }

    TM_END(3);

    return result;
  }
};

}  // namespace htm_mcas
