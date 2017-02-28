#include <common/ds/queue.h>
#include <common/err.h>
#include <common/mem.h>
#include <common/syscall.h>
#include <common/ts7200.h>
#include <common/uart.h>
#include <common/timer.h>
#include <arm_v4.h>
#include <event.h>
#include <ipc.h>
#include <task.h>
#include <sched.h>
#include <proc_state.h>

void initTask();

static char original_ivt[64];

/* Must be called upon exit, restores the value of the low mem addresses we */
/* overwrote to allow the kernel to handle interrupts (redboot might need these) */

void restore_ivt() {
  memcpy((char*)0, original_ivt, 0x40);
}

/* Initialize the interrupt  */
void init_ivt() {
  void _swi_handler();
  void _irq_handler();

  memcpy(original_ivt, (char*)0, 0x40);

  STORE_OPCODE(0x8, "ldr pc, [pc, #0x18]");
  STORE_OPCODE(0x18, "ldr pc, [pc, #0x18]");

  *((int*)0x28) = (int)_swi_handler;
  *((int*)0x38) = (int)_irq_handler;
}

/* Initialize the ICU with the hardware interrupts of interest. */
void init_icu() {
  volatile int* const VIC1ENABLE = (int*)(VIC1_BASE + VIC_ENABLE_OFFSET);
  volatile int* const VIC2ENABLE = (int*)(VIC2_BASE + VIC_ENABLE_OFFSET);
  volatile int* const VIC2SELECT = (int*)(VIC2_BASE + VIC_SELECT_OFFSET);
  volatile int* const VIC1SELECT = (int*)(VIC1_BASE + VIC_SELECT_OFFSET);

  /* All hardware interrupts should map to an IRQ */
  *VIC2SELECT = 0; 
  *VIC1SELECT = 0; 

  *VIC1ENABLE = 0x0;
  *VIC2ENABLE = VIC_UART2_EN | VIC_UART1_EN | VIC_TIMER3_EN;
}

void init_bwio() {
  bwsetfifo( COM2, 0 );
  bwsetfifo( COM1, 0 );
  bwsetspeed( COM2, 115200 );
  bwsetspeed( COM1, 2400 );
}

void run_first_task(void (*start)()) {
  task_t *t0 = task_create(start, 0, -1);
  sched_add_task(t0);

  /* TODO comment and fixme */
  asm volatile("MOV R1, SP");
  asm volatile("MOV R2, LR");
  asm volatile("SUB SP, SP, #68"); /* Account for the proc state object that gets created after the first SWI call. */

  /* Initialize IRQ stack pointer to the same one used by the swi handler */
  asm volatile("MOV R3, SP"); 
  asm volatile("MSR CPSR_c, #0xD2"); 
  asm volatile("MOV SP, R3");

  asm volatile("MSR CPSR_c, #0xD0"); /* User mode with interrupts disabled */
  /* Preserve our LR and SP before calling SWI for the first time */
  asm volatile("MOV SP, R1");
  asm volatile("MOV LR, R2");

  SYSCALL(SYSCALL_INITIALIZATION);
}

/* Loader housekeeping (The redboot elf loader does not appear to zero out memory) */
/* ================================================================================= */

void zero_bss() {
  extern char _BssStart;
  extern char _BssEnd;

  bwprintln("Attempting to zero out %x - %x", &_BssStart, &_BssEnd);
  char *i = &_BssStart; 
  while(i != &_BssEnd) {
    *i = (char)0;
    i++;	
  }
}

/* ================================================================================= */

int main() {
  /* FIXME make no assumptions other than that the processor is in supervisor */
  /* mode. Initialize the kernel, make sure interrupts are enabled/disabled as */
  /* desired. */

  /* General initialzation */
  zero_bss();
  init_bwio();
  init_event();
  init_ivt();

  /* Initialize subsystems */
  init_sched(); 
  init_ipc(); 
  init_icu();
  initialize_timer(10);

  /* Machine specific optimizations */
  ts7200_enable_caches();
  ua_init();
  ua_enable_interrupts(COM2);
  ua_enable_interrupts(COM1);

  run_first_task(initTask);
  bwprintln("BACK IN MAIN, RETURNING TO REDBOOT");
  return 0;
}
