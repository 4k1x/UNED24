#include "syscall.h"
#include "../arch/msr.h"
#include "../arch/gdt.h"
#include "../util/printf.h"

#define SYSRET(ctx, val) ctx->rax = val; return;
extern void syscall_entry();
void undefined_syscall_handler(struct cpu_context* ctx) {
    printf("Undefined syscall %d\n", ctx->rax);
    SYSRET(ctx, SYSCALL_UNDEFINED);
}

syscall_handler syscall_handlers[SYSCALL_HANDLER_COUNT] = {
    [0 ... SYSCALL_HANDLER_COUNT-1] = undefined_syscall_handler
};
void global_syscall_handler(struct cpu_context* ctx) {
    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        syscall_handlers[ctx->rax](ctx);
    } else {
        printf("Syscall number overflow %d\n", ctx->rax);
        SYSRET(ctx, SYSCALL_ERROR);
    }
}

void syscall_enable() {
    cpu_set_msr_u64(MSR_EFER, cpu_get_msr_u64(MSR_EFER) | 1);
    cpu_set_msr_u64(MSR_STAR, 0x23000800000000);
    cpu_set_msr_u64(MSR_LSTAR, (uint64_t)syscall_entry);
    cpu_set_msr_u64(MSR_SYSCALL_FLAG_MASK, 0x200);
}

void syscall_set_gs(uintptr_t addr)
{
    cpu_set_msr_u64(MSR_GS_BASE, addr);
    cpu_set_msr_u64(MSR_KERN_GS_BASE, addr);
}

void syscall_jump_to_usermode(uint64_t rip, uint64_t rsp) {
    uint16_t ds = get_user_data_selector() | 3;
    uint16_t cs = get_user_code_selector() | 3;

    __asm__("mov %0, %%ds" :: "r" (ds));
    __asm__("push %0" :: "r" (ds));
    __asm__("push %0" :: "r" (rsp));
    __asm__("push %0" :: "r" ((uint64_t)(SYSCALL_INITIAL_FLAGS)));
    __asm__("push %0" :: "r" (cs));
    __asm__("push %0" :: "r" (rip));
    __asm__("iretq");
}