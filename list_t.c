#include"item.h"
#include<stdio.h>
#include<stdlib.h>
#include"list_t.h"

struct node{
	item e;
	struct node* next;
};

struct c_list{
	struct node* first;
	int n;
};

/**
 * pre: true
 * post: l -> first == NULL && l -> n == 0 
 */
list newList(){
	list l = malloc(sizeof(struct c_list));
	if(l != NULL){
		l -> first = NULL;
		l -> n = 0; 
	}
	return l;
}

/**
 * 	emptyList(list) -> b
 *	pre: l != NULL
 *  post: b == 1 se l -> first == NULL, b == 0 altrimenti
 */
int emptyList(list l){
	if(l == NULL)
		return 0;
	return (l -> n == 0) ? 1 : 0;
}

int sizeList(list l){
	return l -> n;
}

static struct node* consList(struct node* l,item e){
	struct node *new = malloc(sizeof(struct node));
	if( new == NULL)
		return NULL;
	new -> e = copy_item(e);
	new -> next = l;
	return new;
}

static struct node* tailList(struct node* l){
	return l -> next;
}

/**
 * pre: 0 <= pos <= sizeList(l) && e != NULLITEM 
 * 	 l != NULL
 * post: 
 */
static struct node* insertItem(struct node *l, int pos, item e){
	if(pos == 0)
		return consList(l,e);
	
	struct node* tmp = l, *prec;
	int i=0;
	while(tmp != NULL && i < pos){
		i++;
		prec = tmp;
		tmp = tailList(tmp);
	}

	if(tmp == NULL){
		struct node *last = malloc(sizeof(struct node));
		last -> e = copy_item(e);
		last -> next = NULL;
	
		prec -> next = last;
	}
	else{
		tmp = consList(tmp,e);
		prec -> next = tmp;
	}

	return l;
}

/**
 * pre: 0 <= pos <= sizeList(l) && e != NULLITEM 
 * 		&& l = <a1,a2,...,an>, n>=0  
 * post: l = <a1,a2,....,a_pos-1,a_e,a_pos+1,...,an>, n > 0
 */
int insertList(list l, int pos, item e){
	if(l == NULL)
		return -1;
	struct node* tmp = insertItem(l -> first,pos,e);
	if( tmp == NULL)
		return 0;
	l -> first = tmp;
	l -> n++;
	return 1;
}


static struct node* removeItem(struct node* l, int pos){
	struct node* tmp,*prec;
	if(pos == 0){
		tmp = tailList(l);
		free(l);
		return tmp;
	}

	tmp = l;
	int i = 0;
	while (tmp != NULL && i < pos ){
		i++;
		prec = tmp;
		tmp = tailList(tmp);
	}

	if(tmp == NULL)
		return NULL;
	else{
		prec -> next = tailList(tmp);
		free_item(tmp -> e);
		free(tmp);
	}

	return l;
}

/**
 * pre: l != NULL && 0 <= pos <= sizeList(l)
 * 		l = <a1,a2,..,a_pos,..,an>, n>0
 * post: l = <a1,a2,...,a_pos-1,a_pos+1,...,an>, n >=0
 */
int removeList(list l, int pos){
	if (l == NULL)
		return 0;
	struct node* tmp = removeItem(l->first,pos);
	l -> first = tmp;
	l -> n--;
	return 1;
}


/** 
 * cloneList(l) -> l1
 * pre: !emptyList(l)
 * post: l1 = l
 */
list cloneList(list l){
	list l1 = newList();
	struct node* tmp = l -> first;
	while(tmp != NULL){
		if(insertList(l1,l1 -> n,tmp -> e) == 0){
			freeList(l1);
			return NULL;
		}
		tmp = tmp -> next;
	}

	return l1;
}

item getFirst(list l){
	return emptyList(l) ? NULLITEM : l -> first -> e;
}

item getItem(list l, int pos){
	if(pos < 0 || pos >= l -> n)
		return NULLITEM;

	struct node* tmp = l -> first;
	for (int i = 0; i < pos; i++)
		tmp = tailList(tmp);

	return copy_item(tmp -> e);
}

/**
 * mergeList(l1,l2) -> l3
 * pre: l1 = <a1,a2,....,an>, l2 = <b1,b2,...,bm>, n>0, m>0
 * post: l3 = <a1,a2,...,an,b1,b2,...,bm>
 */
list mergeList(list l1,list l2){
	list l3=newList();
	struct node* tmp1=l1->first;
	struct node* tmp2=l2->first;
	for(int i=0;i<l1->n;i++){
			insertList(l3,l3->n,tmp1->e);
			tmp1=tmp1->next;
	}

	for(int i=0;i<l2->n;i++){
			insertList(l3,l3->n,tmp2->e);
			tmp2=tmp2->next;
	}

	return l3;
}

void freeList(list l){
	while(!emptyList(l))
		removeList(l,0);
	free(l);
}

int searchItem(list l, item e){
	item tmp;
	for (int i = 0; i < sizeList(l); i++)
	{
		tmp = getItem(l,i);
		if(compare_item(tmp,e) == 0)
			return i;
	}
	return -1;
}

/**
 * pre: 0 <= pos <= sizeList(l) && e != NULLITEM 
 * 	 l != NULL
 * post: 
 */
static int updateItem(struct node *l,int pos,item e){
	struct node* tmp = l, *prec;
	int i=0;
	while(tmp != NULL && i < pos){
		i++;
		prec = tmp;
		tmp = tailList(tmp);
	}

	return update_item(tmp -> e, e);
}
/**
 * Update an item within the list, if exists
 * This method is essential when an item have the form of <key,value>, or it just has a param that can
 * be updated and some kind of fixed param that can be used to identify it 
*/
int updateList(list l, item e){
	int pos;
	if( e == NULL || (pos = searchItem(l,e)) == -1 )
		return 0;
	return updateItem(l -> first,pos,e);
}

void outputList(list l){
	for (int i = 0; i < l -> n; i++)
	{
		printf("Elemento %d:\n",i);
		output_item(getItem(l,i));
	}
}


