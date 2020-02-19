-- kcfi-tools Copyright (C) 2015 Universidade Estadual de Campinas
--
-- This software was developed by Joao Moreira <joao@overdrivepizza.com>
-- at Universidade Estadual de Campinas, SP, Brazil, in 2015.
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

hcr_ids = {}
hcr_ids["__put_user_1"] = "1337beef"
hcr_ids["__put_user_2"] = "1337beef"
hcr_ids["__put_user_4"] = "1337beef"
hcr_ids["__put_user_8"] = "1337beef"
hcr_ids["__get_user_1"] = "1337beef"
hcr_ids["__get_user_2"] = "1337beef"
hcr_ids["__get_user_4"] = "1337beef"
hcr_ids["__get_user_8"] = "1337beef"

ids = {}
ids[#ids+1] = "interrupt"
ids[#ids+1] = "saved_rbp"
ids[#ids+1] = "_copy_to_user"
ids[#ids+1] = "__memcpy"
ids[#ids+1] = "apic_timer"
ids[#ids+1] = "swsusp_arch_suspend"
ids[#ids+1] = "restore_registers"
ids[#ids+1] = "fold_64"
ids[#ids+1] = "nowork"
ids[#ids+1] = "salsa20_encrypt_bytes"
ids[#ids+1] = "salsa20_keysetup"
ids[#ids+1] = "salsa20_ivsetup"
ids[#ids+1] = "__clmul_gf128mul_ble"
ids[#ids+1] = "clmul_ghash_mul"
ids[#ids+1] = "clmul_ghash_update"
ids[#ids+1] = "crc_t10dif_pcl"
ids[#ids+1] = "sha256_transform_rorx"
ids[#ids+1] = "sha256_transform_avx"
ids[#ids+1] = "sha256_transform_ssse3"
ids[#ids+1] = "sha512_transform_rorx"
ids[#ids+1] = "__serpent_enc_blk_8way"
ids[#ids+1] = "__serpent_enc_blk_8way"
ids[#ids+1] = "serpent_dec_blk_8way"
ids[#ids+1] = "twofish_enc_blk"
ids[#ids+1] = "twofish_dec_blk"
ids[#ids+1] = "__twofish_enc_blk8"
ids[#ids+1] = "__twofish_dec_blk8"
ids[#ids+1] = "twofish_ecb_enc_8way"
ids[#ids+1] = "twofish_ecb_dec_8way"
ids[#ids+1] = "twofish_cbc_dec_8way"
ids[#ids+1] = "twofish_ctr_8way"
ids[#ids+1] = "twofish_xts_enc_8way"
ids[#ids+1] = "twofish_xts_dec_8way"
ids[#ids+1] = "__twofish_enc_blk_3way"
ids[#ids+1] = "__twofish_enc_blk_3way"
ids[#ids+1] = "twofish_dec_blk_3way"
ids[#ids+1] = "__serpent_enc_blk8_avx"
ids[#ids+1] = "__serpent_dec_blk8_avx"
ids[#ids+1] = "serpent_ecb_enc_8way_avx"
ids[#ids+1] = "serpent_ecb_dec_8way_avx"
ids[#ids+1] = "serpent_cbc_dec_8way_avx"
ids[#ids+1] = "serpent_ctr_8way_avx"
ids[#ids+1] = "serpent_xts_enc_8way_avx"
ids[#ids+1] = "serpent_xts_dec_8way_avx"
ids[#ids+1] = "__serpent_enc_blk16"
ids[#ids+1] = "__serpent_dec_blk16"
ids[#ids+1] = "serpent_ecb_enc_16way"
ids[#ids+1] = "serpent_ecb_dec_16way"
ids[#ids+1] = "serpent_cbc_dec_16way"
ids[#ids+1] = "serpent_ctr_16way"
ids[#ids+1] = "serpent_xts_enc_16way"
ids[#ids+1] = "serpent_xts_dec_16way"
ids[#ids+1] = "__cast6_enc_blk8"
ids[#ids+1] = "__cast6_dec_blk8"
ids[#ids+1] = "cast6_ecb_enc_8way"
ids[#ids+1] = "cast6_ecb_dec_8way"
ids[#ids+1] = "cast6_cbc_dec_8way"
ids[#ids+1] = "cast6_ctr_8way"
ids[#ids+1] = "cast6_xts_enc_8way"
ids[#ids+1] = "cast6_xts_dec_8way"
ids[#ids+1] = "__camellia_enc_blk"
ids[#ids+1] = "camellia_dec_blk"
ids[#ids+1] = "blowfish_dec_blk"
ids[#ids+1] = "blowfish_dec_blk_4way"
ids[#ids+1] = "__blowfish_enc_blk"
ids[#ids+1] = "__blowfish_enc_blk_4way"
ids[#ids+1] = "camellia_dec_blk_2way"
ids[#ids+1] = "__camellia_enc_blk_2way"
ids[#ids+1] = "cast5_cbc_dec_16way"
ids[#ids+1] = "cast5_ctr_16way"
ids[#ids+1] = "__cast5_dec_blk16"
ids[#ids+1] = "cast5_ecb_dec_16way"
ids[#ids+1] = "cast5_ecb_enc_16way"
ids[#ids+1] = "__cast5_enc_blk16"
ids[#ids+1] = "ptregscall_common"
ids[#ids+1] = "gs_change"
ids[#ids+1] = "do_softirq_own_stack"
ids[#ids+1] = "memmove"
ids[#ids+1] = "memset"
ids[#ids+1] = "__copy_user_nocache"
ids[#ids+1] = "bad_from_user"
ids[#ids+1] = "clear_page_c"
ids[#ids+1] = "clear_page_c_e"
ids[#ids+1] = "clear_page"
ids[#ids+1] = "copy_page_rep"
ids[#ids+1] = "copy_page"
ids[#ids+1] = "copy_user_generic_unrolled"
ids[#ids+1] = "copy_user_generic_string"
ids[#ids+1] = "copy_user_enhanced_fast_string"
ids[#ids+1] = "bad_to_user"
ids[#ids+1] = "__get_user_1"
ids[#ids+1] = "__get_user_2"
ids[#ids+1] = "__get_user_4"
ids[#ids+1] = "__get_user_8"
ids[#ids+1] = "bad_get_user"
ids[#ids+1] = "memcpy"
ids[#ids+1] = "__put_user_1"
ids[#ids+1] = "__put_user_2"
ids[#ids+1] = "__put_user_4"
ids[#ids+1] = "__put_user_8"
ids[#ids+1] = "bad_put_user"
ids[#ids+1] = "csum_partial_copy_generic"
ids[#ids+1] = "__iowrite32_copy"
ids[#ids+1] = "swap_pages"
ids[#ids+1] = "virtual_mapped"
ids[#ids+1] = "identity_mapped"
ids[#ids+1] = "relocate_kernel"
ids[#ids+1] = "sha1_transform_avx2"
ids[#ids+1] = "sha1_transform_ssse3"
ids[#ids+1] = "sha1_transform_avx"
ids[#ids+1] = "aes_enc_blk"
ids[#ids+1] = "aes_dec_blk"
ids[#ids+1] = "crc_pcl"
ids[#ids+1] = "do_machine_check"
ids[#ids+1] = "do_alignment_check"
ids[#ids+1] = "do_bounds"
ids[#ids+1] = "do_coprocessor_error"
ids[#ids+1] = "do_coprocessor_segment_overrun"
ids[#ids+1] = "do_device_not_available"
ids[#ids+1] = "do_divide_error"
ids[#ids+1] = "do_double_fault"
ids[#ids+1] = "do_general_protection"
ids[#ids+1] = "do_invalid_op"
ids[#ids+1] = "do_invalid_TSS"
ids[#ids+1] = "do_overflow"
ids[#ids+1] = "do_page_fault"
ids[#ids+1] = "do_segment_not_present"
ids[#ids+1] = "do_simd_coprocessor_error"
ids[#ids+1] = "do_spurious_interrupt_bug"
ids[#ids+1] = "do_stack_segment"
ids[#ids+1] = "smp_apic_timer_interrupt"
ids[#ids+1] = "smp_call_function_interrupt"
ids[#ids+1] = "smp_call_function_single_interrupt"
ids[#ids+1] = "smp_error_interrupt"
ids[#ids+1] = "smp_irq_move_cleanup_interrupt"
ids[#ids+1] = "smp_irq_work_interrupt"
ids[#ids+1] = "smp_kvm_posted_intr_ipi"
ids[#ids+1] = "smp_reboot_interrupt"
ids[#ids+1] = "smp_reschedule_interrupt"
ids[#ids+1] = "smp_spurious_interrupt"
ids[#ids+1] = "smp_thermal_interrupt"
ids[#ids+1] = "smp_threshold_interrupt"
ids[#ids+1] = "smp_x86_platform_ipi"
ids[#ids+1] = "__mutex_lock_slowpath"
ids[#ids+1] = "__mutex_unlock_slowpath"
ids[#ids+1] = "asminline_call"
ids[#ids+1] = "call_rwsem_down_read_failed"
ids[#ids+1] = "call_rwsem_down_write_failed"
ids[#ids+1] = "call_rwsem_wake"
ids[#ids+1] = "call_rwsem_downgrade_wake"
ids[#ids+1] = "__sw_hweight32"
ids[#ids+1] = "__sw_hweight64"
ids[#ids+1] = "__switch_to"
ids[#ids+1] = "bpf_internal_load_pointer_neg_helper"
ids[#ids+1] = "compat_sys_execve"
ids[#ids+1] = "compat_sys_execveat"
ids[#ids+1] = "debug_stack_reset"
ids[#ids+1] = "debug_stack_set_zero"
ids[#ids+1] = "do_debug"
ids[#ids+1] = "do_int3"
ids[#ids+1] = "do_IRQ"
ids[#ids+1] = "do_nmi"
ids[#ids+1] = "do_notify_resume"
ids[#ids+1] = "__do_softirq"
ids[#ids+1] = "dump_stack"
ids[#ids+1] = "early_fixup_exception"
ids[#ids+1] = "early_make_pgtable"
ids[#ids+1] = "early_printk"
ids[#ids+1] = "error_entry"
ids[#ids+1] = "fixup_bad_iret"
ids[#ids+1] = "preempt_schedule_irq"
ids[#ids+1] = "printk"
ids[#ids+1] = "__print_symbol"
ids[#ids+1] = "rwsem_downgrade_wake"
ids[#ids+1] = "rwsem_down_read_failed"
ids[#ids+1] = "rwsem_down_write_failed"
ids[#ids+1] = "rwsem_wake"
ids[#ids+1] = "save_paranoid"
ids[#ids+1] = "schedule"
ids[#ids+1] = "schedule_user"
ids[#ids+1] = "schedule_tail"
ids[#ids+1] = "skb_copy_bits"
ids[#ids+1] = "swap_pages"
ids[#ids+1] = "sync_regs"
ids[#ids+1] = "syscall_trace_enter"
ids[#ids+1] = "syscall_trace_enter_phase1"
ids[#ids+1] = "syscall_trace_enter_phase2"
ids[#ids+1] = "syscall_trace_leave"
ids[#ids+1] = "sys_execve"
ids[#ids+1] = "sys_execveat"
ids[#ids+1] = "sys_rt_sigreturn"
ids[#ids+1] = "sys_clone"
ids[#ids+1] = "sys_fork"
ids[#ids+1] = "sys_vfork"
ids[#ids+1] = "sys_iopl"
ids[#ids+1] = "identity_mapped"
ids[#ids+1] = "virtual_mapped"
ids[#ids+1] = "efi_call"

--- KCFI TAGS from assembly functions:
--  .functions whose prototype doesn't match the prototype of an indirect call
--  .are tagged with unique tag. Overhead is only code size, not CFG relaxation.
ids[#ids+1] = "phys_base"
ids[#ids+1] = "secondary_startup_64"
ids[#ids+1] = "start_cpu0"
ids[#ids+1] = "early_idt_handler"
ids[#ids+1] = "wakeup_long64"
ids[#ids+1] = "do_suspend_lowlevel"
ids[#ids+1] = "sha1_transform_ssse3"
ids[#ids+1] = "sha1_transform_avx"
ids[#ids+1] = "sha1_transform_avx2"
ids[#ids+1] = "crc32_pclmul_le_16"
ids[#ids+1] = "sha512_transform_ssse3"
ids[#ids+1] = "sha512_transform_avx"
ids[#ids+1] = "ret_from_fork"
ids[#ids+1] = "system_call"
ids[#ids+1] = "stub_clone"
ids[#ids+1] = "stub_fork"
ids[#ids+1] = "stub_vfork"
ids[#ids+1] = "stub_iopl"
ids[#ids+1] = "stub_execve"
ids[#ids+1] = "stub_execveat"
ids[#ids+1] = "stub_rt_sigreturn"
ids[#ids+1] = "stub_x32_rt_sigreturn"
ids[#ids+1] = "stub_x32_execve"
ids[#ids+1] = "stub_x32_execveat"
ids[#ids+1] = "irq_entries_start"
ids[#ids+1] = "native_iret"
ids[#ids+1] = "irq_move_cleanup_interrupt"
ids[#ids+1] = "reboot_interrupt"
ids[#ids+1] = "apic_timer_interrupt"
ids[#ids+1] = "x86_platform_ipi"
ids[#ids+1] = "kvm_posted_intr_ipi"
ids[#ids+1] = "threshold_interrupt"
ids[#ids+1] = "thermal_interrupt"
ids[#ids+1] = "call_function_single_interrupt"
ids[#ids+1] = "call_function_interrupt"
ids[#ids+1] = "reschedule_interrupt"
ids[#ids+1] = "error_interrupt"
ids[#ids+1] = "spurious_interrupt"
ids[#ids+1] = "irq_work_interrupt"
ids[#ids+1] = "divide_error"
ids[#ids+1] = "overflow"
ids[#ids+1] = "bounds"
ids[#ids+1] = "invalid_op"
ids[#ids+1] = "device_not_available"
ids[#ids+1] = "double_fault"
ids[#ids+1] = "coprocessor_segment_overrun"
ids[#ids+1] = "invalid_TSS"
ids[#ids+1] = "segment_not_present"
ids[#ids+1] = "spurious_interrupt_bug"
ids[#ids+1] = "coprocessor_error"
ids[#ids+1] = "alignment_check"
ids[#ids+1] = "simd_coprocessor_error"
ids[#ids+1] = "native_load_gs_index"
ids[#ids+1] = "hyperv_callback_vector"
ids[#ids+1] = "debug"
ids[#ids+1] = "int3"
ids[#ids+1] = "stack_segment"
ids[#ids+1] = "general_protection"
ids[#ids+1] = "page_fault"
ids[#ids+1] = "machine_check"
ids[#ids+1] = "paranoid_exit"
ids[#ids+1] = "error_exit"
ids[#ids+1] = "nmi"
ids[#ids+1] = "ignore_sysret"
ids[#ids+1] = "ia32_sysenter_target"
ids[#ids+1] = "ia32_cstar_target"
ids[#ids+1] = "ia32_syscall"
ids[#ids+1] = "saved_rsi"
ids[#ids+1] = "saved_rdi"
ids[#ids+1] = "saved_rbx"
ids[#ids+1] = "saved_rip"
ids[#ids+1] = "saved_rsp"
ids[#ids+1] = "saved_magic"
ids[#ids+1] = "efi_scratch"
ids[#ids+1] = "_copy_from_user"
ids[#ids+1] = "__memset"
ids[#ids+1] = "rdmsr_safe_regs"
ids[#ids+1] = "wrmsr_safe_regs"
ids[#ids+1] = "restore_image"
ids[#ids+1] = "core_restore_code"


os.remove("./include/kcfi/kcfi_tags.h")
os.remove("./build_tools/rids.txt")
os.remove("./build_tools/cids.txt")
os.remove("./build_tools/fs.txt")

--- initiate file
file = io.open("./include/kcfi/kcfi_tags.h", "w")
if not file then
  print("problem opening kcfi_tags.h")
  os.exit()
end

file:write("#ifdef CONFIG_KCFI\n")
file:write("#ifndef __KCFI_TAGS\n")
file:write("#define __KCFI_TAGS 1\n")

--- populate
i = 1
while i <= #ids do
  file:write("#define KCFIc_" .. ids[i] .. " 0x1337beef\n")
  i = i + 1
end

i = 1
while i < #ids do
  file:write("#define KCFIr_" .. ids[i] .. " 0x1337beef\n")
  i = i + 1
end

i = 1
while i < #hcr_ids do
  file:write("#define KCFIr_" .. hcr_ids[i] .. " 0x1337beef\n")
  i = i + 1
end

--- finish file
file:write("#endif\n#endif")
file:close()

--- create code snippets for special cases/macros
os.remove("./include/kcfi/kcfi_snippets.h")
file = io.open("./include/kcfi/kcfi_snippets.h", "w")
if not file then
  print("problem openin kcfi_snippets.h")
  os.exit()
end

file:write("#ifdef CONFIG_KCFI\n")
file:write("#ifndef __KCFI_CODE\n")
file:write("#define __KCFI_CODE 1\n")
file:write("#define cfi_mutex_fastpath_unlock __mutex_fastpath_unlock_cfi")
file:write("(&lock->base.count, __mutex_unlock_slowpath, 0x1337beef)\n")
file:write("#define cfi_mutex_fastpath_unlock2 __mutex_fastpath_unlock_cfi")
file:write("(&lock->count, __mutex_unlock_slowpath, 0x1337beef)\n")
file:write("#define cfi_mutex_fastpath_lock __mutex_fastpath_lock_cfi")
file:write("(&lock->count, __mutex_lock_slowpath, 0x1337beef)\n")
file:write("#endif\n#endif")
file:close()
