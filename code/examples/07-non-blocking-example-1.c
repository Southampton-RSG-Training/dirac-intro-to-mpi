#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

#define ARRAY_SIZE 5

int main(int argc, char **argv)
{
    int my_rank;
    int num_ranks;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (num_ranks != 2) {
        if (my_rank == 0) {
            printf("This example requires two ranks\n");
        }
        MPI_Finalize();
        return 1;
    }

    MPI_Status status;
    MPI_Request request;

    int buffer[ARRAY_SIZE];

    if (my_rank == 0) {
        /* Fill the buffer with some junk */
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            buffer[i] = my_rank + i;
        }

        /* Send it to rank 1*/
        MPI_Isend(buffer, ARRAY_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
        /* Wait, to ensure the buffer is ready for re-use */
        MPI_Wait(&request, &status);
    } else {
        MPI_Irecv(buffer, ARRAY_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);

        for (int i = 0; i < ARRAY_SIZE; ++i) {
            buffer[i] = my_rank + i;
        }

        MPI_Wait(&request, &status);
    }

    return MPI_Finalize();
}
