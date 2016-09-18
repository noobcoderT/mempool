#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mempool.h"

void main(void){
	mempool_t *mempool = mempool_create(1024);
	printf("mempool create succeed!\n");
	char *p = mempool_palloc(mempool, 1020);
	printf("mempool alloc succeed!\n");
	strcpy(p, "mempool works!");
	printf("%s\n", p);
	char *large = mempool_palloc(mempool, 64);
	strcpy(large, "large alloc succeed!");
	printf("%s\n", large);
	mempool_destroy(mempool);
	printf("destroy mempool succeed!\n");
}