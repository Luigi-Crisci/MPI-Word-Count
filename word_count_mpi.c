#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "hash_map.h"
#include "tokenizer.h"
#define MAX_PATH 400

/**
 * Struct used to inform slaves about computation
 */
typedef struct info
{
	long int num_file, num_byte,counting;
} info;

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
			*path = (char*) malloc(strlen(optarg));
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

/**
 * Read all regular files in target directory and order them
 */
struct dirent **read_files(int *num_files, char *path)
{
	int increment, current_dim;
	increment = 10;
	current_dim = increment;
	struct dirent **files = (struct dirent**) malloc(current_dim * sizeof(struct dirent *)),*e;
	
	DIR *dir = opendir(path);
	while ((e = readdir(dir)) != NULL)
	{
		if (e->d_type != DT_REG)
			continue;
		files[*num_files] = (struct dirent*) malloc(sizeof(struct dirent));
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
	 * I order the file to have an order relationship
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
	MPI_Datatype oldtype_info[1] = {MPI_INT64_T}; 
	MPI_Type_create_struct(1, blockcount_info, displacement_info, oldtype_info, INFO);

	//Define STRING type
	MPI_Type_contiguous(30, MPI_CHAR, STRING);

	//Define CELL type
	int bloclcount_cell[2];
	MPI_Aint displacement_cell[2], lb, extent;
	MPI_Datatype oldtype_cell[2] = {*STRING, MPI_INT64_T};
	cell x; //Needed to find offsets because MPI_Get_Extent gave me incorrect results
	//For STRING field
	bloclcount_cell[0] = 1;
	displacement_cell[0] = 0;
	//For LONG INT field
	MPI_Type_get_extent(*STRING, &lb, &extent);
	bloclcount_cell[1] = 1;
	displacement_cell[1] = ((void *)&(x.value)) - ((void *)&(x.key[0]));
	MPI_Type_create_struct(2, bloclcount_cell, displacement_cell, oldtype_cell, CELL);
}

static int is_power_of_two(double num)
{
	return ceil(log2(num)) == log2(num);
}

void write_csv(){
	long int dim;
	cell* cells = compact_map_ordered(&dim);
	FILE* csv = fopen("csv_results.csv","w+");
	
	fputs("Word,Count\n",csv);
	for (int i = 0; i < dim; i++)
		fprintf(csv,"%s,%ld\n",cells[i].key,cells[i].value);
	fclose(csv);
	free(cells);
}

/**
 * Reduce results, using a tree structure to maximize slaves utilization
 */
void reduce(int num_proc, int rank, int num_bytes, MPI_Datatype CELL)
{
	long int cur_rank,level,dim_buffer,num_requests,send_proc;
	cur_rank = rank;
	level = num_requests = 0;
	dim_buffer = 1;

	cell **buffers = calloc(dim_buffer, sizeof(cell*));
	MPI_Request requests[(int)ceil(log2(num_proc))],tmp_send;

	send_proc = rank - 1; //Default behaviour
	while (cur_rank % 2 == 0 && level < ceil(log2(num_proc)))
	{
	 	//I'm the last one
		if (ceil(num_proc / pow(2, level)) == (cur_rank + 1)){
			//If I am a power of two and last one, i communicate with rank 0
			if (is_power_of_two(cur_rank))
			{
				send_proc = 0;
				break;
			}
		}
		else
		{
			if (num_requests == dim_buffer)
			{
				dim_buffer *= 2;
				buffers = realloc(buffers, sizeof(cell*) * dim_buffer);
			}

			buffers[num_requests] = calloc(MAX_WORD_NUM,sizeof(cell));
			MPI_Irecv(buffers[num_requests], MAX_WORD_NUM * sizeof(cell), CELL, (int)(rank + pow(2, level)),
					  0, MPI_COMM_WORLD, &requests[num_requests]);
			num_requests++;
		}
		cur_rank = cur_rank / 2;
		level++;
		send_proc = (int)(rank - pow(2,level));
	}
	
	//Wait to receive all requested data
	int index,received_dim;
	MPI_Status status;
	for (int i = 0; i < num_requests; i++)
	{
		MPI_Waitany(num_requests,requests,&index,&status);
		printf("RANK %d: received data from rank %d\n",rank,status.MPI_SOURCE);
		MPI_Get_count(&status,CELL,&received_dim);
		add_cell_array(buffers[index],received_dim);
		free(buffers[index]);
	}
	free(buffers);
	
	//Send my results to another processor
	if (rank != 0)
	{
		long int dim;
		cell *results = compact_map_ordered(&dim);
		printf("RANK %d: sending data to %ld\n",rank,send_proc);
		MPI_Isend(results, dim, CELL, send_proc , 0, MPI_COMM_WORLD, &tmp_send);
		MPI_Request_free(&tmp_send);
	}
}


/**
 * Perform a word count on multiple files, partitioned with bytes precision, reduced with a tree structure
 */
void count_word_parallel(struct dirent **files, int num_files,
							char *basepath, int num_proc, int rank, MPI_Datatype INFO, MPI_Datatype CELL)
{
	info *infos = (info*) malloc(sizeof(info) * num_proc), *my_info;
	my_info = (info*) malloc(sizeof(info));
	long int partitioning[num_proc];

	//Need to compute info foreach process
	if (rank == 0)
	{
		long int total_dim, dimensions[num_files], overflow, overflow_value, current_file, start_byte;
		int no_more_file = 0;
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

		//Define info data foreach slave
		for (int i = 0; i < num_proc; i++)
		{
			partitioning[i] = overflow && overflow_value-- > 0 ? total_dim / num_proc + 1 : total_dim / num_proc;
			infos[i].num_file = current_file;
			infos[i].num_byte = start_byte;

			if (i != (num_proc - 1) && !no_more_file)
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
							if((current_file + 1) < num_files){
								start_byte = 0;
								current_file++;
						}
						else
							no_more_file = 1;
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
				 * EDIT: Support for files dimension that divide num_proc
				 * BUGFIX: Added support to files which have a number of words less than number of processor
				*/
				if(start_byte != 0 && !no_more_file){
					fseek(f, start_byte - 1, SEEK_SET);
					while (!is_delimeter(c = fgetc(f)))
					{
						start_byte++;
						partitioning[i]++;
					}

					//Skip delimeters after words end
					while (is_delimeter(c = fgetc(f)))
						start_byte++;
					
					//The cutted word could be the last one
					if (feof(f))
					{
						if((current_file + 1) < num_files){
							start_byte = 0;
							current_file++;
						}
						else
							//This condition is necessary to handle a case where the number of words in the files is
							//less then the number of processos. In this case, only the first num_words processor will have a
							//word to compute, while the other ones will do nothing
							//This flag will stop the position calculation for every other processor
							no_more_file = 1;
					}
				}

				fclose(f);
			}

			infos[i].counting = partitioning[i];
			printf("Info for process %i: num_file: %ld -- count: %ld -- start_byte: %ld\n", i, infos[i].num_file, infos[i].counting,infos[i].num_byte);
		}
	}
	//Distributing info about processing to each slave
	MPI_Scatter(infos, 1, INFO, my_info, 1, INFO, 0, MPI_COMM_WORLD);

	long int bytes_to_read = my_info-> counting;
	long int current_file = my_info-> num_file;
	long int start_byte = my_info-> num_byte;

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
			sprintf(path, "%s/%s", basepath, files[current_file]-> d_name);
			file = fopen(path, "r");
			if (start_byte > 0) //Skip to start position
			{
				fseek(file, start_byte, SEEK_SET);
				start_byte = 0;
			}
		}

		readed_bytes = next_word(file, &word);
		bytes_to_read -= readed_bytes;
		//If the file endend, skip to next file
		if (word == NULL)
		{
			fclose(file);
			current_file++;
			file = NULL;
			continue;
		}

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
	reduce(num_proc, rank, my_info->counting, CELL);

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
	if( num_files == 0 ){
		fprintf(stderr,"Path is empty, please provide a path with at least 1 file\n");
		MPI_Type_free(&INFO);
		MPI_Type_free(&STRING);
		MPI_Type_free(&CELL);
		MPI_Finalize();
		return 1;
	}

	//Do the trick
	count_word_parallel(files, num_files, basepath, num_proc, rank, INFO, CELL);

	MPI_Type_free(&INFO);
	MPI_Type_free(&STRING);
	MPI_Type_free(&CELL);

	if (rank == 0)
	{
		write_csv();
		end_time = MPI_Wtime();
		timing_file = fopen("timing.log", "a");
		fprintf(timing_file, "%lf\n", end_time - start_time);
		fclose(timing_file);
	}

	free_hash_map();
	MPI_Finalize();
	return 0;
}
