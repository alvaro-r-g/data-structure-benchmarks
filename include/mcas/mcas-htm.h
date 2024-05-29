#include "tm.h"

namespace mcas_htm {

uint64_t cas(uint64_t* addr0, uint64_t old0, uint64_t new0) {
  // atomically
  if (!TM_BEGIN_NO_CONFLICT(0)) {
    return false;
  }

  if (*addr0 == old0) {
    *addr0 = new0;

    TM_END(0);
    return true;
  }

  TM_END(0);
  return false;
}

uint64_t dcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
              uint64_t* addr1, uint64_t old1, uint64_t new1) {
  // atomically
  if (!TM_BEGIN_NO_CONFLICT(1)) {
    return false;
  }

  if ((*addr0 == old0) && (*addr1 == old1)) {
    *addr0 = new0;
    *addr1 = new1;

    TM_END(1);
    return true;
  }

  TM_END(1);
  return false;
}

uint64_t tcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
              uint64_t* addr1, uint64_t old1, uint64_t new1,
              uint64_t* addr2, uint64_t old2, uint64_t new2) {
  // atomically
  if (!TM_BEGIN_NO_CONFLICT(2)) {
    return false;
  }

  if ((*addr0 == old0) && (*addr1 == old1) &&
      (*addr2 == old2)) {
    *addr0 = new0;
    *addr1 = new1;
    *addr2 = new2;

    TM_END(2);
    return true;
  }

  TM_END(2);
  return false;
}

uint64_t qcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
              uint64_t* addr1, uint64_t old1, uint64_t new1,
              uint64_t* addr2, uint64_t old2, uint64_t new2,
              uint64_t* addr3, uint64_t old3, uint64_t new3) {
  // atomically
  if (!TM_BEGIN_NO_CONFLICT(3)) {
    return false;
  }

  if ((*addr0 == old0) && (*addr1 == old1) &&
      (*addr2 == old2) && (*addr3 == old3)) {
    *addr0 = new0;
    *addr1 = new1;
    *addr2 = new2;
    *addr3 = new3;

    TM_END(3);
    return true;
  }

  TM_END(3);
  return false;
}

}