#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define SWITCH_TABLE_LINE_NUM 5
#define PROMPT_LINE (SWITCH_TABLE_LINE_NUM + 23)
#define INPUT_BUFFER_SZ 200
#define PROMPT "> "

void console_task();
void update_switch_entry(int turnout, int curved);
#endif
