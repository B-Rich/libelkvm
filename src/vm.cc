#include <cstring>
#include <memory>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stropts.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <elkvm.h>
#include <environ.h>
#include <elfloader.h>
#include <flats.h>
#include <kvm.h>
#include <pager.h>
#include <region-c.h>
#include <stack.h>
#include <vcpu.h>
#include "debug.h"

/*
 * Load a flat binary into the guest address space
 * returns 0 on success, an errno otherwise
 */
int elkvm_load_flat(struct kvm_vm *, struct elkvm_flat *, const std::string,
    int kernel);

namespace Elkvm {
  extern ElfBinary binary;
  extern Stack stack;
  extern std::unique_ptr<RegionManager> rm;
}

int elkvm_vm_create(struct elkvm_opts *opts, struct kvm_vm *vm, int mode, int cpus,
    const struct elkvm_handlers *handlers, const char *binary) {
	int err = 0;

	if(opts->fd <= 0) {
		return -EIO;
	}

	vm->fd = ioctl(opts->fd, KVM_CREATE_VM, 0);
	if(vm->fd < 0) {
		return -errno;
	}

	vm->run_struct_size = ioctl(opts->fd, KVM_GET_VCPU_MMAP_SIZE, 0);
	if(vm->run_struct_size < 0) {
		return -EIO;
	}

  Elkvm::rm.reset(new Elkvm::RegionManager(opts->fd));
  //XXX set pml4 address!

	for(int i = 0; i < cpus; i++) {
		err = kvm_vcpu_create(vm, mode);
		if(err) {
			return err;
		}
	}

  Elkvm::ElfBinary bin(binary);

  guestptr_t entry = bin.get_entry_point();
	err = kvm_vcpu_set_rip(elkvm_vcpu_get(vm, 0), entry);
  assert(err == 0);

  Elkvm::Environment env(bin);

  struct kvm_vcpu *vcpu = elkvm_vcpu_get(vm, 0);
  Elkvm::stack.init(vcpu, env);

  err = env.fill(opts, vm);
  if(err) {
    return err;
  }

	err = elkvm_gdt_setup(vm);
	if(err) {
		return err;
	}

	struct elkvm_flat idth;
  std::string isr_path(RES_PATH "/isr");
	err = elkvm_load_flat(vm, &idth, isr_path, 1);
	if(err) {
    if(err == -ENOENT) {
      printf("LIBELKVM: ISR shared file could not be found\n");
    }
		return err;
	}

	err = elkvm_idt_setup(vm, &idth);
	if(err) {
		return err;
	}

	struct elkvm_flat sysenter;
  std::string sysenter_path(RES_PATH "/entry");
	err = elkvm_load_flat(vm, &sysenter, sysenter_path, 1);
	if(err) {
    if(err == -ENOENT) {
      printf("LIBELKVM: SYSCALL ENTRY shared file could not be found\n");
    }
		return err;
	}

  std::string sighandler_path(RES_PATH "/signal");
  vm->sighandler_cleanup = new(struct elkvm_flat);
  assert(vm->sighandler_cleanup != NULL);

  err = elkvm_load_flat(vm, vm->sighandler_cleanup, sighandler_path, 0);
  if(err) {
    if(err == -ENOENT) {
      printf("LIBELKVM: SIGNAL HANDLER shared file could not be found\n");
    }
    return err;
  }

	/*
	 * setup the lstar register with the syscall handler
	 */
	err = kvm_vcpu_set_msr(vm->vcpus->vcpu,
			VCPU_MSR_LSTAR,
			sysenter.region->guest_virtual);
	if(err) {
		return err;
	}

  elkvm_signal_init(vm);

	vm->syscall_handlers = handlers;

  for(int i = 0; i < RLIMIT_NLIMITS; i++) {
    err = getrlimit(i, &vm->rlimits[i]);
    assert(err == 0);
  }

	return 0;
}

int elkvm_set_debug(struct kvm_vm *vm) {
  vm->debug = 1;
  return 0;
}

int elkvm_load_flat(struct kvm_vm *vm, struct elkvm_flat *flat,
    const std::string path,
    int kernel) {
	int fd = open(path.c_str(), O_RDONLY);
	if(fd < 0) {
		return -errno;
	}

	struct stat stbuf;
	int err = fstat(fd, &stbuf);
	if(err) {
		close(fd);
		return -errno;
	}

	flat->size = stbuf.st_size;
  std::shared_ptr<Elkvm::Region> region =
    Elkvm::rm->allocate_region(stbuf.st_size);

  if(kernel) {
    guestptr_t addr = elkvm_pager_map_kernel_page(&vm->pager,
        region->base_address(), 0, 1);
    if(addr == 0x0) {
      close(fd);
      return -ENOMEM;
    }
    region->set_guest_addr(addr);
  } else {
    /* XXX this will break! */
    region->set_guest_addr(0x1000);
    err = Elkvm::rm->get_pager().map_user_page(
        region->base_address(),
        region->guest_address(),
        PT_OPT_EXEC);
    assert(err == 0);
  }

	char *buf = reinterpret_cast<char *>(region->base_address());
	int bufsize = ELKVM_PAGESIZE;
	int bytes = 0;
	while((bytes = read(fd, buf, bufsize)) > 0) {
		buf += bytes;
	}

	close(fd);
  flat->region = region->c_region();

	return 0;
}

int elkvm_vcpu_count(struct kvm_vm *vm) {
	int count = 0;
	struct vcpu_list *vl = vm->vcpus;
	if(vl == NULL) {
		return 0;
	}

	while(vl != NULL) {
		if(vl->vcpu != NULL) {
			count++;
		}
		vl = vl->next;
	}
	return count;
}

int elkvm_init(struct elkvm_opts *opts, int argc, char **argv, char **environ) {
	opts->argc = argc;
	opts->argv = argv;
	opts->environ = environ;

	opts->fd = open(KVM_DEV_PATH, O_RDWR);
	if(opts->fd < 0) {
		return -errno;
	}

	int version = ioctl(opts->fd, KVM_GET_API_VERSION, 0);
	if(version != KVM_EXPECT_VERSION) {
		return -ENOPROTOOPT;
	}

	opts->run_struct_size = ioctl(opts->fd, KVM_GET_VCPU_MMAP_SIZE, 0);
	if(opts->run_struct_size <= 0) {
		return -EIO;
	}

	return 0;
}

int elkvm_cleanup(struct elkvm_opts *opts) {
	close(opts->fd);
	opts->fd = 0;
	opts->run_struct_size = 0;
	return 0;
}

int elkvm_chunk_remap(struct kvm_vm *vm, int num, uint64_t newsize) {
  auto chunk = Elkvm::rm->get_pager().get_chunk(num);
  chunk->memory_size = 0;

	int err = ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, chunk.get());
  assert(err == 0);
  free((void *)chunk->userspace_addr);
  chunk->memory_size = newsize;
  err = posix_memalign(((void **)&chunk->userspace_addr), ELKVM_PAGESIZE, chunk->memory_size);
  assert(err == 0);
	err = ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, chunk.get());
  assert(err == 0);
  return 0;
}

struct kvm_vcpu *elkvm_vcpu_get(struct kvm_vm *vm, int vcpu_id) {
  struct vcpu_list *vcpu_list = vm->vcpus;
  for(int i = 0; i < vcpu_id && vcpu_list != NULL; i++) {
    vcpu_list = vcpu_list->next;
  }

  if(vcpu_list == NULL) {
    return NULL;
  }

  return vcpu_list->vcpu;
}

uint64_t elkvm_chunk_count(struct kvm_vm *vm __attribute__((unused))) {
  return Elkvm::rm->get_pager().chunk_count();
}

struct kvm_userspace_memory_region elkvm_get_chunk(
    struct kvm_vm *vm __attribute__((unused)), int chunk) {
  return *Elkvm::rm->get_pager().get_chunk(chunk);
}

void elkvm_emulate_vmcall(struct kvm_vcpu *vcpu) {
  /* INTEL VMCALL instruction is three bytes long */
  vcpu->regs.rip +=3;
}

int elkvm_dump_valid_msrs(struct elkvm_opts *opts) {
	struct kvm_msr_list *list = reinterpret_cast<struct kvm_msr_list *>(
      malloc( sizeof(struct kvm_msr_list) + 255 * sizeof(uint32_t)));
	list->nmsrs = 255;

	int err = ioctl(opts->fd, KVM_GET_MSR_INDEX_LIST, list);
	if(err < 0) {
		free(list);
		return -errno;
	}

	for(unsigned i = 0; i < list->nmsrs; i++) {
		printf("MSR: 0x%x\n", list->indices[i]);
	}
	free(list);

	return 0;
}

void elkvm_print_regions(struct kvm_vm *vm) {
	printf("\n System Memory Regions:\n");
	printf(" ----------------------\n");
	printf(" Host virtual\t\tGuest virtual\t\tSize\t\t\tD\n");
	elkvm_dump_region(*list_elem_front(vm->root_region));
	printf("\n");
}

void elkvm_dump_region(struct elkvm_memory_region *region) {
	printf("%16p\t0x%016lx\t0x%016lx\t%i\n", region->host_base_p,
		region->guest_virtual, region->region_size, region->grows_downward);
	if(region->lc != NULL) {
		elkvm_dump_region(region->lc);
	}
	if(region->rc != NULL) {
		elkvm_dump_region(region->rc);
	}
}

