#ifndef _DS__BYTE_BUFFER_H_
#define _DS__BYTE_BUFFER_H_


struct ByteBuffer {
  char *buffer;
  int len;
  int head;
  int tail;
};

void init_byte_buffer(struct ByteBuffer *buf, char *internal_buffer, int len);
int byte_buffer_clear(struct ByteBuffer *buf);
int byte_buffer_count(struct ByteBuffer *buf);
int is_byte_buffer_empty(struct ByteBuffer *buf);
int is_byte_buffer_full(struct ByteBuffer *buf);
int byte_buffer_push(struct ByteBuffer *buf, char c);
int byte_buffer_circular_push(struct ByteBuffer *buf, char c);
char byte_buffer_pop(struct ByteBuffer *buf);
char byte_buffer_pop_end(struct ByteBuffer *buf);
char byte_buffer_peek(struct ByteBuffer *buf);

#endif
