word_count_mpi.out: word_count_mpi.o hash_map.o list_t.o item.o cell.o tokenizer.o
	mpicc word_count_mpi.o hash_map.o list_t.o item.o cell.o tokenizer.o -lm -o word_count_mpi.out

hash_map.o: hash_map.c list_t.o item.o cell.o
	gcc -c hash_map.c

list_t.o: list_t.c item.o cell.o
	gcc -c list_t.c

item.o: item.c cell.o
	gcc -c item.c

cell.o: cell.c
	gcc -c cell.c

tokenizer.o: tokenizer.c
	gcc -c tokenizer.c

word_count_mpi.o: word_count_mpi.c hash_map.o list_t.o item.o cell.o tokenizer.o
	mpicc -c word_count_mpi.c

clean:
	rm *.o