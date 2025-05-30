.intel_syntax noprefix

.text
.code64
.global x86_call_into

// this is mostly redundant and exists to somewhat emulate the old x86_switch_mode
x86_call_into:
        push    rbp
        call    rdi
        pop     rbp
        ret

