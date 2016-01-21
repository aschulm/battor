#include "common.h"

#include "ringbuf.h"

uint32_t memcpy32(uint32_t dst, const uint32_t src, uint32_t len)
{
	return (uint32_t)memcpy((void*)dst, (void*)src, len);
}

int ringbuf_init(ringbuf* rb, uint32_t base, uint32_t base_len, //{{{
	memcpy32_ptr write, memcpy32_ptr read)
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
	printf("ringbuf_write() len:%u ringbuf.len:%lu\n", len, rb->len);

	// check write too long
	if ((rb->len + len) > rb->base_len)
		return -1;
	
	uint16_t len_wrote = 0;

	while (len_wrote < len)
	{
		uint32_t len_free = (rb->base_len - rb->write_idx);
		uint16_t len_towrite = ((len - len_wrote) < len_free) 
			? (len - len_wrote) : len_free;

		rb->write(rb->base + rb->write_idx,
			(uint32_t)src + len_wrote,
			len_towrite);
		len_wrote += len_towrite;
		rb->write_idx += len_towrite;
		rb->len += len_towrite;

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

	uint16_t len_read = 0;
	while (len_read < len)
	{
		uint32_t len_toend = (rb->base_len - rb->read_idx);
		uint16_t len_toread = ((len - len_read) < len_toend) 
			? (len - len_read) : len_toend;

		rb->read((uint32_t)dst + len_read, rb->base + rb->read_idx, len_toread);
		len_read += len_toread;
		rb->read_idx += len_toread;
		rb->len -= len_toread;

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
	uint8_t buf[600];

	printf("ringbuf self test\n");

	printf("ringbuf write/read 1...");
	memset(ring, 0, sizeof(ring));
	memset(buf, 0, sizeof(buf));
	ringbuf_init(&rb, (uint32_t)ring, 2, &memcpy32, &memcpy32);
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
	ringbuf_init(&rb, (uint32_t)ring, 2, &memcpy32, &memcpy32);
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
	ringbuf_init(&rb, (uint32_t)ring, 2, &memcpy32, &memcpy32);
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
	ringbuf_init(&rb, (uint32_t)ring, 2, &memcpy32, &memcpy32);
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

	printf("ringbuf past end write...");
	memset(ring, 0, sizeof(ring));
	memset(buf, 0, sizeof(buf));
	ringbuf_init(&rb, (uint32_t)ring, 4, &memcpy32, &memcpy32);
	if (ringbuf_write(&rb, test_src, 1) < 0)
	{
		printf("FAILED (ringbuf_write #1)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 1) < 0)
	{
		printf("FAILED (ringbuf_read #1)\n");
		return 1;
	}
	if (ringbuf_write(&rb, test_src + 1, 4) < 0)
	{
		printf("FAILED (ringbuf_write #2)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 4) < 0)
	{
		printf("FAILED (ringbuf_read #2)\n");
		return 1;
	}
	if (memcmp(buf, test_src + 1, 4) != 0)
	{
		printf("FAILED (memcmp)\n");
		return 1;
	}
	printf("PASSED\n");

	printf("ringbuf SRAM multi write, read...");
	uint8_t test_big_src[] =
	{
		0x00, 0x01, 0x02, 0x03,
		0x10, 0x11, 0x12, 0x13,
		0x20, 0x21, 0x22, 0x23,
		0x30, 0x31, 0x32, 0x33,
		0x40, 0x41, 0x42, 0x43,
		0x50, 0x51, 0x52, 0x53
	};
	memset(buf, 0, sizeof(buf));
	ringbuf_init(&rb, 0x00000000, 100, &sram_write, &sram_read);
	if (ringbuf_write(&rb, test_big_src, 4) < 0)
	{
		printf("FAILED (ringbuf_write #1)\n");
		return 1;
	}
	if (ringbuf_write(&rb, test_big_src + 4, 2) < 0)
	{
		printf("FAILED (ringbuf_write #2)\n");
		return 1;
	}
	if (ringbuf_write(&rb, test_big_src + 6, 18) < 0)
	{
		printf("FAILED (ringbuf_write #3)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 24) < 0)
	{
		printf("FAILED (ringbuf_read)\n");
		return 1;
	}
	if (memcmp(buf, test_big_src, 24) != 0)
	{
		printf("FAILED (memcmp)\n");
		return 1;
	}

	printf("ringbuf SRAM write 600 read 512 read 88...");
	memset(buf, 0, sizeof(buf));
	ringbuf_init(&rb, 0x00000000, 600, &sram_write, &sram_read);
	if (ringbuf_write(&rb, buf, 600) < 0)
	{
		printf("FAILED (ringbuf_write #1)\n");
		return 1;
	}
	if (ringbuf_read(&rb, buf, 512) < 0)
	{
		printf("FAILED (ringbuf_read #1)\n");
		return 1;
	}
if (ringbuf_read(&rb, buf, 88) < 0)
	{
		printf("FAILED (ringbuf_read #2)\n");
		return 1;
	}
	if (rb.len != 0)
	{
		printf("FAILED (ringbuf.len expected 0 found %lu)\n", rb.len);
		return 1;
	}
	printf("PASSED\n");

	return 0;
} //}}}
