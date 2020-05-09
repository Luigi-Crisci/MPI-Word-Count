#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "hash_map.h"
#include "tokenizer.h"
#define MAX_PATH 400

typedef struct info
{
	int num_file, num_byte, counting;
} info;

typedef struct c_couple
{
	int id_file, num_word;
} * couple;

void usage()
{
	fprintf(stderr, "mpicc word_count_mpi --path PATH\n");
	MPI_Finalize();
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

void define_types(MPI_Datatype *CELL, MPI_Datatype *STRING, MPI_Datatype *INFO)
{
	//Define INFO type
	int blockcount_info[1] = {3};
	MPI_Aint displacement_info[1] = {0};
	MPI_Datatype oldtype_info[1] = {MPI_INT};
	MPI_Type_create_struct(1, blockcount_info, displacement_info, oldtype_info, INFO);

	MPI_Type_contiguous(30, MPI_CHAR, STRING);
	int bloclcount_cell[2];
	MPI_Aint displacement_cell[2], lb, extent;
	MPI_Datatype oldtype_cell[2] = {*STRING, MPI_INT};
	cell x; //Needed to find offsets because MPI_Get_Extent gave me incorrect results
	//For STRING
	bloclcount_cell[0] = 1;
	displacement_cell[0] = 0;
	//For INT
	MPI_Type_get_extent(*STRING, &lb, &extent);
	bloclcount_cell[1] = 1;
	displacement_cell[1] = ((void *)&(x.value)) - ((void *)&(x.key[0]));

	MPI_Type_create_struct(2, bloclcount_cell, displacement_cell, oldtype_cell, CELL);
}

couple *count_word_parallel(struct dirent **files, int num_files,
							char *basepath, int num_proc, int rank, MPI_Datatype INFO, MPI_Datatype CELL)
{
	info *infos = malloc(sizeof(info) * num_proc), *my_info;
	my_info = malloc(sizeof(struct info));
	long int partitioning[num_proc];

	//Need to compute info foreach process
	if (rank == 0)
	{
		long int total_dim, dimensions[num_files], overflow, overflow_value, current_file, start_byte;
		char c;
		struct stat s;
		FILE *f;
		char path[MAX_PATH];
		total_dim = overflow = current_file = start_byte = 0;

		//Get dimension for each file
		for (int i = 0; i < num_files; i++)
		{
			sprintf(path, "%s/%s", basepath, files[i]->d_name);
			stat(path, &s);
			dimensions[i] = s.st_size;
			total_dim += dimensions[i];
			printf("File %s -- dimension %ld\n", files[i]->d_name, dimensions[i]);
		}

		printf("Num files: %d -- total bytes: %ld -- partitioning: %ld\n", num_files, total_dim, total_dim / num_proc);

		//Handle non-perfect divisior
		if (total_dim % num_proc != 0)
		{
			overflow = 1;
			overflow_value = total_dim % num_proc;
		}

		//Define info data for each slave
		for (int i = 0; i < num_proc; i++)
		{
			partitioning[i] = overflow && overflow_value-- > 0 ? total_dim / num_proc + 1 : total_dim / num_proc;

			infos[i].num_file = current_file;
			infos[i].num_byte = start_byte;

			if (i != (num_proc - 1))
			{
				/**
			 * We compute the start byte for next slave. Because there is an order relationship between files,
			 * I treat the files as a contiguous array of bytes. To find the next bytes we just find the position we reach
			 * in the virtual array, than translating that to the actual file it belongs
			 */
				start_byte += partitioning[i];
				for (; current_file < num_files; current_file++)
				{
					if (start_byte <= dimensions[current_file])
					{
						if (start_byte == dimensions[current_file])
						{
							start_byte = 0;
							current_file++;
						}
						break;
					}
					start_byte -= dimensions[current_file];
				}

				//Need to check if the current byte cuts a word
				sprintf(path, "%s/%s", basepath, files[current_file]->d_name);
				f = fopen(path, "r");
				
				/**
				 * The byte cuts a word only if the previous character IS NOT a delimiter, 
				 * because it could be the start of a new word otehrwise. So I skip to start_byte - 1
				 * and consume each character of the cutted word that will be computed to the previous slave
				*/
				fseek(f, start_byte - 1, SEEK_SET);
				while (!is_delimeter(c = fgetc(f)))
				{
					start_byte++;
					partitioning[i]++;
				}

				//The cutted word could be the last one
				if (feof(f))
				{
					start_byte = 0;
					current_file++;
				}
			}

			infos[i].counting = partitioning[i];
			printf("Info for process %i: num_file: %ld -- count: %ld -- start_byte: %d\n", i, current_file, partitioning[i], infos[i].num_byte);
		}
	}
	//Distributing info about processing to each slave
	MPI_Scatter(infos, 1, INFO, my_info, 1, INFO, 0, MPI_COMM_WORLD);

	long int bytes_to_read = my_info->counting;
	int current_file = my_info->num_file;
	long int start_byte = my_info->num_byte;

	char path[MAX_PATH], *word;
	long int readed_bytes;
	FILE *file = NULL;
	cell word_cell;
	word_cell.value = 1;

	initialize_map(bytes_to_read / MAX_WORD_LENGHT);
	new_hash_map();
	while (bytes_to_read > 0 && current_file < num_files)
	{
		//Open the file for first time, or when I switch
		if (file == NULL)
		{
			sprintf(path, "%s/%s", basepath, files[current_file]->d_name);
			file = fopen(path, "r");
			if (start_byte > 0) //Skip to start position
			{
				fseek(file, start_byte, SEEK_SET);
				start_byte = 0;
			}
		}

		readed_bytes = next_word(file, &word);
		//If the file endend, skip to next file
		if (word == NULL)
		{
			fclose(file);
			current_file++;
			file = NULL;
			continue;
		}
		bytes_to_read -= readed_bytes;
		/**
		 * Because next_word() trim the delimeter before next word,
		 * when the bytes_to_read end with a delimeterm, next_word() reads also
		 * the word after the end of my portion. To avoid to count that twice, we just jump that one
		 */
		if (bytes_to_read >= 0)
		{
			//Actually add the word to the hash map
			strcpy(word_cell.key, word);
			add_or_update(&word_cell);
		}

		free(word);
	}

	//Receiving and reduce results
	if (rank == 0)
	{
		MPI_Status status;
		int dim;
		//I use partitioning[0] beause is alwayis the biggest partition possible
		cell *data = malloc(sizeof(cell) * partitioning[0]);
		//Receive a results from each slave
		for (int i = 0; i < num_proc - 1; i++)
		{
			MPI_Recv(data, partitioning[0], CELL, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			printf("Received results from slave %d\n", status.MPI_SOURCE);
			MPI_Get_count(&status, CELL, &dim);
			add_cell_array(data, dim);
		}
	}
	else
	{
		long int results_dim;
		cell *results = compact_map(&results_dim);
		MPI_Send(results, results_dim, CELL, 0, 0, MPI_COMM_WORLD); //Send my results to master
		free(results);
	}
}

int main(int argc, char **argv)
{
	int rank, num_proc, num_files = 0, num_word, *partitioning, overflow, overflow_value;
	double start_time, end_time;
	FILE *timing_file;
	char *basepath;
	MPI_Datatype INFO, CELL, STRING;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

	parse_arg(argc, argv, &basepath);
	//Start counting
	start_time = MPI_Wtime();

	//Define CELL and STRING oldtype_cell
	define_types(&CELL, &STRING, &INFO);
	MPI_Type_commit(&INFO);
	MPI_Type_commit(&CELL);

	//Read files
	struct dirent **files = read_files(&num_files, basepath);
	//Do the trick
	count_word_parallel(files, num_files, basepath, num_proc, rank, INFO, CELL);

	MPI_Type_free(&INFO);
	MPI_Type_free(&STRING);
	MPI_Type_free(&CELL);

	if (rank == 0)
	{
		// output_hash_map();
		end_time = MPI_Wtime();
		timing_file = fopen("timing.log", "a");
		fprintf(timing_file, "%lf\n", end_time - start_time);
		fclose(timing_file);
	}




	free_hash_map();
	MPI_Finalize();
	return 0;
}
