#include"hash_map.h"
#include"list_t.h"
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<ctype.h>

typedef struct c_hash_map{
	long int size;
	list *lists;
} *hash_map;

static int num_lists = -1;
static hash_map map;

//Inizialize number of lists
void initialize_map(long int value){
	if( num_lists == -1)
		num_lists = (value > MAX_WORD_NUM) ? MAX_WORD_NUM : (value <= 0 ? DEFAULT_LIST : value) ;
}

/**
 * Perform a FNV hashing
 */
static unsigned hashing(char *key)
{
	for (int i = 0; i < strlen(key); i++)
		key[i] = tolower(key[i]);
	
    unsigned char *p = key;
	unsigned len = strlen(key);
    unsigned h = 2166136261;
    int i;

    for (i = 0; i < len; i++)
    {
        h = (h * 16777619) ^ p[i];
    }

    return h % num_lists < 0 ? h % num_lists * -1 : h % num_lists;
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
 * Add a cell to the hash map. If the cell is arleady present, update that one with this cell value
 * This method if useful if your cells have a key-value shape
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

/**
 * Return an array of all elements contained in the hash map
 */
cell* compact_map(long int *dim){
	long int total_size = 0;
	for(long int i=0; i < num_lists; i++)
		total_size += sizeList(map -> lists[i]);
	*dim = total_size;

	item e;
	cell* array = calloc(sizeof(cell),total_size);
	for (long int i = 0,current_pos = 0; i < num_lists; i++)
			for (int j = 0; j < sizeList(map -> lists[i]); j++){
				e = getItem(map -> lists[i],j);
				memcpy(&array[current_pos++],e,sizeof(cell));
				free(e);
			}
	return array;
}

void insert_order(cell* vet,int lenght, cell* c){

	for (int i = 0; i < lenght; i++)
	{
		if(compare_struct(c,&vet[i]) <= 0){
			//shift right from i
			for (int j = lenght - 1; j >= i; j--)
				memcpy(&vet[j+1],&vet[j],sizeof(cell));
			memcpy(&vet[i],c,sizeof(cell));
			return;
		}
	}
	//Is the last one
	memcpy(&vet[lenght],c,sizeof(cell));	
}

cell* compact_map_ordered(long int *dim){
	long int total_size = 0;
	for(long int i=0; i < num_lists; i++)
		total_size += sizeList(map -> lists[i]);
	*dim = total_size;

	cell* array = calloc(sizeof(cell),total_size);
	for (long int i = 0,current_pos = 0; i < num_lists; i++)
			for (int j = 0; j < sizeList(map -> lists[i]); j++){
				cell* e = getItem(map -> lists[i], j);
				insert_order(array,current_pos,e);
				current_pos++;
				free(e);
			}
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