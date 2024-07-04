// TODO
// Implement String Intern Pool
// Implement Lists

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
/// id holds the fragment id, if 0 this is an invalid fragment
/// vessel_id holds the vessel that contains this fragment 
struct Fragment_s{
	u32 id;
	u32 vessel_id;
	time_t creation;
	time_t challenge_occurance;
	Fragment_k kind;
	FragmentData_t data;
};

/// Vessel struct definition
/// user_id holds the user id, if 0 there is no associated user
/// id is the vessel id, if it is 0 this is an invalid vessel
/// name is the id for the string.
struct Vessel_s {
	u32 id;
	u32 user_id;
	StrId name;
	Vessel_t * vessels; // to be replaced with a list
	Fragment_t * fragments; // to be replaced with a list
	u32 vessels_len;
	u32 fragments_len;
};

/// User struct definition
/// id defaults to 0 if there is not a user
/// memory_strength is determined continiously, and determines the rate at which 
/// the user is challenged
struct User_s{
	u32 id;
	float memory_strength;
	Vessel_t vessel;
	InternPool_t pool;
};


int main(void){
	
	return 0;
}
