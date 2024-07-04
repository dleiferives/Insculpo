// TODO
// add defragmentation to the StringArena (and therefore to the InternPool and everything else too)
// add proper error handling
// cover empty string in arena or smthn
#ifndef __INTERN_POOL_H__
#define __INTERN_POOL_H__
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#define INTERNPOOL_DEFAULT_CAPACITY (256)
#define STRING_ARENA_DEFAULT_CAPACITY (1024)
#define assert(value,string,...) \
	do { \
		if (!(value)){\
			fprintf(stderr, "Assertion Failed, Error: %s\n", #value);\
			fprintf(stderr, "File: %s, Line: %d\n", __FILE__,__LINE__);\
			fprintf(stderr, string "\n", ##__VA_ARGS__);\
			abort();\
		}\
	}while(0)

typedef uint32_t u32;
typedef unsigned char u8;




/// StringArena type
/// holds an arena of strings, 
/// each added string gets put onto the end. 
typedef struct{
	u32 size;
	u32 capacity;
	u8 * data;
}StringArena_t;

StringArena_t *StringArena_new(void);

StringArena_t *StringArena_resize(StringArena_t *arena);

bool StringArena_add(StringArena_t *arena, u8 *data, u32 len);

void StringArena_free(StringArena_t *arena);

typedef struct{
	u32 start;
	u32 len;
}InternPoolString_t;
typedef InternPoolString_t StrId;

// a hashmap, from hashed string, to InternPoolStrings
typedef struct {
	u32 size;
	u32 capacity;
	StringArena_t *arena;
	InternPoolString_t *strings;
}InternPool_t;

InternPool_t *InternPool_new(void);

u32 InternPool_hash_string(u8 *data, u32 len);

void InternPool_resize(InternPool_t *pool);

StrId InternPool_intern(InternPool_t *pool, u8 *data, u32 len);

bool InternPool_contains(InternPool_t *pool, u8 *data, u32 len);

u8 *InternPool_get(InternPool_t *pool, StrId id);

void InternPool_free(InternPool_t *pool);

void StringArena_serialize_to_file(StringArena_t *arena, FILE *file);

void InternPool_serialize_to_file(InternPool_t *pool, FILE *file);

StringArena_t * StringArena_deserialize_from_file(FILE *file);

InternPool_t * InternPool_deserialize_from_file(FILE *file);

void InternPool_serialize(InternPool_t *pool, u8 *file_name);


InternPool_t *InternPool_deserialize(u8 *file_name);

#endif
