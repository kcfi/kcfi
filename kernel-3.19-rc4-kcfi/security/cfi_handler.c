#include<linux/kernel.h>
#include<linux/spinlock.h>

// Outputs below may not be quite correct depending on frame layout
// TODO: improve these functions to allow improved debugging

void call_violation_handler(void)
{
  void * ccfi_ret;
  void * ccaller_addr;
  void * r11;

  asm("\t movq 0x48(%%rsp),%0" : "=r"(ccfi_ret));
  asm("\t movq 0x50(%%rsp),%0" : "=r"(ccaller_addr));
  asm("\t movq %%r11,%0" : "=r"(r11));

  panic("kCFI: CALL VIOLATION %p:%p r11:%p\n", ccfi_ret, ccaller_addr, r11);
}

void ret_violation_handler(void)
{
  void * rcfi_ret;
  void * rcaller_addr;
  void * r11;

  asm("\t movq 0x48(%%rsp),%0" : "=r"(rcfi_ret));
  asm("\t movq 0x50(%%rsp),%0" : "=r"(rcaller_addr));
  asm("\t movq %%r11,%0" : "=r"(r11));

  panic("kCFI: RETURN VIOLATION %p:%p r11:%p\n", rcfi_ret, rcaller_addr, r11);
}

void asm_violation_handler(void)
{
  void * rcfi_ret;
  void * rcaller_addr;
  void * r11;

  asm("\t movq 0x8(%%rsp),%0" : "=r"(rcfi_ret));
  asm("\t movq 0x10(%%rsp),%0" : "=r"(rcaller_addr));
  asm("\t movq %%r11,%0" : "=r"(r11));

  panic("kCFI: ASM VIOLATION %p:%p r11:%p\n", rcfi_ret, rcaller_addr, r11);
}

EXPORT_SYMBOL(ret_violation_handler);
EXPORT_SYMBOL(call_violation_handler);
EXPORT_SYMBOL(asm_violation_handler);
