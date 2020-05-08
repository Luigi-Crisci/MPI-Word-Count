#include"item.h"
#include"cell.h"

item copy_item(item e){
    return copy_struct(e);
}

int compare_item(item x1,item x2){
    return compare_struct(x1,x2);
}

int update_item(item x1, item x2){
    return update_struct(x1,x2);
}

void free_item(item e){
    free_struct(e);
}

void output_item(item e){
    output_struct(e);
}