#pragma once

#include <region.h>

/* 64bit Linux puts the Stack at 47bits */
#define LINUX_64_STACK_BASE 0x800000000000
#define ELKVM_STACK_GROW    0x200000

namespace Elkvm {

  class Stack {
    private:
      std::vector<std::shared_ptr<Region>> stack_regions;
      std::shared_ptr<Region> kernel_stack;
      struct kvm_vcpu *vcpu;
      struct kvm_pager *pager;
      int expand();

    public:
      void init(struct kvm_vcpu *v, struct kvm_pager *p);
      int pushq(uint64_t val);
      uint64_t popq();
      bool is_stack_expansion(guestptr_t pfla);
      bool grow(guestptr_t pfla);
      guestptr_t kernel_base();
  };

//namespace Elkvm
}
