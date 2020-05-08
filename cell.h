typedef struct c_cell{
	char key[30];
	int value;
} cell;

cell* new_cell();
int update_cell(cell* origin,cell* tmp);

//Item interface
int compare_struct(cell* x1, cell* x2);
int update_struct(cell* x1,cell* x2);
cell* copy_struct(cell* c);
void free_struct(cell* c);
void output_struct(cell* c);