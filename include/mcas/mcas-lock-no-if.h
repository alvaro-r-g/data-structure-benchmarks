#include <mutex>

namespace mcas_lock_no_if {

static std::mutex global_cas_lock;

uint64_t cas(uint64_t* addr0, uint64_t old0, uint64_t new0) {
  bool match = false;
  uint64_t trash = 0;
  const uint64_t i_trash = (uint64_t)&trash;

  const uint64_t i_addr0 = (uint64_t)addr0;
            
  // atomically
  {
  	std::lock_guard<std::mutex> lock(global_cas_lock);
  
    match = (*addr0 == old0);
    *(uint64_t*)((i_addr0 * match) + (i_trash * (1 - match))) = new0;

  }
  return match;
}

uint64_t dcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
            uint64_t* addr1, uint64_t old1, uint64_t new1) {
  bool match = false;
  uint64_t trash = 0;
  const uint64_t i_trash = (uint64_t)&trash;

  const uint64_t i_addr0 = (uint64_t)addr0;
  const uint64_t i_addr1 = (uint64_t)addr1;
            
  // atomically
  {
  	std::lock_guard<std::mutex> lock(global_cas_lock);
  
    match = (*addr0 == old0) & (*addr1 == old1);
    *(uint64_t*)((i_addr0 * match) + (i_trash * (1 - match))) = new0;
    *(uint64_t*)((i_addr1 * match) + (i_trash * (1 - match))) = new1;

  }
  return match;
}

uint64_t tcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
            uint64_t* addr1, uint64_t old1, uint64_t new1,
            uint64_t* addr2, uint64_t old2, uint64_t new2) {
  bool match = false;
  uint64_t trash = 0;
  const uint64_t i_trash = (uint64_t)&trash;

  const uint64_t i_addr0 = (uint64_t)addr0;
  const uint64_t i_addr1 = (uint64_t)addr1;
  const uint64_t i_addr2 = (uint64_t)addr2;
            
  // atomically
  {
  	std::lock_guard<std::mutex> lock(global_cas_lock);
  
    match = (*addr0 == old0) & (*addr1 == old1) & (*addr2 == old2);
    *(uint64_t*)((i_addr0 * match) + (i_trash * (1 - match))) = new0;
    *(uint64_t*)((i_addr1 * match) + (i_trash * (1 - match))) = new1;
    *(uint64_t*)((i_addr2 * match) + (i_trash * (1 - match))) = new2;

  }
  return match;
}

uint64_t qcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
            uint64_t* addr1, uint64_t old1, uint64_t new1,
            uint64_t* addr2, uint64_t old2, uint64_t new2,
            uint64_t* addr3, uint64_t old3, uint64_t new3) {
  bool match = false;
  uint64_t trash = 0;
  const uint64_t i_trash = (uint64_t)&trash;

  const uint64_t i_addr0 = (uint64_t)addr0;
  const uint64_t i_addr1 = (uint64_t)addr1;
  const uint64_t i_addr2 = (uint64_t)addr2;
  const uint64_t i_addr3 = (uint64_t)addr3;
            
  // atomically
  {
  	std::lock_guard<std::mutex> lock(global_cas_lock);
  
    match = (*addr0 == old0) & (*addr1 == old1) & (*addr2 == old2) & (*addr3 == old3);
    *(uint64_t*)((i_addr0 * match) + (i_trash * (1 - match))) = new0;
    *(uint64_t*)((i_addr1 * match) + (i_trash * (1 - match))) = new1;
    *(uint64_t*)((i_addr2 * match) + (i_trash * (1 - match))) = new2;
    *(uint64_t*)((i_addr3 * match) + (i_trash * (1 - match))) = new3;

  }
  return match;
}

}