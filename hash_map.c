#include"hash_map.h"
#include"list_t.h"
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<ctype.h>
#include<wchar.h>
#define DEFAULT_LIST 20
#define DEFAULT_PERCENT 20
#define MAX_LISTS 650

typedef struct c_hash_map{
	long int size;
	list *lists;
} *hash_map;

static int num_lists = -1;
static hash_map map;

//Create a number of lists equal to the DEFAULT_PERCENT of the number of values
void initialize_map(long int value){
	if(num_lists == -1 && value > DEFAULT_PERCENT)
		if ( (value / DEFAULT_PERCENT) > MAX_LISTS)
			num_lists = MAX_LISTS;
		else
			num_lists = value / DEFAULT_PERCENT;
}

/**
 * Perform hashing
 * An hash is obtained from the lowered character sum of the key, moduled with the number of lists
 */
static int hashing(char* key){
	int hash = 0;
	for(int i = 0; i < strlen(key); i++){
		key[i] = tolower(key[i]);
		hash += key[i];
	}

	return hash % num_lists < 0 ? hash % num_lists * -1 : hash % num_lists;
}

void new_hash_map(){
	map = malloc(sizeof(struct c_hash_map));
	map -> size = 0;
	if(num_lists == -1)
		num_lists = DEFAULT_LIST;
	map -> lists = malloc(sizeof(struct c_hash_map*) * num_lists);
	for(int i = 0; i < num_lists; i++){
		map ->lists[i] = newList();
	}
}

void new_hash_map_array(cell* array, long int lenght){
	initialize_map(lenght);
	new_hash_map();
	add_cell_array(array,lenght);
}

void add_cell_array(cell* vet,long int dim){
	for (long int i = 0; i < dim; i++)
		add_or_update(&vet[i]);
}

/**
 * Very specific method for WordCount problem instances
 */
int add_or_update(cell* c){
	if( c-> key == NULL )
		return 0;

	int pos = hashing(c -> key);
	if(!updateList(map -> lists[pos],c))
		if(!insertList(map->lists[pos],sizeList(map->lists[pos]),c))
			return 0;

	return 1;
}

cell* compact_map(long int *dim){
	long int total_size = 0;
	for(long int i=0; i < num_lists; i++)
		total_size += sizeList(map -> lists[i]);
	*dim = total_size;

	cell* array = calloc(sizeof(cell),total_size);
	for (long int i = 0,current_pos = 0; i < num_lists; i++)
			for (int j = 0; j < sizeList(map -> lists[i]); j++)
				memcpy(&array[current_pos++],getItem(map -> lists[i],j),sizeof(cell));
	return array;
}

void output_hash_map(){
	for (long int i = 0; i < num_lists; i++){
		if(sizeList(map->lists[i]) == 0)
			continue;
		else{
		printf("LIST %ld\n",i);
			outputList(map->lists[i]);
		}
	}
}

void free_hash_map(){
	for (long int i = 0; i < num_lists; i++)
		freeList(map -> lists[i]);
	free(map);
}