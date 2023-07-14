#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MESSAGE_LENGTH 32

int main(int argc, char **argv)
{
    int my_rank;
    int num_ranks;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    int sum = 0;
    MPI_Status status;

    if (my_rank == 0) {
        for (int i = 1; i < num_ranks; ++i) {
            int recv_num = 0;
            MPI_Recv(&recv_num, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            sum += recv_num;
        }
        for (int i = 1; i < num_ranks; ++i) {
            MPI_Send(&sum, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Send(&my_rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Recv(&sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }

    // sum = my_rank;
    // if (my_rank == 0) {
    //     MPI_Reduce(MPI_IN_PLACE, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    // } else {
    //     MPI_Reduce(&sum, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    // }
    // MPI_Bcast(&sum, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // MPI_Allreduce(MPI_IN_PLACE, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    printf("Rank %d has a sum of %d\n", my_rank, sum);

    return MPI_Finalize();
}
