#include "stdafx.h"
#include "circularBuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

CRITICAL_SECTION tCri;

#ifndef min
#define min(a, b) ((a) > (b))?(b):(a)
#endif
#ifndef max
#define max(a, b) ((a) > (b))?(a):(b)
#endif

uint32_t roundup_pow_of_two(const uint32_t x)
{
	if (x == 0) { return 0; }
	if (x == 1) { return 2; }
	uint32_t ret = 1;
	while (ret < x)
	{
		ret = ret << 1;
	}
	return ret;
}

bool is_FIFO_empty(const struct FIFO *FIFO)
{
	return (FIFO->in == FIFO->out);
}

void FIFO_release(struct FIFO *FIFO)
{
	if (FIFO != NULL)
	{
		if (FIFO->buffer != NULL)
		{
			free(FIFO->buffer);
			FIFO->buffer = NULL;
		}
	}
	free(FIFO);
	DeleteCriticalSection(&tCri);

	return;
}

struct FIFO *FIFO_alloc(unsigned int size)
{
	InitializeCriticalSection(&tCri);
	struct FIFO *ret = (struct FIFO*)malloc(sizeof(struct FIFO));

	if (size & (size - 1))
	{
		size = roundup_pow_of_two(size);
	}

	ret->buffer = (unsigned char*)malloc(size);
	ret->size = size;

	if (!ret->buffer)
	{
		return NULL;
	}

	memset(ret->buffer, 0, size);
	ret->in = 0;
	ret->out = 0;
	ret->memUsed = 0;

	return ret;
}

unsigned int FIFO_put(struct FIFO *FIFO, unsigned char* buffer, unsigned int len)
{
	unsigned int l;

	EnterCriticalSection(&tCri);
	len = min(len, FIFO->size - FIFO->in + FIFO->out);
	l = min(len, FIFO->size - (FIFO->in & (FIFO->size - 1)));
	memcpy(FIFO->buffer + (FIFO->in & (FIFO->size - 1)), buffer, l);
	/* then put the rest (if any) at the beginning of the buffer */
	memcpy(FIFO->buffer, buffer + l, len - l);
	/*
	* Ensure that we add the bytes to the kfifo -before-
	* we update the fifo->in index.
	*/
	FIFO->in += len;
	FIFO->memUsed += len;
	LeaveCriticalSection(&tCri);
	return len;
}

unsigned int FIFO_get(struct FIFO *FIFO, unsigned char *buffer, unsigned int len)
{
	unsigned int l;

	/*
	* Ensure that we sample the fifo->in index -before- we
	* start removing bytes from the kfifo.
	*/
	EnterCriticalSection(&tCri);
	len = min(len, FIFO->in - FIFO->out);
	/* first get the data from fifo->out until the end of the buffer */
	l = min(len, FIFO->size - (FIFO->out & (FIFO->size - 1)));
	memcpy(buffer, FIFO->buffer + (FIFO->out & (FIFO->size - 1)), l);
	/* then get the rest (if any) from the beginning of the buffer */
	memcpy(buffer + l, FIFO->buffer, len - l);
	/*
	* Ensure that we remove the bytes from the kfifo -before-
	* we update the fifo->out index.
	*/
	FIFO->out += len;
	FIFO->memUsed -= len;
	LeaveCriticalSection(&tCri);
	return len;
}

void printMemUsed(struct FIFO *FIFO)
{
	printf("estimated mem used in FIFO : %d, is overwrite %s\n", FIFO->memUsed, (FIFO->memUsed > FIFO->size)?"yes":"no");
}