.code
sqrt PROC
    sqrtsd xmm0, xmm0
    ret
sqrt ENDP
verify_sqrt PROC
    sub rsp, 40
    call root
    add rsp, 40
    mov rax, 4022000000000000h
    movq xmm1, rax
    ucomisd xmm0, xmm1
    sete al
    setnp cl
    and al, cl
    movzx eax, al
    ret
verify_sqrt ENDP
EXTERN root:PROC
END
