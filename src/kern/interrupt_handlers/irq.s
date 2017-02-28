.global	_irq_handler
.align	4

.macro dumpsp
	stmfd sp!, { r0, r1, lr }

	mov r0, #1
	mov r1, #100
	bl bwputc

	mov r0, #1
	add r1, sp, #12
	bl bwputr

	mov r0, #1
	mov r1, #0x0D
	bl bwputc

	mov r0, #1
	mov r1, #0x0A
	bl bwputc

	ldmfd sp!, { r0, r1, lr}
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

@ Identical to the swi handler with the exception that it calls the irq c function
@ and accounts for the fact that the link register is the return address + 4.
@ and does so in IRQ mode rather than supervisor mode.

_irq_handler:
	@ The IRQ mode LR (unlike the SWI LR) refers to the instruction after the one which needs to be next executed
	sub lr, lr, #4 

	@ Switch into system mode
	msr cpsr_c, #0xDF

	stmfd sp, { r0-r14 }
	sub r0, sp, #60

	msr cpsr_c, #0xD2
	mrs r1, spsr 
	stmfd r0!, { r1, lr }

	bl irq

	@ Restore process state and switch to user mode

	@ In theory we could use r0 as the stack pointer - proc struct if we assume the returned value
	@ is a pointer we handed the C function, however it is better not to make this assumption and
	@ the stack pointer is part of the struct we receive anyway.

	ldmfd r0!, { r1, lr }
	msr spsr, r1
	ldmfd r0, { r0-r14 }^
	movs pc, lr
