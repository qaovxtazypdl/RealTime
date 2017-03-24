#ifndef _DISPATCH_H_
#define  _DISPATCH_H_

/* Non blocking send. Unlike send the td which the receiving task
   obtains will be different from the sending task, this is a
   consequence of not being implemented as a kernel primitive and may
   be subject to change. */

void dispatch(int td, void *buf, int buf_len);

#endif
