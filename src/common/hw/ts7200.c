/* Enables both the instruction and data caches. */
void ts7200_enable_caches() {
  asm volatile("MRC p15, 0, R0, c1, c0, 0");
  asm volatile("ORR R0, R0, #0x1000");
  asm volatile("ORR R0, R0, #0x4");
  asm volatile("MCR p15, 0, R0, c1, c0, 0");
}

