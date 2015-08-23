#ifndef RINGBUF_H
#define RINGBUF_H

typedef void* (*memcpy_ptr)(void*, const void*, size_t);

typedef struct ringbuf_
{
	uint16_t read_idx;
	uint16_t write_idx;
	uint16_t len;
	memcpy_ptr read;
	memcpy_ptr write;
	void* base;
	uint16_t base_len;
} ringbuf;

int ringbuf_init(ringbuf* rb, void* user, uint16_t base_len,
	memcpy_ptr write, memcpy_ptr read);
int ringbuf_write(ringbuf* rb, void* src, uint16_t len);
int ringbuf_read(ringbuf* rb, void* dst, uint16_t len);
int ringbuf_self_test();

#endif
