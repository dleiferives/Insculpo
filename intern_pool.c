#include "intern_pool.h"
u8 *InternPool_get_string(InternPool_t *pool, StrId id) {
	return pool->arena->data + pool->strings[id].start;
}

InternPoolString_t InternPool_get(InternPool_t *pool, StrId id) {
  return pool->strings[id];
}

StrId InternPool_contains(InternPool_t *pool, u8 *data, u32 len) {
  u32 hash = InternPool_hash_string(data, len);
  u32 index = hash % pool->capacity;
  while (pool->strings[index].start != 0) {
    if (pool->strings[index].len == len) {
      int equal =
          memcmp(data, pool->arena->data + pool->strings[index].start, len);
      if (equal == 0) {
        return index;
      }
    }
    index = (index + 1) % pool->capacity;
  }
  return 0;
}

StrId InternPool_intern(InternPool_t *pool, u8 *data, u32 len) {
  u32 hash = InternPool_hash_string(data, len);
  u32 index = hash % pool->capacity;
  while (pool->strings[index].start != 0) {
    if (pool->strings[index].len == len) {
      int equal =
          memcmp(data, pool->arena->data + pool->strings[index].start, len);
      if (equal == 0) {
        return index;
      }
    }
    index = (index + 1) % pool->capacity;
  }
  pool->strings[index].start = pool->arena->size;
  pool->strings[index].len = len;
  StringArena_add(pool->arena, data, len);
  pool->size++;
  while (pool->size > pool->capacity / 3) {
    InternPool_resize(pool);
  }
  return index;
}

void InternPool_resize(InternPool_t *pool) {
  // create a new pool
  // set the new pool's capacity to the old pool's capacity +
  // INTERNPOOL_DEFAULT_CAPACITY
  InternPool_t *new_pool = (InternPool_t *)calloc(1, sizeof(InternPool_t));
  assert(new_pool != NULL, "Could not allocate a new InternPool");
  new_pool->capacity = pool->capacity + INTERNPOOL_DEFAULT_CAPACITY;
  new_pool->strings = (InternPoolString_t *)calloc(new_pool->capacity,
                                                   sizeof(InternPoolString_t));
  assert(new_pool->strings != NULL,
         "Could not allocate new strings for InternPool");
  new_pool->arena = pool->arena;

  for (u32 i = 0; i < pool->capacity; i++) {
    if (pool->strings[i].start != 0) {
      u32 hash = InternPool_hash_string(
          pool->arena->data + pool->strings[i].start, pool->strings[i].len);
      u32 index = hash % new_pool->capacity;
      while (new_pool->strings[index].start != 0) {
        index = (index + 1) % new_pool->capacity;
      }
      new_pool->strings[index].start = pool->strings[i].start;
      new_pool->strings[index].len = pool->strings[i].len;
    }
  }
  free(pool->strings);
  pool->strings = new_pool->strings;
  free(new_pool);
}

u32 InternPool_hash_string(u8 *data, u32 len) {
  u32 hash = 5381;
  for (u32 i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + data[i];
  }
  return hash;
}

InternPool_t *InternPool_new(void) {
  InternPool_t *pool = (InternPool_t *)calloc(1, sizeof(InternPool_t));
  pool->size = 0;
  pool->capacity = INTERNPOOL_DEFAULT_CAPACITY;
  pool->strings = (InternPoolString_t *)calloc(INTERNPOOL_DEFAULT_CAPACITY,
                                               sizeof(InternPool_t));
  pool->arena = StringArena_new();
  pool->arena->size = 1;
  return pool;
}

bool StringArena_add(StringArena_t *arena, u8 *data, u32 len) {
  while (arena->capacity <= arena->size + len) {
    arena = StringArena_resize(arena);
  }

  assert(arena->data != NULL, "Could not add string %s to Arena that is empty",
         data);
  assert(
      data != NULL,
      "Could not add empty string to Arena, this should already be covered?");
  for (u32 i = 0; i < len; i++) {
    arena->data[arena->size++] = data[i];
  }
  arena->data[arena->size++] = '\0'; // null terminate the string
  return true;
}

StringArena_t *StringArena_resize(StringArena_t *arena) {
  u8 *result =
      (u8 *)realloc(arena->data, sizeof(u8) * (arena->capacity +
                                               STRING_ARENA_DEFAULT_CAPACITY));
  assert(result != NULL, "Could not resize StringArena");
  arena->capacity = arena->capacity + STRING_ARENA_DEFAULT_CAPACITY;
  return arena;
}

StringArena_t *StringArena_new(void) {
  StringArena_t *result = (StringArena_t *)malloc(sizeof(StringArena_t));
  assert(result != NULL, "Could not allocate a new StringArena");
  result->size = 0;
  result->capacity = STRING_ARENA_DEFAULT_CAPACITY;
  result->data = NULL;
  result->data = (u8 *)malloc(result->capacity * sizeof(u8));
  assert(result->data != NULL, "Could not allocate data for StringArena");
  return result;
}

void InternPool_free(InternPool_t *pool) {
  free(pool->strings);
  StringArena_free(pool->arena);
  free(pool);
}

void StringArena_free(StringArena_t *arena) {
  free(arena->data);
  free(arena);
}

void StringArena_serialize_to_file(StringArena_t *arena, FILE *file) {
  fwrite(&arena->size, sizeof(u32), 1, file);
  fwrite(&arena->capacity, sizeof(u32), 1, file);
  fwrite(arena->data, sizeof(u8), arena->size, file);
}

void InternPool_serialize_to_file(InternPool_t *pool, FILE *file) {
  fwrite(&pool->size, sizeof(u32), 1, file);
  fwrite(&pool->capacity, sizeof(u32), 1, file);
	// iterate through the strings and serialize the index, then the InternPoolString, if the entry's start !=0
	for (u32 i = 0; i < pool->capacity; i++) {
		if (pool->strings[i].start != 0) {
			fwrite(&i, sizeof(u32), 1, file);
			fwrite(&pool->strings[i], sizeof(InternPoolString_t), 1, file);
		}
	  }
	 
	StringArena_serialize_to_file(pool->arena, file);
}

StringArena_t * StringArena_deserialize_from_file(FILE *file) {
	StringArena_t *arena = (StringArena_t *)calloc(1, sizeof(StringArena_t));
	assert(arena != NULL, "Could not allocate a new StringArena");
	fread(&arena->size, sizeof(u32), 1, file);
	fread(&arena->capacity, sizeof(u32), 1, file);
	arena->data = (u8 *)calloc(arena->capacity, sizeof(u8));
	assert(arena->data != NULL, "Could not allocate data for StringArena");
	fread(arena->data, sizeof(u8), arena->size, file);
	return arena;
}


InternPool_t * InternPool_deserialize_from_file(FILE *file) {
	InternPool_t *pool = (InternPool_t *)calloc(1, sizeof(InternPool_t));
	assert(pool != NULL, "Could not allocate a new InternPool");
	fread(&pool->size, sizeof(u32), 1, file);
	fread(&pool->capacity, sizeof(u32), 1, file);
	pool->strings = (InternPoolString_t *)calloc(pool->capacity, sizeof(InternPoolString_t));
	assert(pool->strings != NULL, "Could not allocate new strings for InternPool");
	for (u32 i = 0; i < pool->size; i++) {
		u32 index;
		fread(&index, sizeof(u32), 1, file);
		fread(&pool->strings[index], sizeof(InternPoolString_t), 1, file);
	}
	pool->arena = StringArena_deserialize_from_file(file);
	return pool;
}

void InternPool_serialize(InternPool_t *pool, u8 *file_name){
	FILE *file = fopen(file_name, "wb");
	InternPool_serialize_to_file(pool, file);
	fclose(file);
}

InternPool_t *InternPool_deserialize(u8 *file_name){
	FILE *file = fopen(file_name, "rb");
	InternPool_t *pool = InternPool_deserialize_from_file(file);
	fclose(file);
	return pool;
}

