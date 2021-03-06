//
// libelkvm - A library that allows execution of an ELF binary inside a virtual
// machine without a full-scale operating system
// Copyright (C) 2013-2015 Florian Pester <fpester@os.inf.tu-dresden.de>, Björn
// Döbel <doebel@os.inf.tu-dresden.de>,   economic rights: Technische Universitaet
// Dresden (Germany)
//
// This file is part of libelkvm.
//
// libelkvm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// libelkvm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libelkvm.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cstring>

#include <elkvm/elkvm.h>
#include <elkvm/elkvm-internal.h>
#include <elkvm/stack.h>
#include <elkvm/tss.h>
#include <elkvm/vcpu.h>

int elkvm_tss_setup64(std::shared_ptr<Elkvm::VCPU> vcpu, Elkvm::RegionManager &rm, std::shared_ptr<Elkvm::Region> r) {
  guestptr_t guest_virtual =
    rm.get_pager().map_kernel_page(
			r->base_address(), 0);
  assert(guest_virtual != 0x0 && "could not map tss");

  r->set_guest_addr(guest_virtual);
	struct elkvm_tss64 *tss = (struct elkvm_tss64 *)r->base_address();
	memset(tss, 0, sizeof(struct elkvm_tss64));

	tss->ist1 = vcpu->kernel_stack_base() + ELKVM_PAGESIZE;
	tss->rsp0 = 0xFFFFFFFFFFFFFFFF;
	tss->rsp2 = 0x00007FFFFFFFFFFF;
	return 0;
}

int elkvm_tss_setup32(struct elkvm_tss32 *tss,
		int kernel_data_selector) {
	memset(tss, 0, sizeof(struct elkvm_tss32));

	tss->ss0 = kernel_data_selector;
	tss->esp0 = 0x1042;
	//tss->iopb = sizeof(struct elkvm_tss);

//	/*
//	It's a quirk in the Intel implementation of hardware virtualization
//	extensions. You cannot enter guest mode in vmx with the guest cr0.pe
//	cleared (i.e. real mode), so kvm enters the guest in vm86 mode which
//	is fairly similar and tries to massage things so it looks to the guest
//	as if it is running in real mode. Unfortunately, vm86 mode requires a
//	task state segment in the address space, and there is no way for us to
//	hide it. kvm doesn't know anything about the guest physical memory map,
//	so it has to rely on userspace to supply an unused region.
//	*/
//	int intel_workaround = ioctl(kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_SET_TSS_ADDR);
//	if(intel_workaround > 0) {
//		err = ioctl(vm_fd, KVM_SET_TSS_ADDR, TSS_ADDR);
//		if(err < 0) {
//			return -errno;
//		}
//	}

	return 0;
}
