// From lockfree-mcas/Deque.h

#pragma once

#include "flock/flock.h"

namespace flock_mcas {

class Deque {
 private:
  struct Node : flck::lock {
    int data;
    flck::atomic<Node *> L;
    flck::atomic<Node *> R;
    Node() = default;
    Node(int data, Node *L, Node *R) : data(data), L(L), R(R) {};
  };

  flck::memory_pool<Node> node_pool;
  
  flck::atomic<Node *> LeftHat;   // head
  flck::lock lh_lock;

  flck::atomic<Node *> RightHat;  // tail
  flck::lock rh_lock;

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
    node_pool.retire(dummy);
  }

  // push_left
  void push_front(int const& data) {
    flck::with_epoch([=] {
      Node *new_node = node_pool.new_obj(data, dummy, nullptr);

      while (true) {
        Node* lh = LeftHat.load();
        Node* lhL = (lh->L).load();
        if (lhL == lh) {
          new_node->R = dummy;
          if (lh_lock.try_lock([=] {
            if (rh_lock.try_lock([=] {
              if (LeftHat.load() != lh)
                return false;

              LeftHat = new_node;
              RightHat = new_node;
              return true;
            }))
              return true;
            else return false;
          }))
            return;
        } else {
          new_node->R = lh;
          if (lh_lock.try_lock([=] {
            if (lh->try_lock([=] {
              if (LeftHat.load() != lh || (lh->L).load() != lhL)
                return false;

              LeftHat = new_node;
              lh->L = new_node;
              return true;
            }))
              return true;
            else return false;
          }))
            return;
        }
      }
    });
  }

  // push_right
  void push_back(int const& data) {
    flck::with_epoch([=] {
      Node *new_node = node_pool.new_obj(data, nullptr, dummy);

      while (true) {
        Node* rh = RightHat.load();
        Node* rhR = (rh->R).load();
        if (rhR == rh) {
          new_node->L = dummy;
          if (rh_lock.try_lock([=] {
            if (lh_lock.try_lock([=] {
              if (RightHat.load() != rh)
                return false;

              RightHat = new_node;
              LeftHat = new_node;
              return true;
            }))
              return true;
            else return false;
          }))
            return;
        } else {
          new_node->L = rh;
          if (rh_lock.try_lock([=] {
            if (rh->try_lock([=] {
              if (RightHat.load() != rh || (rh->R).load() != rhR)
                return false;

              RightHat = new_node;
              rh->R = new_node;
              return true;
            }))
              return true;
            else return false;
          }))
            return;
        }
      }
    });
  }

  // pop_left
  int pop_front() {
    return flck::with_epoch([=] {
      while (true) {
        Node* lh = LeftHat.load();
        Node* lhL = (lh->L).load();
        Node* lhR = (lh->R).load();

        if (lhL == lh) {
          if (LeftHat.load() == lh) return -1;
        } else {
          auto result = lh_lock.try_lock_result([=] {
            return lh->try_lock_result([=] {
              if (LeftHat.load() != lh || (lh->R).load() != lhR || (lh->L).load() != lhL)
                return -1;

              LeftHat = lhR;
              lh->R = lh;
              lh->L = lh;

              int data = lh->data;
              node_pool.retire(lh);
              return data;
            }).value_or(-1);
          });

          if (result.has_value() && result.value() != -1)
            return result.value();
        }
      }
    });
  }

  // pop_right
  int pop_back() {
    return flck::with_epoch([=] {
      while (true) {
        Node* rh = RightHat.load();
        Node* rhL = (rh->L).load();
        Node* rhR = (rh->R).load();

        if (rhR == rh) {
          if (RightHat.load() == rh) return -1;
        } else {
          auto result = rh_lock.try_lock_result([=] {
            return rh->try_lock_result([=] {
              if (RightHat.load() != rh || (rh->L).load() != rhL || (rh->R).load() != rhR)
                return -1;

              RightHat = rhL;
              rh->L = rh;
              rh->R = rh;

              int data = rh->data;
              node_pool.retire(rh);
              return data;
            }).value_or(-1);
          });

          if (result.has_value() && result.value() != -1)
            return result.value();
        }
      }
    });
  }
};

}  // namespace flock_mcas
