.intel_syntax noprefix
// https://stackoverflow.com/a/48855022

.code64
.text
.global x86_switch_mode

// executes the given callback in 32 bit mode
x86_switch_mode:
        push    rbx
        push    rbp
        mov     rbp, rsp
        sub     rsp, 0x18

        mov     rbx, rsp
        movq    [rbx], offset .L1
        mov     [rbx+4], edi

        // save value of DS
        mov rcx, ds
        push rcx

        // set DS to the same value we set CS to
        mov ds, edi

        // before the lcall, switch to a stack below 4GB
        // this assumes that the data segment is below 4GB
        mov     rsp, offset stack+0xf0
        lcall   [rbx]
        movsxd  rax, eax

        // restore value of DS
        pop rcx
        mov ds, rcx

        // restore rsp to the original stack
        leave
        pop     rbx
        ret

.code32
.L1:
        // execute the callback in compatibility mode
        call    esi
        lret

.code64
.global x86_check_mode

// returns false for 32-bit mode; true for 64-bit mode
x86_check_mode:
        xor     eax, eax

        // In 32-bit mode, this instruction is executed as
        // inc eax; test eax, eax
        test    rax, rax
        setz    al
        ret

.data
.align  16
stack:  .space 0x100
