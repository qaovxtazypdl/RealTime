#include <io.h>
#include <common/bwio.h>
#include <common/syscall.h>
#include <common/uart.h>
#include <console.h>
#include <clock_server.h>
#include <clock_printer.h>
#include <idle_task.h>
#include <ns.h>
#include <train.h>
#include <sensors.h>

void initTask() {
  create(31, idle);
  init_name_server();
  init_clock_server();
  init_io();
  

  create(1, clock_printer);
  create(1, sensor_printer);
  create(1, console_task);
}
