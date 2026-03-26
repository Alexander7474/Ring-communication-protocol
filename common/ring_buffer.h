#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "config.h"

struct ring_buffer{
        int socket[WAITING_HOST_MAX];
        int read_head;
        int write_head;
};

/* Push a socket fd into the ring buffer. Returns 0 or -1 if full. */
int push_rg_buff(struct ring_buffer* buffer, int i);

/* Removes and returns value in *i. Returns 0 on success, -1 if empty */
int pop_rg_buff(struct ring_buffer* buffer, int* i);

/* Peeks at next value in *i without consuming it. Returns 0 on success, -1 if empty */
int read_rg_buff(struct ring_buffer* buffer, int* i);

/* Returns 1 if empty, 0 if not */
int is_rg_buff_empty(struct ring_buffer* buffer);

#endif // !RING_BUFFER_H
