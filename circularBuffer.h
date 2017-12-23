#ifndef _CIRCULAR_BUFFER_H_
#define _CIRCULAR_BUFFER_H_

/* define a circular buffer */

#include <stdio.h>
#include <stdlib.h>

#ifndef HAVE_ARCH_BUG  
#define BUG() do { \
printf("BUG: failure at %s:%d/%s()!/n", __FILE__, __LINE__, __func__); \
panic("BUG!"); \
} while (0)
#endif  
#ifndef HAVE_ARCH_BUG_ON  
#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while(0)  
#endif  

struct FIFO
{
	unsigned int memUsed;
	unsigned char *buffer;
	unsigned int size;
	unsigned int in;
	unsigned int out;
};

struct FIFO *FIFO_alloc(unsigned int size);
unsigned int FIFO_put(struct FIFO *FIFO, unsigned char* buffer, unsigned int len);
unsigned int FIFO_get(struct FIFO *FIFO, unsigned char *buffer, unsigned int len);
bool is_FIFO_empty(const struct FIFO *FIFO);
void FIFO_release(struct FIFO *FIFO);
void printMemUsed(struct FIFO *FIFO);

#endif
