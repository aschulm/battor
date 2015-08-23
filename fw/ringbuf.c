#include "common.h"

#include "ringbuf.h"

int ringbuf_init(ringbuf* rb, void* base, uint16_t base_len, //{{{
	memcpy_ptr write, memcpy_ptr read)
{
	memset(rb, 0, sizeof(ringbuf));

	rb->base = base;
	rb->base_len = base_len;
	rb->write = write;
	rb->read = read;

	return 0;
} //}}}

int ringbuf_write(ringbuf* rb, void* src, uint16_t len) //{{{
{
	// check write too long
	if ((rb->len + len) > rb->base_len)
		return -1;
	
	while (len > 0)
	{
		uint16_t len_free = (rb->base_len - rb->write_idx);
		uint16_t len_towrite = (len < len_free) ? len : len_free;

		rb->write(rb->base + rb->write_idx, src, len_towrite);
		rb->write_idx += len_towrite;
		rb->len += len_towrite;
		len -= len_towrite;

		// reached end of buffer, wrap around
		if (rb->write_idx >= rb->base_len)
			rb->write_idx = 0;
	}

	return 0;
} //}}}

int ringbuf_read(ringbuf* rb, void* dst, uint16_t len) //{{{
{
	// check read longer than written
	if (len > rb->len)
		return -1;

	while (len > 0)
	{
		uint16_t len_toend = (rb->base_len - rb->read_idx);
		uint16_t len_toread = (len < len_toend) ? len : len_toend;

		rb->read(dst, rb->base + rb->read_idx, len_toread);
		rb->read_idx += len_toread;
		rb->len -= len_toread;
		len -= len_toread;

		// reached end of buffer, wrap around
		if (rb->read_idx >= rb->base_len)
			rb->read_idx = 0;
	}

	return 0;
} //}}}

int ringbuf_self_test() //{{{
{
	ringbuf rb;
	uint8_t ring[100];

	uint8_t test_src[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
	uint8_t buf[10];

	printf("ringbuf self test\n");

	printf("ringbuf write/read 1...");
	memset(ring, 0, sizeof(ring));
	memset(buf, 0, sizeof(buf));
	ringbuf_init(&rb, ring, 2, &memcpy, &memcpy);
	if (ringbuf_write(&rb, test_src, 1) < 0)
	{
		printf("FAILED (ringbuf_write)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 1) < 0)
	{
		printf("FAILED (ringbuf_read)\n");
		return 1;
	}
	if (memcmp(test_src, buf, 1) != 0)
	{
		printf("FAILED (memcmp)\n");
		return 1;
	}
	printf("PASSED\n");

	printf("ringbuf write/read too long...");
	memset(buf, 0, sizeof(buf));
	ringbuf_init(&rb, ring, 2, &memcpy, &memcpy);
	if (ringbuf_write(&rb, test_src, 3) >= 0)
	{
		printf("FAILED (ringbuf_write)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 3) >= 0)
	{
		printf("FAILED (ringbuf_read)\n");
		return 1;
	}
	printf("PASSED\n");

	printf("ringbuf write 2, read 2, wrap around, write 1, read 1...");
	memset(ring, 0, sizeof(ring));
	memset(buf, 0, sizeof(buf));
	ringbuf_init(&rb, ring, 2, &memcpy, &memcpy);
	if (ringbuf_write(&rb, test_src, 2) < 0)
	{
		printf("FAILED (ringbuf_write #1)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 2) < 0)
	{
		printf("FAILED (ringbuf_read #1)\n");
		return 1;
	}
	if (memcmp(buf, test_src, 2) != 0)
	{
		printf("FAILED (memcmp #1)\n");
		return 1;
	}
	if (ringbuf_write(&rb, test_src + 2, 1) < 0)
	{
		printf("FAILED (ringbuf_write #2)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 1) < 0)
	{
		printf("FAILED (ringbuf_read #2)\n");
		return 1;
	}
	if (memcmp(buf, test_src + 2, 1) != 0)
	{
		printf("FAILED (memcmp #2)\n");
		return 1;
	}
	printf("PASSED\n");

	printf("ringbuf read past write...");
	memset(ring, 0, sizeof(ring));
	memset(buf, 0, sizeof(buf));
	ringbuf_init(&rb, ring, 2, &memcpy, &memcpy);
	if (ringbuf_write(&rb, test_src, 1) < 0)
	{
		printf("FAILED (ringbuf_write)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 2) >= 0)
	{
		printf("FAILED (ringbuf_read)\n");
		return 1;
	}
	printf("PASSED\n");

	return 0;
} //}}}
