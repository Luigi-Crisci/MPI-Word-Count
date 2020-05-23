#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include "hash_map.h"
#include "tokenizer.h"

void usage()
{
	fprintf(stderr, "mpicc word_count_mpi --path PATH\n");
	exit(0);
}

void parse_arg(int argc, char **argv, char **path)
{

	*path = NULL;
	static struct option long_options[] = {
		{"path", required_argument, 0, 'p'},
		{0, 0, 0, 0}};

	int opt, opt_index;
	while ((opt = getopt_long(argc, argv, "",
							  long_options, &opt_index)) != -1)
	{
		switch (opt)
		{
		case 'p':
			*path = malloc(strlen(optarg));
			strcpy(*path, optarg);
			break;
		default:
			usage();
			break;
		}
	}
	if (*path == NULL)
		usage();
}

struct dirent **read_files(int *num_files, char *path)
{
	int increment, current_dim;
	increment = 10;
	current_dim = increment;
	struct dirent **files = malloc(current_dim * sizeof(struct dirent *));
	struct dirent *e;
	DIR *dir = opendir(path);
	while ((e = readdir(dir)) != NULL)
	{
		if (e->d_type != DT_REG)
			continue;
		files[*num_files] = malloc(sizeof(struct dirent));
		memcpy(files[*num_files], e, sizeof(struct dirent));
		if (*num_files == current_dim)
		{
			current_dim += 10;
			files = realloc(files, sizeof(struct dirent *) * current_dim);
		}
		*num_files = (*num_files) + 1;
	}
	files = realloc(files, sizeof(struct dirent *) * (*num_files));

	/**
	 * For some reason my workstation is unable to call qsort
	 * I order the file ho have an order relationship
	 */
	for (int i = 0; i < *num_files; i++)
	{
		for (int j = i + 1; j < *num_files; j++)
			if (strcmp(files[i]->d_name, files[j]->d_name) > 0)
			{
				e = files[i];
				files[i] = files[j];
				files[j] = e;
			}
	}
	return files;
}

int main(int argc, char **argv)
{
	char path[400], *basepath;
	int num_files = 0;
	FILE *fc;
	parse_arg(argc, argv, &basepath);
	struct timeval start, end;

	gettimeofday(&start,0);
	
	struct dirent **files = read_files(&num_files, basepath);

	initialize_map(100000); //Just get the maximun amount of lists
	new_hash_map();
	cell tmp;
	tmp.value = 1;

	for (int i = 0; i < num_files; i++)
	{
		sprintf(path, "%s/%s", basepath, files[i]->d_name);
		fc = fopen(path, "r");
		char *word;
		while ((next_word(fc, &word)) != 0 && word != NULL)
		{
			strcpy(tmp.key, word);
			add_or_update(&tmp);
			free(word);
		}
		fclose(fc);
	}

	long int dim;
	cell* c = compact_map_ordered(&dim);
	for (int i = 0; i < dim; i++)
	{
		output_struct(&c[i]);
	}
	
	free_hash_map();

	gettimeofday(&end,0);
	long seconds = end.tv_sec - start.tv_sec;
	long microseconds = ((seconds * 1000000) + end.tv_usec) - start.tv_usec;
	printf("Elapsed time: %ld\n",microseconds);

	return 0;
}