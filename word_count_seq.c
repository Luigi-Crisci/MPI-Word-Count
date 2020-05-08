#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<dirent.h>
#include<string.h>
#include"hash_map.h"
#include"tokenizer.h"

int main(){
	FILE* fc = fopen("/home/peppe/vscode_pcpc_workspace/project/tmp/testfile","r+");
	int count = 0;

	initialize_map(100000);	
	new_hash_map();

	char* word;
	while ((next_word(fc,&word)) != 0)
	{
		cell tmp;
		strcpy(tmp.key, word);
		tmp.value = 1;
		add_or_update(&tmp);
		free(word);
	}
	
	output_hash_map();
	
	free_hash_map();

	return 0;
}