# An MPI implementation of Word Count Problem  

This is an MPI implementation of *Word Count* problem made with MPI (OpenMPI implementation), made as a final project for the course *Programmazione Concorrente e Parallela su Cloud, 2020, Unisa.*  

Please go to *resources* sub-dir to find full documentation (in *Italian*) about this solution.

---------  

## Build info

To build this solution just type:  

> make  

To use this on a cluster:

> mpirun -np NUM_PROCESSOR --hostfile HOSTFILE word_count_mpi.out --path PATH_TO_FILES_TO_ANALYZE

To use on a single machine:  

> mpirun -n NUM_PROCESSOR word_count_mpi.out --path PATH_TO_FILES_TO_ANALYZE

-------------  

## Documentation info

Documentation about this solution is written in *Pandoc Markdown*, so some feature may not look good in GitHub Markdown View. In the same folder you can find a *pdf* already compiled, or you can build it on your own by using the compile script on a Unix-like system:

> bash compile.sh

## Authors
 - Luigi Crisci