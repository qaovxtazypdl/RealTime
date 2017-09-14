#include "tasks/name_server.h"
#include "bwio/bwio.h"
#include "common/algs.h"
#include "common/byte_buffer.h"
#include "kernel/syscall.h"

#define RPS_SERVER_NS_NAME "RPS Server"
// 8 copies of task at higher prio than server.

/*
tasks sign up
AD (1 player lower prio, 1 player higher prio) - simple test
ABAB (players all lower prio than server - send block) - tests reqeueing of signed up clients
CBBBBBA (players all higher prio than server - receive block) - tests requeueing into the same player slot

A play play play play quit
B play quit
C play play play play play play play play play play play play play play quit



1 play play play play play play play play play play play play play play quit
2 play quit play quit play quit play quit play quit play play play play quit
*/

/*
request:
signup - 0x0
play - 0x1 (rock), 0x2 (paper), 0x3 (scissors)
quit - 0xff

return:
0xff error
signup - 0x0 if success.
play - 0x10 if win, 0x11 if lose, 0x12 if tie, 0x13 if other player quit
quit - 0x0 if success
*/

#define SIGNUP 0xff
/* do not change these*/
#define QUIT 0x0
#define ROCK 0x1
#define PAPER 0x2
#define SCISSORS 0x3
/* do not change these*/

#define WIN 0x10
#define LOSS 0x11
#define TIE 0x12
#define RAGEQUIT 0x13


int choose_rps_action() {
  return (read_debug_timer() % 3) + ROCK;
}

void task__rps_server(){
  int retval = -1;
  retval = RegisterAs(RPS_SERVER_NS_NAME);

  // rock paper scissors quit
  char *action_to_name[4];
  action_to_name[QUIT] = "QUIT";
  action_to_name[ROCK] = "ROCK";
  action_to_name[PAPER] = "PAPER";
  action_to_name[SCISSORS] = "SCISSORS";

  char signup_buffer[32];
  struct ByteBuffer signup_queue;
  init_byte_buffer(&signup_queue, signup_buffer, 32);

  char rcvbuf = -1, rplbuf = -1;
  int received_tid = -1;

  int player_a_tid = -1, player_b_tid = -1;
  int player_a_choice = -1, player_b_choice = -1;

  char player_a_response, player_b_response;
  int reply_tid_a = -1, reply_tid_b = -1;
  for (;;) {
    dbwprintf(COM2, "RPS Server - Receiving (waiting for send)... \n\r");
    retval = Receive(&received_tid, &rcvbuf, 1);
    dbwprintf(COM2, "RPS Server - Received command from tid %d: %d\n\r", received_tid, rcvbuf);

    if (retval == 1) {
      if (rcvbuf == SIGNUP) {
        if (player_a_tid >= 0 && player_b_tid >= 0) {
          // if game slots full, queue
          if (is_byte_buffer_full(&signup_queue)) {
            rplbuf = 0xff;
            bwprintf(COM2, "RPS Server - Signup Queue full: %d\n\r", retval);
          } else {
            bwprintf(COM2, "RPS Server - Placed tid on signup queue: %d\n\r", received_tid);
            byte_buffer_push(&signup_queue, received_tid);
          }
        } else if (player_a_tid < 0) {
          bwprintf(COM2, "RPS Server - signed up: %d\n\r", received_tid);
          player_a_tid = received_tid;
          rplbuf = 0;
          retval = Reply(received_tid, &rplbuf, 1);
          if (retval != 1) {
            bwprintf(COM2, "RPS Server - Reply failure: %d\n\r", retval);
          }
        } else {
          bwprintf(COM2, "RPS Server - signed up: %d\n\r", received_tid);
          player_b_tid = received_tid;
          rplbuf = 0;
          retval = Reply(received_tid, &rplbuf, 1);
          if (retval != 1) {
            bwprintf(COM2, "RPS Server - Reply failure: %d\n\r", retval);
          }
        }
      } else if (rcvbuf == ROCK || rcvbuf == PAPER || rcvbuf == SCISSORS || rcvbuf == QUIT) {
        dbwprintf(COM2, "RPS Server - play/quit processed from: %d\n\r", received_tid);
        if (player_a_tid == received_tid) {
          player_a_choice = rcvbuf;
        } else if (player_b_tid == received_tid) {
          player_b_choice = rcvbuf;
        } else {
          rplbuf = 0xff;
          retval = Reply(received_tid, &rplbuf, 1);
          if (retval != 1) {
            bwprintf(COM2, "RPS Server - Reply failure: %d\n\r", retval);
          }
          bwprintf(COM2, "RPS Server - Invalid tid trying to play: %d\n\r", retval);
        }
      }
    } else {
      rplbuf = 0xff;
      retval = Reply(received_tid, &rplbuf, 1);
      if (retval != 1) {
        bwprintf(COM2, "RPS Server - Reply failure: %d\n\r", retval);
      }
      bwprintf(COM2, "RPS Server - Receive failure: %d\n\r", retval);
    }

    if (player_a_tid >= 0 && player_b_tid >= 0 && player_a_choice >= 0 && player_b_choice >= 0) {
      // ready to handle rps game
      dbwprintf(COM2, "RPS Server - Playing game between: %d and %d\n\r", player_a_tid, player_b_tid);
      bwprintf(
        COM2, "RPS Server - %d plays %s, while %d plays %s\n\r",
        player_a_tid, action_to_name[player_a_choice], player_b_tid, action_to_name[player_b_choice]
      );

      reply_tid_a = player_a_tid;
      reply_tid_b = player_b_tid;

      if (player_a_choice == QUIT || player_b_choice == QUIT) {
        if (player_a_choice == QUIT && player_b_choice == QUIT) {
          player_a_response = 0;
          player_b_response = 0;
          player_a_tid = -1;
          player_b_tid = -1;
          bwprintf(COM2, "RPS Server - Both players ragequit!\n\r");
        } else if (player_a_choice == QUIT) {
          player_a_response = 0;
          player_b_response = RAGEQUIT;
          bwprintf(COM2, "RPS Server - Player %d ragequit!\n\r", player_a_tid);
          player_a_tid = -1;
        } else if (player_b_choice == QUIT) {
          player_a_response = RAGEQUIT;
          player_b_response = 0;
          bwprintf(COM2, "RPS Server - Player %d ragequit!\n\r", player_b_tid);
          player_b_tid = -1;
        }
      } else {
        // 0 if tie, 1 if p1 win, 2 if p2 win.
        int result = (3 + player_a_choice - player_b_choice) % 3;
        if (result == 0) {
          player_a_response = TIE;
          player_b_response = TIE;
          bwprintf(COM2, "RPS Server - It was a tie!\n\r");
        } else if (result == 1) {
          player_a_response = WIN;
          player_b_response = LOSS;
          bwprintf(COM2, "RPS Server - Player %d won!\n\r", player_a_tid);
        } else if (result == 2) {
          player_a_response = LOSS;
          player_b_response = WIN;
          bwprintf(COM2, "RPS Server - Player %d won!\n\r", player_b_tid);
        }
      }

      bwprintf(COM2, "\n\rPress any key to continue...\n\r\n\r");
      bwgetc(COM2);

      retval = Reply(reply_tid_a, &player_a_response, 1);
      if (retval != 1) {
        bwprintf(COM2, "RPS Server - Reply to A failure: %d\n\r", retval);
      }

      retval = Reply(reply_tid_b, &player_b_response, 1);
      if (retval != 1) {
        bwprintf(COM2, "RPS Server - Reply to B failure: %d\n\r", retval);
      }

      player_a_choice = -1;
      player_b_choice = -1;

      if (player_a_tid < 0 && !is_byte_buffer_empty(&signup_queue)) {
        player_a_tid = byte_buffer_pop(&signup_queue);
        dbwprintf(COM2, "RPS Server - Replying to signup on queue for: %d\n\r", player_a_tid);

        rplbuf = 0;
        retval = Reply(player_a_tid, &rplbuf, 1);
        if (retval != 1) {
          bwprintf(COM2, "RPS Server - Reply failure: %d\n\r", retval);
        }
      }

      if (player_b_tid < 0 && !is_byte_buffer_empty(&signup_queue)) {
        player_b_tid = byte_buffer_pop(&signup_queue);
        dbwprintf(COM2, "RPS Server - Replying to signup on queue for: %d\n\r", player_b_tid);

        rplbuf = 0;
        retval = Reply(player_b_tid, &rplbuf, 1);
        if (retval != 1) {
          bwprintf(COM2, "RPS Server - Reply failure: %d\n\r", retval);
        }
      }
    }
  }

  Exit();
}

void task__rps_player_a(){
  int rps_server_tid = -1;
  int my_tid = -1;
  int rcvlen = -1;
  int i = 0;
  char msgbuf, replybuf;

  // rock paper scissors quit
  char *action_to_name[4];
  action_to_name[QUIT] = "QUIT";
  action_to_name[ROCK] = "ROCK";
  action_to_name[PAPER] = "PAPER";
  action_to_name[SCISSORS] = "SCISSORS";

  my_tid = MyTid();
  rps_server_tid = WhoIs(RPS_SERVER_NS_NAME);

  bwprintf(COM2, "%d - Signing up\n\r", my_tid);
  msgbuf = SIGNUP;
  rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
  if (rcvlen == 1 && replybuf == 0) {
    dbwprintf(COM2, "%d - Sign up success\n\r", my_tid);
  } else {
    bwprintf(COM2, "%d - Sign up failure: %d\n\r", my_tid, rcvlen);
    Exit();
  }

  for (i = 0; i < 4; i++) {
    msgbuf = (char) choose_rps_action();
    bwprintf(COM2, "%d - Playing: %s\n\r", my_tid, action_to_name[(int) msgbuf]);
    rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
    if (rcvlen == 1) {
      if (replybuf == WIN) {
        bwprintf(COM2, "%d - WON!\n\r", my_tid);
      } else if (replybuf == LOSS) {
        bwprintf(COM2, "%d - LOST!\n\r", my_tid);
      } else if (replybuf == TIE) {
        bwprintf(COM2, "%d - TIED!\n\r", my_tid);
      } else if (replybuf == RAGEQUIT) {
        bwprintf(COM2, "%d - OTHER PLAYER RAGEQUIT!\n\r", my_tid);
      }
    } else {
      bwprintf(COM2, "%d - Play internal failure: %d\n\r", my_tid, rcvlen);
      Exit();
    }
  }

  bwprintf(COM2, "%d - Quitting\n\r", my_tid);
  msgbuf = QUIT;
  rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
  if (rcvlen == 1 && replybuf == 0) {
    bwprintf(COM2, "%d - Quit success\n\r", my_tid);
  } else {
    bwprintf(COM2, "%d - Quit failure: %d\n\r", my_tid, rcvlen);
  }

  bwprintf(COM2, "%d - Exiting cleanly.\n\r", my_tid);
  Exit();
}

void task__rps_player_b(){
  int rps_server_tid = -1;
  int my_tid = -1;
  int rcvlen = -1;
  int i = 0;
  char msgbuf, replybuf;

  // rock paper scissors quit
  char *action_to_name[4];
  action_to_name[QUIT] = "QUIT";
  action_to_name[ROCK] = "ROCK";
  action_to_name[PAPER] = "PAPER";
  action_to_name[SCISSORS] = "SCISSORS";

  my_tid = MyTid();
  rps_server_tid = WhoIs(RPS_SERVER_NS_NAME);

  bwprintf(COM2, "%d - Signing up\n\r", my_tid);
  msgbuf = SIGNUP;
  rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
  if (rcvlen == 1 && replybuf == 0) {
    dbwprintf(COM2, "%d - Sign up success\n\r", my_tid);
  } else {
    bwprintf(COM2, "%d - Sign up failure: %d\n\r", my_tid, rcvlen);
    Exit();
  }

  for (i = 0; i < 1; i++) {
    msgbuf = (char) choose_rps_action();
    bwprintf(COM2, "%d - Playing: %s\n\r", my_tid, action_to_name[(int) msgbuf]);
    rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
    if (rcvlen == 1) {
      if (replybuf == WIN) {
        bwprintf(COM2, "%d - WON!\n\r", my_tid);
      } else if (replybuf == LOSS) {
        bwprintf(COM2, "%d - LOST!\n\r", my_tid);
      } else if (replybuf == TIE) {
        bwprintf(COM2, "%d - TIED!\n\r", my_tid);
      } else if (replybuf == RAGEQUIT) {
        bwprintf(COM2, "%d - OTHER PLAYER RAGEQUIT!\n\r", my_tid);
      }
    } else {
      bwprintf(COM2, "%d - Play internal failure: %d\n\r", my_tid, rcvlen);
      Exit();
    }
  }

  bwprintf(COM2, "%d - Quitting\n\r", my_tid);
  msgbuf = QUIT;
  rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
  if (rcvlen == 1 && replybuf == 0) {
    bwprintf(COM2, "%d - Quit success\n\r", my_tid);
  } else {
    bwprintf(COM2, "%d - Quit failure: %d\n\r", my_tid, rcvlen);
  }

  bwprintf(COM2, "%d - Exiting cleanly.\n\r", my_tid);
  Exit();
}

void task__rps_player_c(){
  int rps_server_tid = -1;
  int my_tid = -1;
  int rcvlen = -1;
  int i = 0;
  char msgbuf, replybuf;

  // rock paper scissors quit
  char *action_to_name[4];
  action_to_name[QUIT] = "QUIT";
  action_to_name[ROCK] = "ROCK";
  action_to_name[PAPER] = "PAPER";
  action_to_name[SCISSORS] = "SCISSORS";

  my_tid = MyTid();
  rps_server_tid = WhoIs(RPS_SERVER_NS_NAME);

  bwprintf(COM2, "%d - Signing up\n\r", my_tid);
  msgbuf = SIGNUP;
  rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
  if (rcvlen == 1 && replybuf == 0) {
    dbwprintf(COM2, "%d - Sign up success\n\r", my_tid);
  } else {
    bwprintf(COM2, "%d - Sign up failure: %d\n\r", my_tid, rcvlen);
    Exit();
  }

  for (i = 0; i < 14; i++) {
    msgbuf = (char) choose_rps_action();
    bwprintf(COM2, "%d - Playing: %s\n\r", my_tid, action_to_name[(int) msgbuf]);
    rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
    if (rcvlen == 1) {
      if (replybuf == WIN) {
        bwprintf(COM2, "%d - WON!\n\r", my_tid);
      } else if (replybuf == LOSS) {
        bwprintf(COM2, "%d - LOST!\n\r", my_tid);
      } else if (replybuf == TIE) {
        bwprintf(COM2, "%d - TIED!\n\r", my_tid);
      } else if (replybuf == RAGEQUIT) {
        bwprintf(COM2, "%d - OTHER PLAYER RAGEQUIT!\n\r", my_tid);
      }
    } else {
      bwprintf(COM2, "%d - Play internal failure: %d\n\r", my_tid, rcvlen);
      Exit();
    }
  }

  bwprintf(COM2, "%d - Quitting\n\r", my_tid);
  msgbuf = QUIT;
  rcvlen = Send(rps_server_tid, &msgbuf, 1, &replybuf, 1);
  if (rcvlen == 1 && replybuf == 0) {
    bwprintf(COM2, "%d - Quit success\n\r", my_tid);
  } else {
    bwprintf(COM2, "%d - Quit failure: %d\n\r", my_tid, rcvlen);
  }

  bwprintf(COM2, "%d - Exiting cleanly.\n\r", my_tid);
  Exit();
}
// runs at prio 2
void task__rps_init_round_1() {
  Create(10, &task__rps_player_a);
  Create(20, &task__rps_player_a);
  Exit();
}

// runs at prio 2
void task__rps_init_round_2() {
  Create(20, &task__rps_player_a);
  Create(20, &task__rps_player_b);
  Create(20, &task__rps_player_a);
  Create(20, &task__rps_player_b);
  Exit();
}

// runs at prio 2
void task__rps_init_round_3() {
  Create(10, &task__rps_player_c);
  Create(10, &task__rps_player_b);
  Create(10, &task__rps_player_b);
  Create(10, &task__rps_player_b);
  Create(10, &task__rps_player_b);
  Create(10, &task__rps_player_b);
  Create(10, &task__rps_player_a);
  Exit();
}

// runs at lowest prio.
void task__rps_init() {
  bwprintf(COM2, "Initializing server...\n\r");
  Create(15, &task__rps_server);
  bwprintf(COM2, "\n\r\n\r============== Round 1 starting ==============\n\r");
  Create(2, &task__rps_init_round_1);
  bwprintf(COM2, "\n\r\n\r============== Round 2 starting ==============\n\r");
  Create(2, &task__rps_init_round_2);
  bwprintf(COM2, "\n\r\n\r============== Round 3 starting ==============\n\r");
  Create(2, &task__rps_init_round_3);
  Exit();
}

