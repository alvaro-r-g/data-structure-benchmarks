// Adapted from lockbased/BinarySearchTree.h

#pragma once

#include <climits>

#include "tm.h"

namespace htm_lock {

class BinarySearchTree {
 private:
  struct Node {
    int value;
    Node* left;
    Node* right;
    Node() = default;
  };

  Node* root;
  int sentinel_min = INT_MIN;
  int sentinel_max = INT_MAX;

  typedef enum {
    LEFT,
    RIGHT,
  } node_type;

 public:
  BinarySearchTree() : root(nullptr) {};

  void insert(const int value) {
    Node *new_node = new Node();
    new_node->value = value;

    TM_BEGIN(0);

    if (!root) {
      root = new_node;
      TM_END(0);
      return;
    }

    Node *curr = root;
    Node *prev = nullptr;
    node_type type = LEFT;

    while (curr) {
      prev = curr;
      if (value < curr->value) {
        curr = curr->left;
        type = LEFT;
      } else {
        curr = curr->right;
        type = RIGHT;
      }
    }

    if (type == LEFT) {
      prev->left = new_node;
    } else {
      prev->right = new_node;
    }

    TM_END(0);
  }

  void remove(int value) {
    Node *prev = nullptr;
    node_type type = LEFT;

    TM_BEGIN(1);

    Node *curr = root;
    while (curr) {
      if (curr->value == value) {
        if (!curr->left && !curr->right) { // node to be removed has no children’s
          if (curr != root && prev) { // delete leaf node
            if (type == LEFT)
            prev->left = nullptr;
            else
            prev->right = nullptr;
          } else
            root = nullptr; // deleted node is root
        } else if (curr->left && curr->right) { // node to be removed has two children’s
          curr->value = get_min_UNSAFE(curr->right); // find minimum value from right subtree
          value = curr->value;
          prev = curr;
          curr = curr->right; // continue from right subtree delete min node
          type = RIGHT;
          continue;
        } else { // node to be removed has one children
          if (curr == root) { // root with one child
            root = root->left ? root->left : root->right;
          } else { // subtree with one child
            if (type == LEFT)
            prev->left = curr->left ? curr->left : curr->right;
            else
            prev->right = curr->left ? curr->left : curr->right;
          }
        }
      }
      prev = curr;
      if (value < curr->value) {
        curr = curr->left;
        type = LEFT;
      } else {
        curr = curr->right;
        type = RIGHT;
      }
    }

    TM_END(1);
  }

  int get_min() {
    TM_BEGIN(2);

    int min = get_min_UNSAFE(root);

    TM_END(2);

    return min;
  }

  int get_min(Node *_root) {
    TM_BEGIN(3);

    int min = get_min_UNSAFE(_root);

    TM_END(3);
    
    return min;
  }

 private:

  int get_min_UNSAFE(Node *_root) {
    if (!_root) return sentinel_max;
    
    auto curr = _root;

    while (curr->left) {
      curr = curr->left;
    }

    return curr->value;
  }
  
};

}  // namespace htm_lock
