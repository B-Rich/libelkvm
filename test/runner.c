#include <CUnit/Basic.h>

#include "test_kvm.h"
#include "test_pager.h"
#include "test_vcpu.h"
#include "test_vm.h"

int main() {
	CU_pSuite kvm_suite = NULL;
	CU_pSuite vm_suite = NULL;
	CU_pSuite pager_suite = NULL;
	CU_pSuite vcpu_suite = NULL;

	if(CUE_SUCCESS != CU_initialize_registry()) {
		return CU_get_error();
	}

	/* add the suite for the pager tests */
	pager_suite = CU_add_suite("Pager Suite", init_pager_suite, clean_pager_suite);
	if(NULL == pager_suite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if((NULL == CU_add_test(pager_suite, "kvm_pager_initialize", test_kvm_pager_initialize)) ||
	(NULL == CU_add_test(pager_suite, "kvm_pager_is_invalid_guest_base", test_kvm_pager_is_invalid_guest_base)) ||
	(NULL == CU_add_test(pager_suite, "kvm_pager_create_mem_chunk", test_kvm_pager_create_mem_chunk)) ||
	(NULL == CU_add_test(pager_suite, "kvm_pager_create_page_table", test_kvm_pager_create_page_tables))
		) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the vcpu test suite */
	vcpu_suite = CU_add_suite("VCPU Test Suite", init_vcpu_suite, clean_vcpu_suite);
	if(NULL == vcpu_suite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if(
		(NULL == CU_add_test(vcpu_suite, "kvm_vcpu_create", test_kvm_vcpu_create)) ||
		(NULL == CU_add_test(vcpu_suite, "kvm_vcpu_add_tail", test_kvm_vcpu_add_tail)) ||
		(NULL == CU_add_test(vcpu_suite, "kvm_vcpu_destroy", test_kvm_vcpu_destroy)) ||
		(NULL == CU_add_test(vcpu_suite, "kvm_vcpu_get_regs", test_kvm_vcpu_get_regs)) ||
		(NULL == CU_add_test(vcpu_suite, "kvm_vcpu_set_regs", test_kvm_vcpu_set_regs))
		) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the suite for kvm */
	kvm_suite = CU_add_suite("KVM_Suite", init_vm_suite, clean_vm_suite);
	if(NULL == kvm_suite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if((NULL == CU_add_test(kvm_suite, "Test kvm_init()", test_kvm_init)) ||
		(NULL == CU_add_test(kvm_suite, "Test kvm_cleanup()", test_kvm_cleanup))
		) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the suite for the elkvm */
	vm_suite = CU_add_suite("VM_Suite", init_vm_suite, clean_vm_suite);
	if(NULL == vm_suite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if((NULL == CU_add_test(vm_suite, "Test kvm_vm_create()", test_kvm_vm_create)) ||
		(NULL == CU_add_test(vm_suite, "kvm_vm_vcpu_count()", test_kvm_vm_vcpu_count))
		) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* run the tests */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}

