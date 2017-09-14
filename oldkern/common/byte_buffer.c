#include "common/byte_buffer.h"

void init_byte_buffer(struct ByteBuffer *buf, char *internal_buffer, int len) {
  int i = 0;
  buf->buffer = internal_buffer;
  buf->len = len;
  buf->head = 0;
  buf->tail = 0;
  for (i = 0; i < buf->len; i++) {
    buf->buffer[i] = 0;
  }
}
int byte_buffer_clear(struct ByteBuffer *buf) {
  return buf->head = buf->tail = 0;
}
int byte_buffer_count(struct ByteBuffer *buf) {
  return (buf->len + buf->tail - buf->head) % buf->len;
}
int is_byte_buffer_empty(struct ByteBuffer *buf) {
  return buf->head == buf->tail;
}
int is_byte_buffer_full(struct ByteBuffer *buf) {
  return (buf->tail + 1) % buf->len == buf->head;
}
// returns 0 if success
int byte_buffer_push(struct ByteBuffer *buf, char c) {
  if (is_byte_buffer_full(buf)) return -1;
  else {
    buf->buffer[buf->tail] = c;
    buf->tail = (buf->tail + 1) % buf->len;
    return 0;
  }
}
// returns 0 if success
int byte_buffer_circular_push(struct ByteBuffer *buf, char c) {
  if (is_byte_buffer_full(buf)) {
    buf->head = (buf->head + 1) % buf->len;
  }
  buf->buffer[buf->tail] = c;
  buf->tail = (buf->tail + 1) % buf->len;
  return 0;
}
// returns 0 if fail
char byte_buffer_pop(struct ByteBuffer *buf) {
  char result;
  if (is_byte_buffer_empty(buf)) return 0;
  else {
    result = buf->buffer[buf->head];
    buf->head = (buf->head + 1) % buf->len;
    return result;
  }
}
// returns 0 if fail
char byte_buffer_pop_end(struct ByteBuffer *buf) {
  char result;
  if (is_byte_buffer_empty(buf)) return 0;
  else {
    unsigned int result_index = (buf->tail + buf->len - 1) % buf->len;
    result = buf->buffer[result_index];
    buf->tail = result_index;
    return result;
  }
}
char byte_buffer_peek(struct ByteBuffer *buf) {
  if (is_byte_buffer_empty(buf)) return 0;
  else return buf->buffer[buf->head];
}
