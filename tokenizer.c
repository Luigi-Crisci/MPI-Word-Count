#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include"tokenizer.h"

static char *default_delimiters = "/home/peppe/vscode_pcpc_workspace/WordCount/delimiters.txt";
static char end_lines[4] = {'\n','\t','\r',' '};
static char *delimiters = NULL;
static int delimeters_size;

int define_delimiters(char *path)
{
	FILE *file;
	if ((file = fopen(path, "r")) == NULL)
		return 0;

	char c;
	int local_dim;
	delimeters_size = DEFAULT_DELIMITER_SIZE;
	delimiters = calloc(delimeters_size, sizeof(char));
	memcpy(delimiters,end_lines,4 * sizeof(char));
	for (local_dim = 4; (c = fgetc(file)) != EOF; local_dim++)
	{
		if (local_dim == delimeters_size)
			delimiters = realloc(delimiters, (delimeters_size += 10));
		delimiters[local_dim] = c;
	}

	if (local_dim > delimeters_size)
	{
		delimiters = realloc(delimiters, local_dim);
		delimeters_size = local_dim;
	}
	return 1;
}

int is_delimeter(char c)
{
	if (delimiters == NULL)
		define_delimiters(default_delimiters);
	for (int i = 0; i < delimeters_size; i++)
		if (delimiters[i] == c)
			return 1;
	return 0;
}

int next_word(FILE *file,char** word)
{

	if (delimiters == NULL)
		define_delimiters(default_delimiters);

	int lenght = 0, check,bytes_read;
	char c;
	*word = malloc(MAX_WORD_LENGHT);
	strcpy(*word, "");
	bytes_read = 0;
	while ((c = fgetc(file)) != EOF)
	{
		bytes_read++;
		check = is_delimeter(c);
		if (check)
			if (strlen(*word) == 0)
				continue;
			else
			{
				(*word)[lenght] = '\0';
				return bytes_read;
			}
		(*word)[lenght++] = c;
	}

	if(strlen(*word) == 0)
		*word = NULL;
	else
		(*word)[lenght] = '\0';
	return bytes_read;
	
}