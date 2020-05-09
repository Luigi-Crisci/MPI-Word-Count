#include<stdio.h>
#include<stdlib.h>
#include<mpi.h>
#include<unistd.h>
#include<dirent.h>
#include<string.h>
#include<getopt.h>
#include"hash_map.h"
#include"tokenizer.h"
#define MAX_PATH 400

typedef struct info{
	int num_file,num_word,counting;
}info;

typedef struct c_couple{
	int id_file,num_word;
} *couple;

void usage(){
	fprintf(stderr,"mpicc word_count_mpi --path PATH\n");
	MPI_Finalize();
	exit(0);
}

void parse_arg(int argc,char** argv,char** path){
	*path = NULL;
	static struct option long_options[] = {
		{"path",required_argument,0,'p'},
		{0,0,0,0}
	};

	int opt,opt_index;
	while( (opt = getopt_long(argc,argv,"",
								long_options,&opt_index)) != -1){
			switch (opt)
			{
				case 'p':*path = malloc(strlen(optarg)); strcpy(*path,optarg); break;
				default: usage(); break;
			}
	}
	if( *path == NULL )
		usage(); 
}

struct dirent** read_files(int *num_files,char* path){
	int increment, current_dim;
	increment = 10;
	current_dim = increment;
	struct dirent **files = malloc(current_dim *sizeof(struct dirent*));
	struct dirent *e;
	DIR* dir = opendir(path);
	while( ( e = readdir(dir)) != NULL){
		if( e -> d_type != DT_REG )
			continue;
		files[*num_files] = malloc(sizeof(struct dirent));
		memcpy(files[*num_files],e,sizeof(struct dirent));
		if( *num_files == current_dim ){
			current_dim += 10;
			files = realloc(files,sizeof(struct dirent *) * current_dim);
		}
		*num_files = (*num_files) + 1;
	}
	files = realloc(files,sizeof(struct dirent*) * (*num_files));
	
	//For some reason my workstation is unable to call qsort
	for(int i = 0; i < *num_files; i++){
		for (int j = i+1; j < *num_files; j++)
			if(strcmp(files[i]->d_name,files[j]->d_name) > 0){
				e = files[i];
				files[i] = files[j];
				files[j] = e;	
			}
	}
	return files;
}

couple* count_word(struct dirent **files,int num_files,int *count,char* basepath){
	*count = 0;
	char path[MAX_PATH],*word;
	couple *couples = malloc(sizeof(couple) * num_files);
	int local_count;
	FILE* file;
	for (int i = 0; i < num_files; i++)
	{
		local_count = 0;
		sprintf(path,"%s/%s",basepath,files[i] -> d_name);
		file = fopen(path,"r");
		while( (word = next_word(file)) != NULL){
			free(word);
			local_count++;
		}
		couples[i] = malloc(sizeof(struct c_couple));
		couples[i] -> id_file = i;
		couples[i] -> num_word = local_count;
		
		(*count) += local_count;
		fclose(file);
	}
	return couples;
}

void define_types(MPI_Datatype *CELL,MPI_Datatype *STRING,MPI_Datatype *INFO){
	//Define INFO type
	int blockcount_info[1] = {3};
	MPI_Aint displacement_info[1] = {0};
	MPI_Datatype oldtype_info[1] = {MPI_INT};
	MPI_Type_create_struct(1,blockcount_info,displacement_info,oldtype_info,INFO);

	MPI_Type_contiguous(30,MPI_CHAR,STRING);
	int bloclcount_cell[2];
	MPI_Aint displacement_cell[2],lb,extent;
	MPI_Datatype oldtype_cell[2] = {*STRING,MPI_INT};
	cell x; //Needed to find offsets because MPI_Get_Extent gave me incorrect results
	//For STRING
	bloclcount_cell[0]=1;
	displacement_cell[0]=0;
	//For INT
	MPI_Type_get_extent(*STRING,&lb,&extent);
	bloclcount_cell[1] = 1;
	displacement_cell[1] = ((void*)&(x.value)) - ((void*)&(x.key[0]));
	
	MPI_Type_create_struct(2,bloclcount_cell,displacement_cell,oldtype_cell,CELL);
}

int main(int argc, char** argv){
	int rank,num_proc,num_files = 0,num_word,*partitioning,overflow,overflow_value;
	double start_time,end_time;
	FILE* timing_file;
	char* basepath;
	MPI_Datatype INFO,CELL,STRING;

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&num_proc);

	parse_arg(argc,argv,&basepath);
	//Start counting
	start_time = MPI_Wtime();
	partitioning = malloc(sizeof(int) * num_proc);
	
	//Define CELL and STRING oldtype_cell
	define_types(&CELL,&STRING,&INFO);
	MPI_Type_commit(&INFO);
	MPI_Type_commit(&CELL);

	//Read files
	struct dirent **files = read_files(&num_files,basepath);
	info *infos,*my_info;
	my_info = malloc(sizeof(struct info));
	
	//calculate 
	if ( rank == 0){
		infos = malloc(sizeof(info) * num_proc);
		couple *couples = count_word(files,num_files,&num_word,basepath);
		for(int i = 0; i < num_files; i++){
			printf("File %d: num: %d\n",i,couples[i] -> num_word);
		}

		overflow = 0;
		if(num_word % num_proc != 0){
			overflow = 1;
			overflow_value = num_word % num_proc;
		}

		int current_file = 0;
		int start_word = 0;
		//Define info data for each slave
		for(int i = 0; i < num_proc; i++){
			partitioning[i] = overflow && overflow_value-- > 0 ? 
										num_word / num_proc + 1 :
										num_word / num_proc; 
			
			//Find which file it has to execute
			infos[i].num_file = current_file;
			infos[i].counting = partitioning[i];
			infos[i].num_word = start_word;
			printf("Info for process %i: num_file: %d -- count: %d -- start_word: %d\n",i,current_file,partitioning[i],start_word);
			
			//Calculate next file
			start_word+= partitioning[i];
			for(; current_file < num_files; current_file++){
				if(start_word <= (couples[current_file] -> num_word)){
					if(start_word == (couples[current_file] -> num_word)){
						start_word = 0;
						current_file++;
					}
					break;
				}
				start_word -= couples[current_file] -> num_word;
			}
		}
	}

	//Send information about computation to all process
	MPI_Scatter(infos,1,INFO,my_info,1,INFO,0,MPI_COMM_WORLD);

	int word_to_read = my_info -> counting;
	int current_file = my_info -> num_file;
	int start_word = my_info -> num_word;
	initialize_map(word_to_read); 
	new_hash_map();

	char path[MAX_PATH],*word;
	cell word_cell;
	word_cell.value = 1;
	FILE* file = NULL;
	while (word_to_read > 0 && current_file < num_files)
	{
		if ( file == NULL ){
			sprintf(path,"%s/%s",basepath,files[current_file] -> d_name);
			file = fopen(path,"r");
			if( start_word > 0){
				//Need to jump start_word-1 words
				while (start_word > 0){
					free(next_word(file));
					start_word--;
				}
			}
		}
		
		if( (word = next_word(file)) == NULL ){
			//If the file endend, skip to next file
			fclose(file);
			current_file++;
			file = NULL;
			continue;
		}
		//Actually add the word to my lists
		strcpy(word_cell.key,word);
		add_or_update(&word_cell);
		free(word);
		word_to_read--;
	}
	
	//Receiving and reduce results
	if(rank == 0){
		MPI_Status status;
		int dim;
		cell data[partitioning[0]];
		 for (int i = 0; i < num_proc - 1; i++){
			MPI_Recv(data,partitioning[0],CELL,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
			printf("Received results from slave %d\n",status.MPI_SOURCE);
			MPI_Get_count(&status,CELL,&dim);
			add_cell_array(data,dim);
		 }
	}
	else
	{
		int results_dim;
		cell* results = compact_map(&results_dim);
		MPI_Send(results,results_dim,CELL,0,0,MPI_COMM_WORLD); //Send my results to master
		free(results);
	}

	MPI_Type_free(&INFO);
	MPI_Type_free(&STRING);
	MPI_Type_free(&CELL);

	if (rank == 0){
		end_time = MPI_Wtime();
		timing_file = fopen("timing.log","a");
		fprintf(timing_file,"%lf\n",end_time - start_time);
		fclose(timing_file);
	}
		
	free_hash_map();
	MPI_Finalize();
	return 0;
}
