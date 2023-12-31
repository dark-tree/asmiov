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
        sub     rsp, 0x18

        mov     rbx, rsp
        movq    [rbx], offset .L1

        mov     ecx, [code_segment]
        mov     [rbx+4], ecx

        // save value of DS
        mov     rcx, ds
        push    rcx

        // set DS to the selected segment
        mov     ecx, [data_segment]
        mov     ds, ecx

        // before the lcall, switch to a stack below 4GB
        // this assumes that the data segment is below 4GB
        mov     rsp, offset stack+0xf0
        lcall   [rbx]
        movsxd  rax, eax

        // restore value of DS
        pop     rcx
        mov     ds, rcx

        // restore rsp to the original stack
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
