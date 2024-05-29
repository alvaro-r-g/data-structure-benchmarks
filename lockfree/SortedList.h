//
// Created by kankava on 2020-03-30.
// Lock-free implementation of sorted doubly linked list
//

#pragma once

#include "DoublyLinkedList.h"

namespace lockfree {

class SortedList {
 private:
  doublylinked lst;
 public:
  SortedList() {
    lst.initialise();
  }
  void insert(int item) {
    lst.add(item);
  }
  void remove(int item) {
    lst.deleten(item);
  }
  int count(int item) {
    return lst.count(item);
  }
};

}  // namespace lockfree
