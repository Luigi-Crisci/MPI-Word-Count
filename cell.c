#include<string.h>
#include<stdlib.h>
#include"cell.h"
#include<stdio.h>

cell* new_cell(){
	return malloc(sizeof(cell));
}

int update_cell(cell* to, cell* from){
	if(from == NULL || to == NULL || strcmp(to->key,from->key) != 0 )
		return 0;
	to -> value = to ->value + from -> value;
	return 1; 
}

//Item interface
int compare_struct(cell *x1, cell *x2){
	return strcmp(x1 -> key, x2 -> key);
}

int update_struct(cell *x1,cell *x2){
	return update_cell(x1,x2);
}

cell* copy_struct(cell *e){
	cell* copy = new_cell();
	*copy = *e;
	strcpy(copy->key,e->key);
	return copy;
}

void free_struct(cell* e){
	free(e);
}

void output_struct(cell *c){
	printf("Key: %s -- Value: %d\n", c -> key, c -> value);
}