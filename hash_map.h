#include"cell.h"
#define DEFAULT_LIST 1000
#define MAX_WORD_NUM 100000

void new_hash_map();
void new_hash_map_array(cell*,long int);
int add_or_update(cell* c);
cell* compact_map(long int*);
cell* compact_map_ordered(long int*);
void initialize_map(long int);
void output_hash_map();
void free_hash_map();
void add_cell_array(cell*,long int);