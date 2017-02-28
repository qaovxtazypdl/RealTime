ENTRY(_Reset)
SECTIONS
{
. = 0x10000;
_test = .;
.startup . : { src/emul/startup.o(.text) }
.text : { *(.text) }
.data : { *(.data) }
.bss : { 
	_BssStart = ABSOLUTE(.) ;
	*(.bss ) 
	_BssEnd = ABSOLUTE(.) ;
}
. = ALIGN(8);
. = . + 0x1000000; /* 4kB of stack memory */
stack_top = .;
}
