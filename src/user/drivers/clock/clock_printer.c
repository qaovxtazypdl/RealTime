#include <io.h>
#include <ns.h>
#include <clock_server.h>
#include <clock_printer.h>

#define CLOCK_LINE_NUM 3

static void inline update_clock(int32_t centi_seconds) {
	int32_t deci_seconds = centi_seconds / 10;
	int32_t seconds = centi_seconds / 100;
	int32_t minutes = seconds / 60;
	int32_t hours = minutes / 60;
	deci_seconds %= 60;
	seconds %= 60;
	minutes %= 60;
	hours %= 60;

  printf(COM2, "%s%m%s%2d:%2d:%2d.%d%s",
      SAVE_CURSOR,
      (int[]) { 0, CLOCK_LINE_NUM },
      CLEAR_LINE,
      hours,
      minutes % 60,
      seconds % 60,
      deci_seconds % 10,
      RESTORE_CURSOR);
}


void clock_printer() {
	while(1) {
		delay(10);
		update_clock(get_time());
	}
}
