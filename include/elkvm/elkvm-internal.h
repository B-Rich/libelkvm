#pragma once

#if 1
#include <vector>

#include <elkvm-signal.h>
#include <elfloader.h>
#include <heap.h>
#include <region.h>
#include <region_manager.h>
#include <stack.h>

namespace Elkvm {

  bool operator==(const VM &lhs, const Elkvm::kvm_vm &rhs);
  VM &get_vmi(Elkvm::kvm_vm *vm);

  unsigned get_hypercall_type(VM &, std::shared_ptr<struct kvm_vcpu>);

  unsigned get_hypercall_type(VM &, std::shared_ptr<struct kvm_vcpu>);
  
  //namespace Elkvm
}
#endif
