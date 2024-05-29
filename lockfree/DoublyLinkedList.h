// Original code from Patel et al. 2017 paper
// A hardware implementation of the MCAS synchronization primitive

#pragma once

#include <climits>
#include <cstdint>
#include <cstdlib>

#include "DPointer.h"

namespace lockfree {

class doublylinked {
 public:
  class Node {
   public:
    int value;
    DPointer<doublylinked::Node, sizeof(size_t)> after;
    Node *before;
    Node() {}
    Node(int key) {
      this->value = key;
      this->before = NULL;
      this->after = DPointer<doublylinked::Node, sizeof(size_t)>();
    }
  };
  Node *headdummy, *taildummy;
  bool deleten(int key) {
    Node *pred = headdummy, *curr;
    curr = pred->after.ptr;
    while (curr->value < key) {
      pred = curr;
      curr = curr->after.ptr;
    }
    return deleteNode(curr, true);
  }
  bool deleteNode(Node *thisNode, bool retry) {
    DPointer<doublylinked::Node, sizeof(size_t)> nextref;
    while (true) {
      nextref = thisNode->after;
      if (nextref.mark) return false;
      Node *next = nextref.ptr;
      if (thisNode->after.cas(DPointer<Node, sizeof(size_t)>(next, true), next))
        ;
      break;
      if (!retry) return false;
    }
    getBack(thisNode);
    return true;
  }
  Node *getBack(Node *refNode) {
    Node *prevref = refNode->before;
    Node *currentNode = refNode;
    while (true) {
      prevref = currentNode->before;
      Node *backnode = prevref;
      DPointer<doublylinked::Node, sizeof(size_t)> backAtref = backnode->after;
      Node *backAftNode = backAtref.ptr;
      if (backAtref.mark)
        currentNode = backnode;
      else if (backAftNode == refNode)
        return backnode;
      else {
        Node *maybeback = fixforwarduntil(backnode, refNode);
        if ((maybeback == NULL) && (backnode->after.mark))
          currentNode = backnode;
        else
          return maybeback;
      }
    }
  }

  Node *fixforwarduntil(Node *thisNode, Node *laterNode) {
    Node *nextnode, *worknode = thisNode;
    DPointer<doublylinked::Node, sizeof(size_t)> thisnodeAtref, workNodeAtref,
        laternodeAtref;
    while (true) {
      thisnodeAtref = thisNode->after;
      if (thisnodeAtref.mark) return NULL;
      laternodeAtref = laterNode->after;
      if ((laternodeAtref.ptr != NULL) && (laternodeAtref.mark)) return NULL;
      workNodeAtref = worknode->after;
      if (workNodeAtref.ptr == NULL) return NULL;
      if (!(workNodeAtref.mark)) {
        fixforward(worknode);
        workNodeAtref = worknode->after;
      }
      nextnode = workNodeAtref.ptr;
      if (nextnode == laterNode)
        return worknode;
      else if (nextnode->after == NULL)
        return NULL;
      else
        worknode = nextnode;
    }
  }

  void initialise() {
    Node *a = new Node(0);
    headdummy = a;
    taildummy = a;
    Node *b = new Node(100000000);
    b->before = a;
    a->after = DPointer<doublylinked::Node, sizeof(size_t)>(b, 0);
  }

  bool add(int key) {
    Node *mynode = new Node(key);
    Node *pred = headdummy, *curr;
    curr = pred->after.ptr;
    while (curr->value < key) {
      if (pred == curr) break;
      pred = curr;
      curr = curr->after.ptr;
    }
    if (headdummy == pred)
      return insertafter(headdummy, mynode);
    else
      return insertafter(pred, mynode);
  }

  bool insertafter(Node *previous, Node *mynode) {
    while (true) {
      DPointer<doublylinked::Node, sizeof(size_t)> prevAtref = previous->after;
      if (prevAtref.mark) return false;
      Node *prevafter = fixforward(previous);
      if (insertBetween(mynode, previous, prevafter)) return true;
    }
  }
  bool insertBetween(Node *thisNode, Node *prev, Node *after) {
    thisNode->before = prev;
    thisNode->after = DPointer<doublylinked::Node, sizeof(size_t)>(after, 0);
    if (prev->after.cas(DPointer<Node, sizeof(size_t)>(thisNode, false),
                        after)) {
      reflectforward(thisNode);
      return true;
    }
    return false;
  }

  Node *fixforward(Node *thisNode) {
    DPointer<doublylinked::Node, sizeof(size_t)> thisAtref = thisNode->after;
    Node *laterNode = thisAtref.ptr;
    Node *laterLater;
    while (true) {
      DPointer<doublylinked::Node, sizeof(size_t)> nextref = laterNode->after;
      if (nextref == NULL || !nextref.mark) {
        reflectforward(thisNode);
        return laterNode;
      } else {
        laterLater = nextref.ptr;
        thisNode->after.cas(DPointer<Node, sizeof(size_t)>(laterLater, false),
                            laterNode);
        laterNode = laterLater;
      }
    }
  }

  void reflectforward(Node *previous) {
    DPointer<doublylinked::Node, sizeof(size_t)> prevAtref = previous->after;
    if (prevAtref.mark) return;
    Node *afterNode = prevAtref.ptr;
    Node *afterBeforeref = afterNode->before;
    Node *afterBeforeNode = afterBeforeref;
    if (afterBeforeNode == previous) return;
    DPointer<doublylinked::Node, sizeof(size_t)> afterAtref = afterNode->after;
    if (afterAtref == NULL && !afterAtref.mark) afterNode->before = previous;
  }

  int count(int key) {
    int n = 0;
    Node *pred = headdummy, *curr;
    curr = pred->after.ptr;
    while (curr->value <= key) {
      if (pred == curr) break;
      if (curr->value == key) n++;
      pred = curr;
      curr = curr->after.ptr;
    }
    return n;
  }
};

}  // namespace lockfree