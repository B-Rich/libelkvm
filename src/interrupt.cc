#include <memory>

#include <elkvm/elkvm-log.h>
#include <elkvm/interrupt.h>
#include <elkvm/vcpu.h>

namespace Elkvm {

int VM::handle_interrupt(const std::shared_ptr<VCPU>& vcpu) {
  uint64_t interrupt_vector = vcpu->pop();

  if(debug_mode()) {
    DBG() << " INTERRUPT with vector " << std::hex << "0x" << interrupt_vector
      << " detected";
    vcpu->get_sregs();
    print(std::cerr, *vcpu);
    print_stack(std::cerr, *this, *vcpu);

    guestptr_t stack_p = vcpu->get_reg(Elkvm::Reg_t::rsp);
    guestptr_t *host_p = static_cast<guestptr_t *>(
        get_region_manager()->get_pager().get_host_p(stack_p));
    assert(host_p != nullptr);
    guestptr_t instr = *(host_p + 1);
    print_code(std::cerr, *this, instr);
  }

  uint64_t err_code = vcpu->pop();
  switch(interrupt_vector) {
    case Interrupt::Vector::debug_trap:
      return Elkvm::Interrupt::handle_debug_trap(vcpu, err_code);
    case Interrupt::Vector::stack_segment_fault:
      return Elkvm::Interrupt::handle_stack_segment_fault(err_code);
    case Interrupt::Vector::general_protection_fault:
      return Elkvm::Interrupt::handle_general_protection_fault(err_code);
    case Interrupt::Vector::page_fault:
      return Elkvm::Interrupt::handle_page_fault(*this, vcpu, err_code);
  }

  return Interrupt::failure;
}

namespace Interrupt {

int handle_stack_segment_fault(uint64_t code) {
  ERROR() << "STACK SEGMENT FAULT";
  ERROR() << "Error Code: " << code;
  return failure;
}

int handle_general_protection_fault(uint64_t code) {
  ERROR() << "GENERAL PROTECTION FAULT";
  ERROR() << "Error Code:" << code;
  return failure;
}


int handle_debug_trap(const std::shared_ptr<VCPU>& vcpu, uint64_t code) {
  // code is RIP in this case
  ERROR() << "Debug trap @ RIP " << (void*)code;
  // push RIP back and IRET from handler
  vcpu->push(code);
  return success;
}

int handle_page_fault(VM &vm,
    const std::shared_ptr<VCPU>& vcpu,
    uint64_t code) {
  int err = vcpu->get_sregs();
  assert(err == 0 && "error getting vcpu sregs");

  CURRENT_ABI::paramtype pfla = vcpu->get_reg(Elkvm::Reg_t::cr2);
  DBG() << "Page fault @ " << (void*)pfla;
  handle_segfault(pfla);
  if(vcpu->handle_stack_expansion(code, vm.debug_mode())) {
    return success;
  }

  void *hp = vm.get_region_manager()->get_pager().get_host_p(pfla);
  Elkvm::dump_page_fault_info(pfla, code, hp);
  if(hp) {
    vm.get_region_manager()->get_pager().dump_page_tables();
  }

  return failure;
}

int handle_segfault(guestptr_t pfla) {
  if(pfla <= 0x1000) {
    ERROR() << "\n\nABORT: SEGMENTATION FAULT at 0x" << std::hex << pfla
      << std::endl << std::endl;
    exit(EXIT_FAILURE);
  }
  return failure;
}

//namespace Interrupt
}

//namespace Elkvm
}