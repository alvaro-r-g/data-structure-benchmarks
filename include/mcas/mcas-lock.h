#include <mutex>
#include <iostream>

namespace mcas_lock {

static std::mutex global_cas_lock;

uint64_t cas(uint64_t* addr0, uint64_t old0, uint64_t new0) {
  // atomically
  std::lock_guard<std::mutex> lock(global_cas_lock);
  {
    if (*addr0 == old0) {
      *addr0 = new0;
      return true;
    }
    return false;
  }
}

uint64_t dcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
              uint64_t* addr1, uint64_t old1, uint64_t new1) {
  // atomically
  std::lock_guard<std::mutex> lock(global_cas_lock);
  {
    if ((*addr0 == old0) && (*addr1 == old1)) {
      *addr0 = new0;
      *addr1 = new1;
      return true;
    }
    return false;
  }
}

uint64_t tcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
              uint64_t* addr1, uint64_t old1, uint64_t new1,
              uint64_t* addr2, uint64_t old2, uint64_t new2) {
  // atomically
  std::lock_guard<std::mutex> lock(global_cas_lock);
  {
    if ((*addr0 == old0) && (*addr1 == old1) &&
        (*addr2 == old2)) {
      *addr0 = new0;
      *addr1 = new1;
      *addr2 = new2;
      return true;
    }
    return false;
  }
}

uint64_t qcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
              uint64_t* addr1, uint64_t old1, uint64_t new1,
              uint64_t* addr2, uint64_t old2, uint64_t new2,
              uint64_t* addr3, uint64_t old3, uint64_t new3) {
  // atomically
  std::lock_guard<std::mutex> lock(global_cas_lock);
  {
    if ((*addr0 == old0) && (*addr1 == old1) &&
        (*addr2 == old2) && (*addr3 == old3)) {
      *addr0 = new0;
      *addr1 = new1;
      *addr2 = new2;
      *addr3 = new3;
      return true;
    }
    return false;
  }
}

}