void printf(char *fmt, ...) {
        va_list va;

        va_start(va,fmt);
        bwformat( channel, fmt, va );
        va_end(va);
}



static void putw(int n, char fc, char *bf) {
	char ch;
	char *p = bf;

	while( *p++ && n > 0 ) n--;
	while( n-- > 0 ) putchar(fc);
	while( ( ch = *bf++ ) ) putchar(ch);
}

static void format ( int channel, char *fmt, va_list va ) {
	char bf[12];
	char ch, lz;
	int w;

	
	while ( ( ch = *(fmt++) ) ) {
		if ( ch != '%' )
			putc( channel, ch );
		else {
			lz = 0; w = 0;
			ch = *(fmt++);
			switch ( ch ) {
			case '0':
				lz = 1; ch = *(fmt++);
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				ch = bwa2i( ch, &fmt, 10, &w );
				break;
			}
			switch( ch ) {
			case 0: return;
			case 'c':
				putchar(va_arg( va, char ));
				break;
			case 's':
				putw(w, 0, va_arg( va, char* ) );
				break;
			case 'u':
				bwui2a(va_arg( va, unsigned int ), 10, bf );
				putw(w, lz, bf);
				break;
			case 'd':
				bwi2a( va_arg( va, int ), bf );
				putw(w, lz, bf);
				break;
			case 'x':
				bwui2a( va_arg( va, unsigned int ), 16, bf );
				putw(w, lz, bf);
				break;
			case '%':
				putchar( channel, ch );
				break;
			}
		}
	}
}
