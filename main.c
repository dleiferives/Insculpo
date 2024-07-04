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
		printf("Vessel id=%d, name=\"%s\"\n", i, name);
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


// commands for repl
// "@exit" -> either goes back to main menu, or exits the program
// "ctrl-c" -> exits the program
// "help" -> prints the help menu (this)
// "list" -> prints the vessels
// "select <vessel_id>" -> selects the vessel
// "add <front> <back>" -> adds a fragment to the selected vessel
// "remove <fragment_id>" -> removes a fragment from the selected vessel
// "study" -> starts the study session (shows you the front of the fragments (if they are TypeInAnswer) and then you have to answer the back, then it shows what the back is and what you wrote) when there are no more fragments, it will exit back to the main menu (if you type @exit then it will exit back to the main menu)
// "fragments" -> prints the fragments for the selected vessel
void User_REPL(User_t *user){
	u32 vessel_id = 0;
	bool running = true;
	printf("Commands:\n");
	printf("\n@exit -> either goes back to main menu, or exits the program\n");
	printf("\nctrl-c -> exits the program\n");
	printf("\nhelp -> prints the help menu (this)\n");
	printf("\nlist -> prints the vessels\n");
	printf("\nselect <vessel_id> -> selects the vessel\n");
	printf("\nadd <front> <back> -> adds a fragment to the selected vessel\n");
	printf("\nremove <fragment_id> -> removes a fragment from the selected vessel\n");
	printf("\nstudy -> starts the study session (shows you the front of the fragments (if they are TypeInAnswer) and then you have to answer the back, then it shows what the back is and what you wrote) when there are no more fragments, it will exit back to the main menu (if you type @exit then it will exit back to the main menu)\n");
	printf("\nfragments -> prints the fragments for the selected vessel\n");
	while(running){
		printf(">");
		char command[256] = {0};
		char first_word[256] = {0};
		bool first_word_done = false;
		for(u32 i = 0; i < 256; i++){
			command[i] = getchar();
			if(!first_word_done && command[i] != ' '){
				first_word[i] = command[i];
			} else if (!first_word_done && command[i] == ' '){
				first_word_done = true;
				first_word[i] = 0;
			}
			if(command[i] == '\n'){
				command[i] = 0;
				first_word[i] = 0;
				break;
			}
		}
		// ansi code to clear screen
		printf("\033[H\033[J");
		// ansi code to move cursor to top left

		if(strcmp(first_word, "@exit") == 0){
			running = false;
		}else if(strcmp(first_word, "help") == 0){
			printf("Commands:\n");
			printf("\n@exit -> either goes back to main menu, or exits the program\n");
			printf("\nctrl-c -> exits the program\n");
			printf("\nhelp -> prints the help menu (this)\n");
			printf("\nlist -> prints the vessels\n");
			printf("\nselect <vessel_id> -> selects the vessel\n");
			printf("\nadd <front> <back> -> adds a fragment to the selected vessel\n");
			printf("\nremove <fragment_id> -> removes a fragment from the selected vessel\n");
			printf("\nstudy -> starts the study session (shows you the front of the fragments (if they are TypeInAnswer) and then you have to answer the back, then it shows what the back is and what you wrote) when there are no more fragments, it will exit back to the main menu (if you type @exit then it will exit back to the main menu)\n");
			printf("\nfragments -> prints the fragments for the selected vessel\n");
		}else if(strcmp(first_word, "list") == 0){
			User_vessels_print(user);
		}else if(strcmp(first_word, "select") == 0){
			// get the vessel id from the command
			sscanf(command, "select %d", &vessel_id);
			if(vessel_id >= user->vessels.size){
				printf("Vessel id is out of bounds\n");
				vessel_id = 0;
			}
		}else if(strcmp(first_word, "add") == 0){
			if(vessel_id == 0){
				printf("No vessel selected\n");
			}else{
				printf("Front: ");
				u8 front[256] = {0};
				u8 back[256] = {0};
				for(u32 i = 0; i < 256; i++) {
					front[i] = getchar();
					if(front[i] == '\n'){
						front[i] = 0;
						break;
					}
				}
				printf("Back: ");
				for(u32 i = 0; i < 256; i++) {
					back[i] = getchar();
					if(back[i] == '\n'){
						back[i] = 0;
						break;
					}
				}
				User_fragments_add_type_in_answer(user, vessel_id, InternPool_intern(user->pool, front, strlen(front)), InternPool_intern(user->pool, back, strlen(back)));
			}
		}else if(strcmp(first_word, "remove") == 0){
			if(vessel_id == 0){
				printf("No vessel selected\n");
			}else{
				u32 fragment_id;
				sscanf(command, "remove %d", &fragment_id);
				Vessel_t *vessel = User_vessels_get(user, vessel_id);
				if(fragment_id >= vessel->fragment_ids.size){
					printf("Fragment id is out of bounds\n");
				}else{
					u32 *fragment_ids = (u32 *)vessel->fragment_ids.data;
					for(u32 i = fragment_id; i < vessel->fragment_ids.size - 1; i++){
						fragment_ids[i] = fragment_ids[i + 1];
					}
					vessel->fragment_ids.size--;
				}
			}
		} else if(strcmp(first_word, "study") == 0){
			if(vessel_id == 0){
			printf("No vessel selected\n");
			}else{
			Vessel_t *vessel = User_vessels_get(user, vessel_id);
				for(u32 i = 0; i < vessel->fragment_ids.size; i++){
					printf("\033[H\033[J");
					u32 *fragment_ids = (u32 *)vessel->fragment_ids.data;
					Fragment_t *fragment = (Fragment_t *)user->fragments.data + fragment_ids[i];
					u8 *front;
					u8 *back;
					switch(fragment->kind){
						case FRAGMENT_TYPE_IN_ANSWER:
							front = InternPool_get(user->pool, fragment->data.type_in_answer.front);
							back = InternPool_get(user->pool, fragment->data.type_in_answer.back);
							printf("%s\n", front);
							// ansi code to get width of terminal
							for(u32 i = 0; i < fragment->data.type_in_answer.front.len; i++){
								printf("-");
							}
							printf("\n");
							u8 answer[256] = {0};
							for(u32 i = 0; i < 256; i++){
								answer[i] = getchar();
								if(answer[i] == '\n'){
									answer[i] = 0;
									break;
								}
							}
							printf("%s\n", back);
							if(i < vessel->fragment_ids.size - 1)
								printf("Press enter to continue\n");
						break;
						default:
						printf("Fragment %d: Unknown\n", i);
						break;
					}
				}
			}
		} 
		else if(strcmp(first_word, "fragments") == 0){
			if(vessel_id == 0){
				printf("No vessel selected\n");
			}else{
				Vessel_t *vessel = User_vessels_get(user, vessel_id);
				for(u32 i = 0; i < vessel->fragment_ids.size; i++){
					u32 *fragment_ids = (u32 *)vessel->fragment_ids.data;
					Fragment_t *fragment = (Fragment_t *)user->fragments.data + fragment_ids[i];
					u8 *front;
					u8 *back;
					switch(fragment->kind){
						case FRAGMENT_TYPE_IN_ANSWER:
							front = InternPool_get(user->pool, fragment->data.type_in_answer.front);
							back = InternPool_get(user->pool, fragment->data.type_in_answer.back);
							printf("Fragment id=%d Type In Answer: %s -> %s\n", i, front, back);
						break;
						default:
						printf("Fragment %d: Unknown\n", i);
						break;
					}
				}
			}
		} else {
			printf("Unknown command: \"%s\" %s \n", command, first_word);
		}
	}
			
}



int main(int argc, char **argv){
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
		
	// load the user from a file, if none are provided then create a new user
	if(argc > 1){
		User_t new_user = User_deserialize(argv[1]);
		User_REPL(&new_user);
	} else {
		printf("No user file provided\n");
		printf("Creating new user \"user.bin\", with some basic sets\n");
		printf("To load a user from a file, provide the file as the first argument\n");
		printf("Like so: %s user.bin\n", argv[0]);
		User_t new_user = User_new();
		User_vessels_add(&new_user, InternPool_intern(new_user.pool, "Programming Culture", 19));
		User_fragments_add_type_in_answer(&new_user, 1, InternPool_intern(new_user.pool, "What tends to come after \"Hello\"?", 33), InternPool_intern(new_user.pool, "World", 5));
		printf("User created\n");
		printf("Press enter to start\n");
		while(getchar() != '\n');
		User_REPL(&new_user);
		User_serialize(&new_user, "user.bin");
	}


	

	return 0;
}
