#ifndef _COMMON__ALG_H_
#define _COMMON__ALG_H_

#include "common/byte_buffer.h"

int strncpy(char *src, char *dst, int n);
int memcpy_unsafe_nonaligned(char *src, char *dst, int n);

int strcmp(char *str1, char *str2);
int memcpy(char *msg, int msglen, char *reply, int rplen);
int clz(unsigned int x, char clz_lookup_map[]);

char c2x( char ch );
int a2d( char ch );
char a2i( char ch, char **src, int base, int *nump );
void ui2a( unsigned int num, unsigned int base, char *bf );
void i2a( int num, char *bf );

int memzero(char *buf, int bytes);
int parse_next_token(struct ByteBuffer *readbuf, char *buf);
int str2uint(char *str, unsigned int *result);

void enable_debug_timer();
inline int read_debug_timer();

void INTERRUPTS_OFF();
void INTERRUPTS_ON();

#endif

