#ifndef _DBG_H_
#define _DBG_H_
#include <common/bwio.h>
#include <common/bwio.h>
#include <common/ts7200.h>
#include <task.h>

#ifdef DEBUG 
#define dbg_noprefix(arg0, ...) \
bwprintf(COM2, arg0"\n\r", ##__VA_ARGS__); 
#define dbg_noprefixnnl(arg0, ...) \
bwprintf(COM2, arg0, ##__VA_ARGS__); 
#define dbgnnl(arg0, ...) \
bwprintf(COM2, "DEBUG: %s: "arg0,__func__, ##__VA_ARGS__); 
#define dbg(arg0, ...) \
bwprintf(COM2, "DEBUG: %s: "arg0"\n\r",__func__, ##__VA_ARGS__); 
#else
#define dbgnnl(...) /* No newline */
#define dbg_noprefix(...)
#define dbg(...)
#define dbg_noprefixnnl(...)
#endif

/* TODO implement something better (i.e which terminates gracefully) */
#define kassert(exp, msg, ...) { \
  if(!(exp)) { \
    bwprintln("\r\n================================================================================="); \
    bwprintln("KASSERT FAILURE in %s (l%d): " #exp" | " msg, __func__, __LINE__, ##__VA_ARGS__); \
    bwprintln("================================================================================="); \
    while(1) {} \
  } \
}

/* t == temporary, for debugging without the cruft of a full trace :/ */
#define dbgt(arg0, ...) { \
    bwprintf(COM2, "DELETE ME: %s (l%d): "arg0"\n\r",__func__, __LINE__, ##__VA_ARGS__); \
}

#define dbgtc(cond, arg0, ...) { \
  if(cond) { \
    bwprintf(COM2, "DELETE ME: %s: "arg0"\n\r",__func__, ##__VA_ARGS__); \
  } \
}

/* Dump all registers to STDOUT preserving everything except the program counter. */

/* Save all scratch registers  */
#define DBG_SAVE_ALL_THE_THINGS \
  asm("stmfd sp, { r0-r15 }"); \
  asm("sub sp, sp, #64"); \
  asm("mrs r0, CPSR"); \
  asm("stmfd sp!, { r0 }"); 

/* Restore scratch registers (for debugging) */
#define DBG_RESTORE_ALL_THE_THINGS \
  asm("ldmfd sp!, { r0 }"); \
  asm("msr CPSR, r0"); \
  asm("ldmfd sp, { r0-r3 }"); \
  asm("add sp, sp, #48"); \
  asm("ldmfd sp, { r12 }"); \
  asm("add sp, sp, #8"); \
  asm("ldmfd sp, { lr }"); \
  asm("add sp, sp, #8"); 

#define DBG_DUMP_REG() \
  DBG_SAVE_ALL_THE_THINGS \
  dbg_print_reg(); \
  DBG_RESTORE_ALL_THE_THINGS

/* Print the hex encoded value of the opcode corresponding to an asm statement */

#define DBG_PRINT_OPCODE(inst, res) \
  asm("ldr r0, [pc]"); \
  asm("b skip"); \
  asm(inst); \
  asm("skip:\n\t"); \
  asm("mov %0, r0" :  \
    "=r" (res)); \
  dbg("Opcode: %x", res);


#define DBG_DUMP_PROC_STATE(msg, var) {\
  dbg("%s \n\r", msg); \
  dbg("\t"#var"->psr 0x%x", var->psr); \
  dbg("\t"#var"->r0 0x%x", var->r0); \
  dbg("\t"#var"->r1 0x%x", var->r1); \
  dbg("\t"#var"->r2 0x%x", var->r2); \
  dbg("\t"#var"->r3 0x%x", var->r3); \
  dbg("\t"#var"->r4 0x%x", var->r4); \
  dbg("\t"#var"->r5 0x%x", var->r5); \
  dbg("\t"#var"->r6 0x%x", var->r6); \
  dbg("\t"#var"->r7 0x%x", var->r7); \
  dbg("\t"#var"->r8 0x%x", var->r8); \
  dbg("\t"#var"->r9 0x%x", var->r9); \
  dbg("\t"#var"->r10 0x%x", var->r10); \
  dbg("\t"#var"->r11 0x%x", var->r11); \
  dbg("\t"#var"->ip 0x%x", var->ip); \
  dbg("\t"#var"->sp 0x%x", var->sp); \
  dbg("\t"#var"->lr 0x%x", var->lr); \
  dbg("\t"#var"->pc 0x%x", var->pc); \
}

void dbg_print_reg();
int dbg_start_timer();
int dbg_timer_val();
void dbg_print_syscall(int comment);
void dbg_print_bytes(char *start, int len);
void dbg_print_task(task_t *t);

#define pnum(num) \
    char numloc[12]; \
    bwi2a(num, numloc); \
    putstr(COM2, numloc); \
    putstr(COM2, "\r\n");

#endif
