#include "vcpu.h"

#define LINUX_64_STACK_BASE 0x800000000000

/*
 * Push a value onto the Stack of the VM
 */
int push_stack(struct kvm_vm *, struct kvm_vcpu *, uint16_t);

/*
 * Pop a value from the VM's Stack
 */
uint16_t pop_stack(struct kvm_vm *, struct kvm_vcpu *);

/*
 * Dump the stack to stdout
 */
void dump_stack(struct kvm_vm *);
