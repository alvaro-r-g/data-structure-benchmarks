#pragma once

#include <sys/time.h>
#include <cassert>
#include <cstdint>
#include <memory>
#include "mcas.h"

namespace lockfree_mcas {
namespace ArraySwap {

const int NUM_SUB_ITEMS = 2;
const int NUM_ROWS = 100000;

struct Element {
  int32_t value_[NUM_SUB_ITEMS];
};

struct Datum {
  // pointer to the hashmap
  Element* elements_;
};

struct sps {
  Datum* array;
  int num_rows_;
  int num_sub_items_;
};

sps* S;

void datum_init(sps* s) {
  for (int i = 0; i < NUM_ROWS; i++) {
    s->array[i].elements_ = (Element*)malloc(sizeof(Element));

    for (int j = 0; j < NUM_SUB_ITEMS; j++)
      s->array[i].elements_->value_[j] = i + j;
  }
}

void datum_free(sps* s) {
  for (int i = 0; i < NUM_ROWS; i++) {
    free(s->array[i].elements_);
  }
}

void initialize() {
  S = (sps*)malloc(sizeof(sps));
  S->num_rows_ = NUM_ROWS;
  S->num_sub_items_ = NUM_SUB_ITEMS;
  S->array = (Datum*)malloc(sizeof(Datum) * NUM_ROWS);
  datum_init(S);

  // fprintf(stderr, "Created array at %p\n", (void*)S);
}

bool swap(unsigned int index_a, unsigned int index_b) {
  // check if index is out of array
  assert(index_a < NUM_ROWS && index_b < NUM_ROWS);

  // exit if swapping the same index
  if (index_a == index_b) return true;

  // enforce index_a < index_b not needed in lockfree
  // if (index_a > index_b) {
  //   unsigned int index_tmp = index_a;
  //   index_a = index_b;
  //   index_b = index_tmp;
  // }

  while (true) {
    Element* addr_a = S->array[index_a].elements_;
    Element* addr_b = S->array[index_b].elements_;

    if (dcas(reinterpret_cast<uint64_t*>(&S->array[index_a].elements_),
         reinterpret_cast<uint64_t>(addr_a), reinterpret_cast<uint64_t>(addr_b),
         reinterpret_cast<uint64_t*>(&S->array[index_b].elements_),
         reinterpret_cast<uint64_t>(addr_b),
         reinterpret_cast<uint64_t>(addr_a))) return true;
  }
}

}  // namespace ArraySwap
}  // namespace lockfree_mcas
