#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "author_cache.h"

static uint64_t hash_str(const char *s) {
		uint64_t h = 1469598103934665603ULL;
			while (*s) {
				h ^= (unsigned char)(*s++);
				h *= 1099511628211ULL;
			}
			return h;
}

static const char *ltrim(const char *s) {
	while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
		s++;
	return s;
}

static void rtrim(char *s) {
	size_t n = strlen(s);
	while (n > 0) {
		char c = s[n -1];
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
			n--;
		else
			break;
	}
		s[n] = '\0';
}

static void make_key(
		char *out, 
		const char *first_name,
		const char *last_name,
		const char *initial
		) {

	first_name = ltrim(first_name);
	last_name = ltrim(last_name);
	initial = ltrim(initial);

	char f[128], l[128], i[64];

	strncpy(f, first_name, sizeof(f)-1);
	strncpy(l, last_name, sizeof(l)-1);
	strncpy(i, initial, sizeof(i)-1);

	f[sizeof(f)-1] = 0;
	l[sizeof(l)-1] = 0;
	i[sizeof(i)-1] = 0;

	rtrim(f);
	rtrim(l);
	rtrim(i);

	char *p = out;

	for (char *s = f; *s; s++) *p++ = *s;
	*p++ = '|';
	
	for (char *s = l; *s; s++) *p++ = *s;
	*p++ = '|';
	
	for (char *s = i; *s; s++) *p++ = *s;
	*p = '\0';

}

void author_cache_init(
		AuthorCache *c) {
	memset(c, 0, sizeof(*c));
	printf("[CACHE INIT] addr=%p\n", 
			(void*)c);
}

int author_cache_get(AuthorCache *c,
		const char *first_name,
		const char *last_name,
		const char *initial)
{
	char key[256];
	make_key(key,
		first_name,
		last_name,
		initial);

	uint64_t h = hash_str(key);
	size_t idx = h & (CACHE_SIZE -1);

		for (int i = 0; i < CACHE_SIZE; i++) {
		size_t p = (idx + i) & (CACHE_SIZE -1);

		if (!c->table[p].used)
		return -1;
		
		if (c->table[p].hash == h && 
			strcmp(c->table[p].key, key) == 0)
			return c->table[p].id;
	}
	return -1;
}

void author_cache_put(
		AuthorCache *c,
		const char *first_name,
		const char *last_name,
		const char *initial,
		int id
		)
{
		
	char key[256];
	make_key(
		key, 
		first_name,
		last_name,
		initial
		);

	uint64_t h = hash_str(key);
	size_t idx = h & (CACHE_SIZE - 1);
	
	for (int i = 0; i < CACHE_SIZE; i++) {
		size_t p = (idx + i) & (CACHE_SIZE - 1);

		if (c->table[p].used) {
			if (c->table[p].key &&
				strcmp(c->table[p].key, key) == 0) {
				return;
			}
			continue;
		}

		char *copy = strdup(key);
		if(!copy) return;
		
		c->table[p].hash = h;
		c->table[p].key = copy;
		c->table[p].id = id;
	    	c->table[p].used = 1;
		return;
	}
}
