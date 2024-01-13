.intel_syntax noprefix
// https://stackoverflow.com/a/48855022

.data
data_segment: .long 0x2B
code_segment: .long 0x23

.text
.code64
.global x86_switch_mode

// executes the given callback in 32 bit mode
x86_switch_mode:
        push    rbx
        push    rbp
        mov     rbp, rsp

        sub     rsp, 8
        mov     rbx, rsp

        // assemble the address
        movq    [rbx], offset .L1
        mov     ecx, [code_segment]
        mov     [rbx+4], ecx

        // store value of DS
        mov     r11, ds

        // set DS to the selected segment
        mov     ecx, [data_segment]
        mov     ds, ecx

        // before the lcall, switch to a stack below 4GB
        // this assumes that the data segment is below 4GB
        mov     rsp, offset stack+0xf0

        // make sure our 32 bit code can't modify our registers
        // even if the 32 bit code tries to save (push) and restore (pop)
        // the required registers (ebp, ebx, esi, edi) it will still clear
        // the high part of our 64 bit RBP and RBX registers
        push    rbp

        // make the jump to 32 bit mode
        lcall   [rbx]
        movsxd  rax, eax
        xor     rdx, rdx

        // restore RBP
        pop     rbp

        // restore value of DS
        mov     ds, r11

        // restore RSP to the original stack
        leave
        pop     rbx
        ret

.code32
.L1:
        // execute the callback in compatibility mode
        call    edi
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
