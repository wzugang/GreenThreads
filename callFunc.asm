    extern printf      ; the C function, to be called
    extern average
    extern example_func
    extern addThreadData
    extern hitASM
    extern printThreadData

    SECTION .data       ; Data section, initialized variables

fmt:    db "Entered asm func", 10, 0 ;
val:    db "Val: %u", 10, 0

    SECTION .bss

mainstack:      resd 1 ; Stored mainstack esp
currentthread:  resd 1 ; Pointer to current thread

    SECTION .text

    ; extern void yield(uint32_t status);
    global yield
yield:
    push    ebp
    mov     ebp, esp

    ; Get curThread pointer
    mov     edx, dword [currentthread]
    ; Get return address
    mov     eax, [ebp+4]
    ; Get yield status value
    mov     ecx, [ebp+8]
    ; Set validity of thread
    mov     [edx + 28], ecx ; ThreadData->stillValid
    ; Set return address to continue execution
    mov     [edx + 4], eax  ; ThreadData->curFuncAddr
    ; Determine current value of esp from perspective of curThread
    mov     ecx, esp
    ; Need to remove ebp, return address from consideration
    add     ecx, 8
    ; Set curThread StackCur value
    mov     [edx+16], ecx   ; ThreadData->t_StackCur

    ; Restore ebp and "pop" return address and yield status off stack
    pop     ebp
    add     esp, 8

    ; Save ebp for thread
    mov     [edx + 24], ebp

    jmp     schedulerReturn


    ; extern void callFunc(size_t argBytes, void* funcAddr, uint8_t* stackPtr, ThreadData* curThread);
    global callFunc
callFunc:
    push    ebp                     ; set up stack frame
    mov     ebp,esp

    push    ebx                     ; Register preservation
    push    esi

    ; Get function arguments. Args START at [ebp + 8]
    ; First set the currentthread value
    mov     edx, dword [ebp + 20]   ; ThreadData*, curThread
    mov     dword [currentthread], edx

    ; Determine if we are starting a new thread, or if we're continuing
    ; execution
    mov     ecx, [edx + 4] ; ThreadData->curFuncAddr (0 if start of thread)
    test    ecx, ecx
    ; If not zero, then continue where we left off
    jne     continueThread

    ; Get rest of arguments
    mov     eax, dword [ebp + 8]    ; argBytes (counter)
    mov     ecx, dword [ebp + 12]   ; funcAddr (address)
    mov     esi, dword [ebp + 16]   ; stack bottom (pointer)

    mov     [edx + 4], ecx  ; ThreadData->curFuncAddr, init to start of func

    ; Set stack pointer to be before arguments
    sub     esi, eax
    ; Allocate 4 bytes for return address
    sub     esi, 4
    mov     dword [esi], schedulerReturn

    ; Store the value of the main stack pointer, and store ebp as top value
    push    ebp
    mov     [mainstack], esp
    mov     esp, esi

    jmp     ecx                     ; Call function


continueThread:
    ; Get ThreadData* curThread (probably already in edx anyway)
    mov     edx, dword [currentthread]
    ; In order to restart execution, we need to set esp to correct value,
    ; and then "return" to the previous stage of execution. As part of setting
    ; up for a clean return, push ebp as the last thing on the mainstack
    push    ebp
    ; Save mainstack esp
    mov     [mainstack], esp
    ; Set esp to StackCur of current thread
    mov     esp, [edx + 16] ; ThreadData->t_StackCur
    ; Set ebp to t_ebp of current thread
    mov     ebp, [edx + 24]
    ; Set stillValid to 0, to account for possibly naturally returning from
    ; the function at the end of its execution. A thread is still valid if
    ; stillValid != 0 OR curFuncAddr == 0 (meaning thread hasn't started yet)
    mov     dword [edx + 28], 0
    ; Get "return" address to return to thread execution point
    mov     edx, [edx + 4]
    jmp     edx


schedulerReturn:
    ; Restore the value of the main stack pointer
    mov     esp, [mainstack]
    ; Restore current ebp
    pop     ebp

    mov     ebx, [ebp - 4]                     ; Restore registers
    mov     esi, [ebp - 8]

    mov     esp, ebp                ; takedown stack frame
    pop     ebp
    ret
