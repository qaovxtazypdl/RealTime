#include "common/algs.h"
#include "emul/ts7200.h"

// assume src is null terminated,
// assume dst is a general buffer, n is max amount to copy
int strncpy(char *src, char *dst, int n) {
  int bytes_copied = 0;
  while (bytes_copied < (n - 1) && *src) {
    dst[bytes_copied] = src[bytes_copied];
    bytes_copied++;
  }
  dst[bytes_copied] = '\0';
  return bytes_copied + 1;
}

int memcpy_unsafe_nonaligned(char *src, char *dst, int n) {
  int bytes_copied = 0;
  while (bytes_copied < n) {
    dst[bytes_copied] = src[bytes_copied];
    bytes_copied++;
  }
  return bytes_copied;
}

int strcmp(char *str1, char *str2) {
  while (*str1 && *str2 && *str1 == *str2) {
    ++str1;
    ++str2;
  }
  return (int) (*str1 - *str2);
}

int memcpy(char *msg, int msglen, char *reply, int rplen) {
  int bytes_copied = 0, bytes_to_copy;
  if (msglen < rplen) {
    bytes_to_copy = msglen;
  } else {
    bytes_to_copy = rplen;
  }

  while (bytes_to_copy >= 4) {
    *((int*)(reply + bytes_copied)) = *((int*)(msg + bytes_copied));
    bytes_to_copy -= 4;
    bytes_copied += 4;
  }

  while (bytes_to_copy > 0) {
    reply[bytes_copied] = msg[bytes_copied];
    bytes_to_copy--;
    bytes_copied++;
  }

  return bytes_copied;
}

int clz(unsigned int x, char clz_lookup_map[]) {
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  x++;
  return clz_lookup_map[x * 0x076be629 >> 27];
}

char c2x( char ch ) {
  if ( (ch <= 9) ) return '0' + ch;
  return 'a' + ch - 10;
}

int a2d( char ch ) {
  if( ch >= '0' && ch <= '9' ) return ch - '0';
  if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
  if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
  return -1;
}

char a2i( char ch, char **src, int base, int *nump ) {
  int num, digit;
  char *p;

  p = *src; num = 0;
  while( ( digit = a2d( ch ) ) >= 0 ) {
    if ( digit > base ) break;
    num = num*base + digit;
    ch = *p++;
  }
  *src = p; *nump = num;
  return ch;
}

void ui2a( unsigned int num, unsigned int base, char *bf ) {
  int n = 0;
  int dgt;
  unsigned int d = 1;

  while( (num / d) >= base ) d *= base;
  while( d != 0 ) {
    dgt = num / d;
    num %= d;
    d /= base;
    if( n || dgt > 0 || d == 0 ) {
      *bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
      ++n;
    }
  }
  *bf = 0;
}

void i2a( int num, char *bf ) {
  if( num < 0 ) {
    num = -num;
    *bf++ = '-';
  }
  ui2a( num, 10, bf );
}

void enable_debug_timer() {
  volatile int *enabler = (int *) TIMER4_ENABLE;
  *enabler &= ~(0x10);
  *enabler |= 0x10;
}

inline int read_debug_timer() {
  volatile int *low_word = (volatile int *) TIMER4_LOW;
  return *low_word;
}











int memzero(char *buf, int bytes) {
  int i;
  for (i = 0; i < bytes; i++) {
    buf[i] = 0;
  }
  return 0;
}

// buffer should be "large enough"
int parse_next_token(struct ByteBuffer *readbuf, char *buf) {
  char next_char = 0;

  // get rid of leading whitespace
  while (
    !is_byte_buffer_empty(readbuf) && (next_char = byte_buffer_pop(readbuf)) && (
      next_char == ' ' ||
      next_char == '\t' ||
      next_char == '\0'
    )
  );

  *buf = next_char;
  *buf++;

  while(
    !is_byte_buffer_empty(readbuf) && (next_char = byte_buffer_pop(readbuf)) && !(
      next_char == ' ' ||
      next_char == '\t' ||
      next_char == '\0'
    )
  ) {
    *buf = next_char;
    *buf++;
  }
  *buf = '\0';
  return 0;
}


// parses unsigned integers only
// returns -1 if error, 0 if success
int str2uint(char *str, unsigned int *result) {
  char ch;
  *result = 0;
  while (str && (ch = *str)) {
    if (ch >= '0' && ch <= '9') {
      *result = (*result * 10) + (ch - '0');
    } else {
      return -1;
    }
    str++;
  }
  return 0;
}


void INTERRUPTS_OFF() {
  int *irq_clear_1 = (int *) (VIC1_BASE + VIC_INT_CLEAR);
  int *irq_clear_2 = (int *) (VIC2_BASE + VIC_INT_CLEAR);
  *irq_clear_1 = 0xffffffff;
  *irq_clear_2 = 0xffffffff;
}

void INTERRUPTS_ON() {
  int *irq_enable_1 = (int *) (VIC1_BASE + VIC_INT_ENABLE);
  int *irq_enable_2 = (int *) (VIC2_BASE + VIC_INT_ENABLE);
  *irq_enable_1 = 0x0;
  *irq_enable_2 =
    VIC2_TIMER3_INTERRUPT |
    VIC2_UART1_INTERRUPT |
    VIC2_UART2_INTERRUPT;
}
