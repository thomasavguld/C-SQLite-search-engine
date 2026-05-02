#ifndef AUTHOR_CACHE_H
#define AUTHOR_CACHE_H

#include <stdint.h>
#include <stddef.h>

#define CACHE_SIZE 131072

typedef struct {
	char *key;
	uint64_t hash;
	int id;
	uint8_t used;
} CacheEntry;

typedef struct {
	CacheEntry table[CACHE_SIZE];
} AuthorCache;

void author_cache_init(
		AuthorCache *c
		);

int author_cache_get(
		AuthorCache *c,
		const char *first_name,
		const char *last_name,
		const char *initial
		);

void author_cache_put(
		AuthorCache *c,
		const char *first_name,
		const char *last_name,
		const char *initial,
		int id
		);

#endif
