#ifndef _MEMPOOL_H
#define _MEMPOOL_H 1

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#define MEMPOOL_POOL_ALIGNMENT 16
#define MEMPOOL_MAX_ALLOC (int) getpagesize()

#if (MEMPOOL_HAVE_POSIX_MEMALIGN || MEMPOOL_HAVE_MEMALIGN)
void *mempool_memalign(size_t alignment, size_t size, ngx_log_t *log);
#else
#define mempool_memalign(alignment, size)  mempool_alloc(size)
#endif

typedef uintptr_t mempool_uint_t;

enum mempool_palloc_align{
	NALIGN = 0,
	ALIGN  = 1,
};

/*typedef mempool_chain_s{

}mempool_chain_t;*/

typedef struct mempool_data_s mempool_data_t;
typedef struct mempool_large_s mempool_large_t;
typedef struct mempool_cleanup_s mempool_cleanup_t;
typedef struct mempool_s mempool_t;

struct mempool_data_s{
	char *last;
	char *end;
	mempool_t *next;
	mempool_uint_t failed;
};

struct mempool_large_s{
	mempool_large_t *next;
	void *alloc;
};

struct mempool_cleanup_s{
	void *mem;
	mempool_cleanup_t *next;
};

struct mempool_s{
	mempool_data_t mem;
	size_t max;
	mempool_t *current;
	//mempool_chain_t *chain;
	mempool_large_t *large;
	mempool_cleanup_t *cleanup;
};

void *mempool_alloc(size_t size);
void *mempool_calloc(size_t size);
mempool_t *mempool_create(size_t size);
void *mempool_memaligh(size_t alignment, size_t size);
void mempool_destroy(mempool_t *pool);
void mempool_reset(mempool_t *pool);
void *mempool_palloc(mempool_t *pool, size_t size);
void *mempool_pnalloc(mempool_t *pool, size_t size);
void *mempool_pmemalign(mempool_t *pool, size_t size, size_t alignment);
int mempool_pfree(mempool_t *pool, void *p);
void *mempool_pcalloc(mempool_t *pool, size_t size);
#endif