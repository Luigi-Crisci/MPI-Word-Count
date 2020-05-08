typedef void* item;
#define NULLITEM NULL

int compare_item(item x1,item x2);
int update_item(item to, item from);
item copy_item(item e);
void free_item(item e);
void output_item(item e);