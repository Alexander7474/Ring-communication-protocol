#include <errno.h>

#include "ring_buffer.h"
#include "config.h"

/**
 * rg_buff_set - Met in ring_buffer à zero
 * @buffer: pointeur vers le ring_buffer
 */
void rg_buff_set(struct ring_buffer* buffer)
{
        for(unsigned int i = 0; i < WAITING_HOST_MAX; i++)
                buffer->socket[i] = -1;
        buffer->write_head = 0;
        buffer->read_head = 0;
}

/**
 * push_rg_buff - Push un socket dans un struct ring_buffer
 * @buffer: pointeur vers le struct ring_buffer
 * @i: socket file descriptor 
 *
 * Return: 0 si ok, -1 si le struct ring_buffer est plein
 */
int push_rg_buff(struct ring_buffer* buffer, int i)
{
    int next_write = (buffer->write_head + 1) % WAITING_HOST_MAX;

    if (next_write == buffer->read_head)
        return -1;

    buffer->socket[buffer->write_head] = i;
    buffer->write_head = next_write;
    return 0;
}

/**
 * pop_rg_buff - Retire un socket dans un struct ring_buffer
 * @buffer: pointeur vers le struct ring_buffer
 * @i: socket de sortie, si la function à réussie
 *
 * Return: 0 si ok, -1 si le @buffer est vide
 */
int pop_rg_buff(struct ring_buffer* buffer, int* i)
{
    if (buffer->read_head == buffer->write_head)
        return -1;

    *i = buffer->socket[buffer->read_head];
    buffer->read_head = (buffer->read_head + 1) % WAITING_HOST_MAX;
    return 0;
}

/**
 * read_rg_buff - Lit un socket dans un struct ring_buffer
 * @buffer: pointeur vers le struct ring_buffer
 * @i: socket de sortie, si la function à réussie
 *
 * Return: 0 si ok, -1 si le @buffer est vide
 */
int read_rg_buff(struct ring_buffer* buffer, int* i)
{
    if (buffer->read_head == buffer->write_head)
        return -1;

    *i = buffer->socket[buffer->read_head];
    return 0;
}

/**
 * is_rg_buffer_full - Détérmine si un ring_buffer est full
 * @buffer: pointeur vers le ring_buffer
 *
 * Return: 1 si full, 0 si non
 */
int is_rg_buff_full(struct ring_buffer* buffer)
{
        if (buffer->read_head == buffer->write_head)
                return 1;

        return 0;
}

/**
 * rg_buff_size - Cacule le nombre de socket dans un ring_buffer
 * @buffer: pointeur vers le ring_buffer
 *
 * Return: Nombre de socket(s) dans @buffer
 */
int rg_buff_size(struct ring_buffer* buffer)
{
        if(buffer->read_head > buffer->write_head)
                return buffer->write_head + (WAITING_HOST_MAX - buffer->read_head);
        return buffer->write_head - buffer->read_head;
}

