#pragma once

#if 1
#include <vector>

#include <elfloader.h>
#include <heap.h>
#include <region.h>
#include <region_manager.h>
#include <stack.h>

namespace Elkvm {

  bool operator==(const VM &lhs, const VM &rhs);
  unsigned get_hypercall_type(std::shared_ptr<struct kvm_vcpu>);
  
  //namespace Elkvm
}
#endif
