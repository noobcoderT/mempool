#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mempool.h"

#define mempool_memzero(buf, n) (void) memset(buf, 0, n)
#define MEMPOOL_ALIGNMENT sizeof(unsigned long)
#define mempool_free free
#define mempool_align(d, a) (((d) + (a - 1)) & ~(a - 1))
#define mempool_align_ptr(p, a) (char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

static void *mempool_palloc_small(mempool_t *pool, size_t size, int align);
static void *mempool_palloc_block(mempool_t *pool, size_t size);
static void *mempool_palloc_large(mempool_t *pool, size_t size);

void *mempool_alloc(size_t size){
	void *p;
	p = malloc(size);
	if(NULL == p){
		printf("malloc error occured!\n");
	}
	return p;
}

void *mempool_calloc(size_t size){
	void *p;
	p = mempool_alloc(size);
	if(p){
		mempool_memzero(p, size);
	}
	return p;
}

#if (MEMPOOL_HAVE_POSIX_MEMALIGN)
void *mempool_memalign(size_t alignment, size_t size){
	void *p;
	int err;
	err = posix_memalign(&p, alignment, size);
	if(err){
		printf("Failed to create memory pool!\n");
		p = NULL;
	}
	return p;
}

#elif (MEMPOOL_HAVE_MEMALIGN)
void *mempool_memalign(size_t alignment, size_t size){
	void *p;
	p = memalign(alignment, size);
	if (NULL == P){
		printf("Failed to create memory pool!\n");
	}
	return p;
}
#endif

mempool_t *mempool_create(size_t size){
	mempool_t *p;
	p = mempool_memalign(MEMPOOL_POOL_ALIGNMENT, size);
	if(NULL == p){
		return NULL;
	}
	p->mem.last = (char *)p + sizeof(mempool_t);
	p->mem.end = (char *)p + size;
	p->mem.next = NULL;
	p->mem.failed = 0;
	size = size - sizeof(mempool_t);
	p->max = (size < MEMPOOL_MAX_ALLOC) ? size : MEMPOOL_MAX_ALLOC;
	p->current = p;
	p->large = NULL;
	p->cleanup = NULL;
	return p;
}

void mempool_destroy(mempool_t *pool){
	mempool_t *p, *n;
	mempool_large_t *l;
	mempool_cleanup_t *c;
	for(c = pool->cleanup; c; c = c->next){
		if(c->mem){
			mempool_free(c->mem);
		}
	}
	for(l = pool->large; l; l = l->next){
		if(l->alloc){
			mempool_free(l->alloc);
		}
	}
	for(p = pool, n = pool->mem.next; ; p = n, n = n->mem.next){
		mempool_free(p);
		if(NULL == n){
			break;
		}
	}
}

void mempool_reset(mempool_t *pool){
	mempool_t *p;
	mempool_large_t *l;
	for(l = pool->large; l; l = l->next){
		if(l->alloc){
			mempool_free(l->alloc);
		}
	}
	for(p = pool; p; p = p->mem.next){
		p->mem.last = (char *)p + sizeof(mempool_t);
		p->mem.failed = 0;
	}
	pool->current = pool;
	pool->large = NULL;
}

void *mempool_palloc(mempool_t *pool, size_t size){
	if(size <= pool->max){
		return mempool_palloc_small(pool, size, ALIGN);
	}
	return mempool_palloc_large(pool, size);
}

void *mempool_pnalloc(mempool_t *pool, size_t size){
	if(size <= pool->max){
		return mempool_palloc_small(pool, size, NALIGN);
	}
	return mempool_palloc_large(pool, size);
}

static void *mempool_palloc_small(mempool_t *pool, size_t size, int align){
	char *m;
	mempool_t *p;
	p = pool->current;
	do{
		m = p->mem.last;
		if(align){
			m = mempool_align_ptr(m, MEMPOOL_ALIGNMENT);
		}
		if((size_t)(p->mem.end - m) >= size){
			p->mem.last = m + size;
			return m;
		}
		p = p->mem.next;
	}while(p);
	return mempool_palloc_block(pool, size);
}

static void *mempool_palloc_block(mempool_t *pool, size_t size){
	char *m;
	size_t psize;
	mempool_t *p, *new;
	psize = (size_t)(pool->mem.end - (char *)pool);
	m = mempool_memalign(MEMPOOL_POOL_ALIGNMENT, psize);
	if(NULL == m){
		return NULL;
	}
	new = (mempool_t *)m;
	new->mem.end = m + psize;
	new->mem.next= NULL;
	new->mem.failed = 0;
	m += sizeof(mempool_data_t);
	m = mempool_align_ptr(m, MEMPOOL_ALIGNMENT);
	new->mem.last = m + size;
	for(p = pool->current; p->mem.next; p = p->mem.next){
		if(p->mem.failed++ > 4){
			pool->current = p->mem.next;
		}
	}
	p->mem.next = new;
	return m;
}

static void *mempool_palloc_large(mempool_t *pool, size_t size){
	void *p;
	mempool_uint_t n;
	mempool_large_t *large;
	p = mempool_alloc(size);
	if(NULL == p){
		return NULL;
	}
	n = 0;
	for(large = pool->large; large; large = large->next){
		if(NULL == large->alloc){
			large->alloc = p;
			return p;
		}
		if(n++ > 3){
			break;
		}
	}
	large = mempool_palloc_small(pool, sizeof(mempool_large_t), ALIGN);
	if(NULL == large){
		mempool_free(p);
		return NULL;
	}
	large->alloc = p;
	large->next = pool->large;
	pool->large = large;
	return p;
}

void *mempool_pmemalign(mempool_t *pool, size_t size, size_t alignment){
	void *p;
	mempool_large_t *large;
	p = mempool_memalign(alignment, size);
	if(NULL == p){
		return NULL;
	}
	large = mempool_palloc_small(pool, sizeof(mempool_large_t), ALIGN);
	if(NULL == large){
		mempool_free(p);
		return NULL;
	}
	large->alloc = p;
	large->next = pool->large;
	pool->large = large;
	return p;
}

int mempool_pfree(mempool_t *pool, void *p){
	mempool_large_t *l;
	for(l = pool->large; l; l = l->next){
		if(p == l->alloc){
			mempool_free(l->alloc);
			l->alloc = NULL;
			return 1;
		}
	}
	return 2;
}

void *mempool_pcalloc(mempool_t *pool, size_t size){
	void *p;
	p = mempool_palloc(pool, size);
	if(p){
		mempool_memzero(p, size);
	}
	return p;
}