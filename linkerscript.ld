ENTRY (main)

MEMORY
{
ram : ORIGIN = 0x00218000, LENGTH = 32M
rom : ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{
.data : /* Initialized data. */
{
_DataStart = . ;
*(.data)
_DataEnd = . ;
} >ram

.bss : /* Uninitialized data. */
{ 
_BssStart = . ;
*(.bss) 
_BssEnd = . ;
} >ram


.text : /* The actual instructions. */
{
*(.text)
*(.got)
*(.got.plt)
*(.rodata)
*(.data.rel.local)
*(.rodata.str1.4)
*(.glue_7)
*(.glue_7t)
} >ram

stack_top = .; /* Needs to be defined for startup.s, not used unless code is being compiled for the emulator */
}
