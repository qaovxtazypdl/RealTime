.global	_swi_handler
.align	4

.macro HWI
    stmfd sp!, {r0, r1}
    LDR R1, =0x10140018 
    ORR R0, R0, #0x80000
    str R0, [R1]
    ldmfd sp!, {R0, R1}
.endm

.macro dumpreg reg
	stmfd sp, { \reg }
	sub sp, sp, #4
	stmfd sp!, { r0, r1, lr }

	mov r0, #1
	mov r1, #100
	bl bwputc

	mov r0, #1
	ldr r1, [sp, #12]
	bl bwputr

	mov r0, #1
	mov r1, #0x0D
	bl bwputc

	mov r0, #1
	mov r1, #0x0A
	bl bwputc


	ldmfd sp!, { r0, r1, lr}
	ldmfd sp!, { \reg }
.endm

@ The SWI ISR which passes control to a C function which consumes and returns a pointer to a
@ proc_state struct corresponding to the current and desired state of the CPU respectively.
	
_swi_handler:
	@ Switch into system mode
	msr cpsr_c, #0xDF

	stmfd sp, { r0-r14 }
	sub r0, sp, #60

	msr cpsr_c, #0xD3
	mrs r1, spsr 
	stmfd r0!, { r1, lr }

	@ Pass comment field as the second argument

	ldr r1, [lr, #-4]
	bic r1, r1, #0xFF000000

	bl swi

	@ Restore process state and switch to user mode

	@ In theory we could use r0 as the stack pointer - proc struct if we assume the returned value
	@ is a pointer we handed the C function, however it is better not to make this assumption and
	@ the stack pointer is part of the struct we receive anyway.

	ldmfd r0!, { r1, lr }
	msr spsr, r1
	ldmfd r0, { r0-r14 }^
	movs pc, lr
