#ifndef _BWIO_H_
#define _BWIO_H_

/*
 * bwio.h
 */
#include "arg.h"

#define COM1 0
#define COM2 1
#define ON	1
#define	OFF	0

int bwsetfifo( int channel, int state );

int bwsetspeed( int channel, int speed );

int bwputc( int channel, char c );

int bwgetc( int channel );

int bwputx( int channel, char c );

int bwputstr( int channel, char *str );

int bwputr( int channel, unsigned int reg );

void bwputw( int channel, int n, char fc, char *bf );

void bwprintf( int channel, char *format, ... );

void bwui2a( unsigned int num, unsigned int base, char *bf );

void bwi2a( int num, char *bf );

#define bwprintln(str, ...) bwprintf(COM2, str"\n\r", ##__VA_ARGS__)


#endif
