struct clsv_pending_task {
  int time;
  int td;
};

struct clsv_msg {
  enum clsv_msg_op {
    CLOCK_SERVER_DELAY_REQUEST,
    CLOCK_SERVER_DELAY_UNTIL_REQUEST,
    CLOCK_SERVER_GET_TIME_REQUEST
  } op;
  int delay; 
  int *time;
};
