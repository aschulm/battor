#ifndef RINGBUF_H
#define RINGBUF_H

typedef uint32_t (*memcpy32_ptr)(uint32_t, const uint32_t, uint32_t);

typedef struct ringbuf_
{
	uint32_t read_idx;
	uint32_t write_idx;
	uint32_t len;
	memcpy32_ptr read;
	memcpy32_ptr write;
	uint32_t base;
	uint32_t base_len;
} ringbuf;

uint32_t memcpy32(uint32_t dst, const uint32_t src, uint32_t len);

int ringbuf_init(ringbuf* rb, uint32_t user, uint32_t base_len,
	memcpy32_ptr write, memcpy32_ptr read);
int ringbuf_write(ringbuf* rb, void* src, uint16_t len);
int ringbuf_read(ringbuf* rb, void* dst, uint16_t len);
int ringbuf_self_test();

#endif
