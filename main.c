// TODO
// If the user wants to resize their serialized data, then all of the references will have to be updated
// this is because the fragments, and the names all use the offsets into that pool for their data
// Implement timing for the user, and the fragments

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "intern_pool.h"


// Things that one want to remember
// Intervals that one should be challenged at
typedef uint32_t u32;
typedef int32_t s32;
typedef char s8;
typedef unsigned char u8;


// type pre-declarations
typedef enum Fragment_e Fragment_k;
typedef union FragmentData_u FragmentData_t;
typedef struct Fragment_s Fragment_t;
typedef struct Vessel_s Vessel_t;
typedef struct User_s User_t;



#define LIST_DEFAULT_CAPACITY 128
typedef enum{
	TYPE_U8,
	TYPE_U16,
	TYPE_U32,
	TYPE_U64,
	TYPE_S8,
	TYPE_S16,
	TYPE_S32,
	TYPE_S64,
	TYPE_FLOAT,
	TYPE_DOUBLE,
	TYPE_STRINGID,
	TYPE_VESSEL,
	TYPE_USER,
	TYPE_FRAGMENT,
	TYPE_LIST,
	TYPE_COUNT_,
}Type_k;

typedef struct {
	size_t member_size;
	u32 size;
	u32 capacity;
	void *data;
	Type_k type;
}List_t;


List_t List_new(Type_k type, size_t member_size){
	List_t list = {0};
	list.member_size = member_size;
	list.capacity = LIST_DEFAULT_CAPACITY;
	list.size = 0;
	list.type = type;
	list.data = calloc(list.capacity, list.member_size);
	assert(list.data != NULL, "Could not allocate list data");
	return list;
}

void List_resize(List_t * list){
	list->capacity *= 2;
	list->data = realloc(list->data, list->capacity * list->member_size);
	assert(list->data != NULL, "Could not reallocate list data");
}

void List_add(List_t * list, void * data){
	while(list->size >= list->capacity - 1 ){
		List_resize(list);
	}
	void *dest = (u8 *)list->data + (list->size * list->member_size);
	memcpy(dest, data, list->member_size);
	list->size++;
}

void List_serialize_to_file(List_t * list, FILE * file){
	fwrite(&list->size, sizeof(u32), 1, file);
	fwrite(&list->capacity, sizeof(u32), 1, file);
	fwrite(&list->type, sizeof(Type_k), 1, file);
	fwrite(&list->member_size, sizeof(size_t), 1, file);
	fwrite(list->data, list->member_size, list->size, file);
}

List_t List_deserialize_from_file(FILE * file){
	List_t list = {0};
	fread(&list.size, sizeof(u32), 1, file);
	fread(&list.capacity, sizeof(u32), 1, file);
	fread(&list.type, sizeof(Type_k), 1, file);
	fread(&list.member_size, sizeof(size_t), 1, file);
	list.data = calloc(list.capacity, list.member_size);
	assert(list.data != NULL, "Could not allocate list data");
	fread(list.data, list.member_size, list.size, file);
	return list;
}

enum Fragment_e{
	FRAGMENT_TYPE_IN_ANSWER,	
	FRAGMENT_COUNT,
};

struct FragmentTypeInAnswer_s{
	StrId front;
	StrId back;
};

union FragmentData_u{
	struct FragmentTypeInAnswer_s type_in_answer;
};

/// Fragment struct definition
/// id holds the fragment id, if 0 this is an invalid fragment (has no vessel)
/// vessel_id holds the vessel that contains this fragment 
struct Fragment_s{
	u32 id;
	u32 vessel_id;
	time_t creation;
	time_t challenge_occurance;
	Fragment_k kind;
	FragmentData_t data;
};

void Fragment_serialize_to_file(Fragment_t * fragment, FILE * file){
	fwrite(&fragment->id, sizeof(u32), 1, file);
	fwrite(&fragment->vessel_id, sizeof(u32), 1, file);
	fwrite(&fragment->creation, sizeof(time_t), 1, file);
	fwrite(&fragment->challenge_occurance, sizeof(time_t), 1, file);
	fwrite(&fragment->kind, sizeof(Fragment_k), 1, file);
	switch(fragment->kind){
		case FRAGMENT_TYPE_IN_ANSWER:
		fwrite(&fragment->data.type_in_answer.front, sizeof(StrId), 1, file);
		fwrite(&fragment->data.type_in_answer.back, sizeof(StrId), 1, file);
		break;
		default:
		break;
	}
}

Fragment_t Fragment_deserialize_from_file(FILE * file){
	Fragment_t fragment = {0};
	fread(&fragment.id, sizeof(u32), 1, file);
	fread(&fragment.vessel_id, sizeof(u32), 1, file);
	fread(&fragment.creation, sizeof(time_t), 1, file);
	fread(&fragment.challenge_occurance, sizeof(time_t), 1, file);
	fread(&fragment.kind, sizeof(Fragment_k), 1, file);
	switch(fragment.kind){
		case FRAGMENT_TYPE_IN_ANSWER:
		fread(&fragment.data.type_in_answer.front, sizeof(StrId), 1, file);
		fread(&fragment.data.type_in_answer.back, sizeof(StrId), 1, file);
		break;
		default:
		break;
	}
	return fragment;
}

/// Vessel struct definition
/// user_id holds the user id, if 0 there is no associated user
/// id is the vessel id, if it is 0 this is an invalid vessel
/// name is the id for the string.
struct Vessel_s {
	u32 id;
	u32 user_id;
	StrId name;
	List_t fragment_ids;
};

void Vessel_serialize_to_file(Vessel_t * vessel, FILE * file){
	fwrite(&vessel->id, sizeof(u32), 1, file);
	fwrite(&vessel->user_id, sizeof(u32), 1, file);
	fwrite(&vessel->name, sizeof(StrId), 1, file);
	List_serialize_to_file(&vessel->fragment_ids, file);
}

Vessel_t Vessel_deserialize_from_file(FILE * file){
	Vessel_t vessel = {0};
	fread(&vessel.id, sizeof(u32), 1, file);
	fread(&vessel.user_id, sizeof(u32), 1, file);
	fread(&vessel.name, sizeof(StrId), 1, file);
	vessel.fragment_ids = List_deserialize_from_file(file);
	return vessel;
}



/// User struct definition
/// id defaults to 0 if there is not a user
/// memory_strength is determined continiously, and determines the rate at which 
/// the user is challenged
struct User_s{
	u32 id;
	float memory_strength;
	InternPool_t *pool;
	List_t fragments;
	List_t vessels;
};

void User_serialize_to_file(User_t * user, FILE * file){
	fwrite(&user->id, sizeof(u32), 1, file);
	fwrite(&user->memory_strength, sizeof(float), 1, file);
	InternPool_serialize_to_file(user->pool, file);
	List_serialize_to_file(&user->fragments, file);
	List_serialize_to_file(&user->vessels, file);
}

User_t User_deserialize_from_file(FILE * file){
	User_t user = {0};
	fread(&user.id, sizeof(u32), 1, file);
	fread(&user.memory_strength, sizeof(float), 1, file);
	user.pool = InternPool_deserialize_from_file(file);
	user.fragments = List_deserialize_from_file(file);
	user.vessels = List_deserialize_from_file(file);
	return user;
}

void User_vessels_add(User_t *user, StrId name){
	assert(user != NULL, "User cannot be null when creating a vessel");
	Vessel_t vessel = {
		.user_id = user->id,
		.name = name,
		.id = user->vessels.size,

	};
	vessel.fragment_ids = List_new(TYPE_U32, sizeof(u32));
	List_add(&user->vessels, &vessel);
}

Vessel_t *User_vessels_get(User_t *user, u32 id){
	assert(user != NULL, "User cannot be null when getting a vessel");
	assert(id < user->vessels.size, "Vessel id is out of bounds");
	return &((Vessel_t *)user->vessels.data)[id];
}

void User_fragments_add_type_in_answer(User_t *user, u32 vessel_id, StrId front, StrId back){
	assert(user != NULL, "User cannot be null when creating a fragment");
	assert(vessel_id < user->vessels.size, "Vessel id is out of bounds");
	assert(vessel_id != 0, "Vessel id cannot be 0");
	Vessel_t *vessel = User_vessels_get(user, vessel_id);
	Fragment_t fragment = {
		.id = user->fragments.size,
		.vessel_id = vessel_id,
		.creation = time(NULL),
		.challenge_occurance = 0,
		.kind = FRAGMENT_TYPE_IN_ANSWER,
		.data.type_in_answer.front = front,
		.data.type_in_answer.back = back,
	};
	List_add(&user->fragments, &fragment);
	List_add(&vessel->fragment_ids, &fragment.id);
}

User_t User_new(void){
	User_t user = {0};
	user.pool = InternPool_new();
	user.fragments = List_new(TYPE_FRAGMENT, sizeof(Fragment_t));
	user.vessels = List_new(TYPE_VESSEL, sizeof(Vessel_t));
	user.vessels.size = 1;
	user.fragments.size = 1;
	return user;
}

void User_vessels_print(User_t *user){
	for(u32 i = 1; i < user->vessels.size; i++){
		Vessel_t *vessel = User_vessels_get(user, i);
		u8 *name = InternPool_get(user->pool, vessel->name);
		printf("Vessel %d: %s\n", i, name);
	}
}

void User_fragments_print(User_t *user){
	for(u32 i = 1; i < user->fragments.size; i++){
		Fragment_t *fragment = (Fragment_t *)user->fragments.data + i;
		u8 *front;
		u8 *back;
		switch(fragment->kind){
			case FRAGMENT_TYPE_IN_ANSWER:

				front = InternPool_get(user->pool, fragment->data.type_in_answer.front);
				back = InternPool_get(user->pool, fragment->data.type_in_answer.back);
				printf("Fragment %d Type In Answer: %s -> %s\n", i, front, back);
			break;
			default:
			printf("Fragment %d: Unknown\n", i);
			break;
		}
	}
}

void User_serialize(User_t *user, u8 *file){
	FILE *f = fopen(file, "wb");
	User_serialize_to_file(user, f);
	fclose(f);
}

User_t User_deserialize(u8 *file){
	FILE *f = fopen(file, "rb");
	User_t user = User_deserialize_from_file(f);
	fclose(f);
	return user;
}







int main(void){
	// InternPool_t * pool = InternPool_new();
	// StrId hello = InternPool_intern(pool, "Hello", 5);
	// StrId world = InternPool_intern(pool, "World", 5);
	// printf("%d\n", world.start);
	// u8 *str = InternPool_get(pool, hello);
	// printf("%s\n", str);
	// str = InternPool_get(pool, world);
	// printf("%s\n", str);

	// InternPool_serialize(pool, "pool.bin");

	// InternPool_t * new_pool = InternPool_deserialize("pool.bin");
	// str = InternPool_get(new_pool, hello);
	// printf("%s\n", str);
	// world = InternPool_intern(new_pool, "W0rld", 5);
	// printf("%d\n", world.start);
	// str = InternPool_get(new_pool, world);
	// printf("%s\n", str);
	// world = InternPool_intern(new_pool, "World", 5);
	// printf("%d\n", world.start);
	// str = InternPool_get(new_pool, world);
	// printf("%s\n", str);
	//
		
	User_t user = User_new();

	User_vessels_add(&user, InternPool_intern(user.pool, "Vessel 1", 8));
	User_vessels_add(&user, InternPool_intern(user.pool, "Vessel 2", 8));

	User_fragments_add_type_in_answer(&user, 1, InternPool_intern(user.pool, "Front 1", 7), InternPool_intern(user.pool, "Back 1", 6));
	User_fragments_add_type_in_answer(&user, 1, InternPool_intern(user.pool, "Front 2", 7), InternPool_intern(user.pool, "Back 2", 6));

	User_vessels_print(&user);
	User_fragments_print(&user);

	User_serialize(&user, "user.bin");

	User_t new_user = User_deserialize("user.bin");

	User_vessels_print(&new_user);

	

	return 0;
}
