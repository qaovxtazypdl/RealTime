#include <common/arg.h>
#include <common/string.h>
#include <io.h>

#define MAX_STR_SZ 1024

/* Pilfered from bwio */
static int ui2a( char *buf, int mw, int num) {
  int n = 0, i;
  int dgt;
  int d = 1;
  int sz = 1;

  while( (num / d) >= 10 ) { 
    sz++;
    d *= 10;
  }

  if((mw - sz) > 0)
    for (i = 0; i < (mw - sz); i++)
     *buf++ = '0'; 

  while( d != 0 ) {
    dgt = num / d;
    num %= d;
    d /= 10;
    if( n || dgt > 0 || d == 0 ) {
      *buf++ = (char)(dgt + ( dgt < 10 ? '0' : 'a' - 10 ));
      ++n;
    }
  }
  if(!n) *buf++ = '0';

  return (sz < mw) ? mw : sz;
}

/* Sugar spice and everything nice...*/

static int i2xa(int c, char *buf) {
	int i;
	char b;
	int sz = 0;
  int seen = 0;
  int n = sizeof(int) * 2;

	for(i = 0;i < n;i++) {
    b = (0xF & (c >> ((n - i - 1) * 4)));
    if(b) seen++;
		b = (char)((b > 9) ? ('A' + (b - 10)) : ('0' + b));
		if(seen) {
      sz++;
      *buf++ = b;
    }
	}

  if(sz == 0)
    *buf = '0';
			
	return sz;
}

static int _sprintf(char *buf, char *fmt, va_list ap) {
	char *tail = buf;
	int esc = 0; 

	int *pos;
	char *c = fmt;
	int w;

	while(*c) {
		if(esc) {
			if(*c <= '9' && *c >= '0') {
				w *= 10;
				w += *c & 0xF;
        c++;
				continue;
			}

			esc--;
			switch(*c) {
				case 'x':
					tail += i2xa(va_arg(ap, int), tail);
					break;
				case 'c':
					*tail++ = va_arg(ap, char);
					break;
				case 'd':
					tail += ui2a(tail, w, va_arg(ap, int));
					break;
				case 's':
					tail += strcpy(tail, va_arg(ap, char*));
					break;
				/* non-standard: moves to the line described by the x,y array */
				case 'm': 
					pos = va_arg(ap, int*);
					*tail++ = 27;
					*tail++ = '[';
					tail += ui2a(tail, 0, pos[1]);
					*tail++ = ';';
					tail += ui2a(tail, 0, pos[0]);
					*tail++ = 'H';
					break;
			}
		} else if (*c == '%') {
			w = 0;
			esc++;
		}
		else {
			*tail++ = *c;
		}

		c++;
	}

	*tail = '\0';

	return tail - buf;
}

/* OPT using a global variable and doing this in the IO server might be cheaper with caching, 
   not sure if it's worth doing at this point. */

/* Expensive, prefer putstr when possible, assumes formatted string is less than 1024 characters */

void printf(int channel, char *fmt, ...) {
	char buf[MAX_STR_SZ];
  va_list ap;
  va_start(ap, fmt);

  _sprintf(buf, fmt, ap);
  putstr(channel, buf);
}

int sprintf(char *buf, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  return _sprintf(buf, fmt, ap);
}
