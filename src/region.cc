#include <iostream>

#include <inttypes.h>
#include <stdio.h>

#include <elkvm.h>
#include "region.h"
#include "region-c.h"

struct elkvm_memory_region *elkvm_region_create(struct kvm_vm *vm, uint64_t req_size) {
  Elkvm::Region &r = Elkvm::rm.allocate_region(req_size);
	return r.c_region();
}

int elkvm_region_free(struct kvm_vm *vm, struct elkvm_memory_region *region) {
  Elkvm::rm.free_region(region->host_base_p, region->region_size);
  memset(region->host_base_p, 0, region->region_size);
  return 0;
}

int elkvm_init_region_manager(struct kvm_pager *const pager) {
  Elkvm::rm.set_pager(pager);
  Elkvm::rm.add_system_chunk();
  return 0;
}

namespace Elkvm {

  std::ostream &print(std::ostream &stream, const Region &r) {
    if(free) {
      stream << "FREE ";
    }
    stream << "REGION[" << &r << "] guest address: " << r.guest_address()
      << " host_p: " << r.base_address() << " size: " << r.size() << std::endl;
    return stream;
  }

  bool same_region(const void *p1, const void *p2) {
    const Region &r = Elkvm::rm.find_region(p1);
    return r.contains_address(p2);
  }

  struct elkvm_memory_region *Region::c_region() const {
    struct elkvm_memory_region *r = new(struct elkvm_memory_region);
    r->host_base_p = host_p;
    r->guest_virtual = addr;
    r->region_size = rsize;
    r->used = !free;
    r->grows_downward = 0;
    r->rc = r->lc = NULL;
    return r;
  }

  void *Region::last_valid_address() const {
    return host_p + rsize;
  }

  std::ostream &Region::print(std::ostream &stream) {
    if(free) {
      stream << "FREE ";
    }
    stream << "REGION[" << this << "] guest address: " << addr << " host_p: "
      << host_p << " size: " << rsize << std::endl;
    return stream;
  }

  Region Region::slice_begin(const size_t size) {
    assert(free);
    assert(rsize > size);

    Region r(host_p, size);
    host_p += size;
    rsize  -= size;
    return r;
  }

//namespace Elkvm
}
