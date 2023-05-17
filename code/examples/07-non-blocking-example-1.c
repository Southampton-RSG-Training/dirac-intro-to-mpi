#include <mpi.h>
#include <stdio.h>

#define MESSAGE_SIZE 16
#define DEFAULT_TAG 0

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int my_rank;
    int num_ranks;
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (num_ranks < 2) {
        printf("This example requires at least two ranks\n");
        MPI_Finalize();
        return 1;
    }

    MPI_Request request;
    char message[MESSAGE_SIZE] = "Hello, world!";

    if (my_rank == 0) {
        for (int i = 1; i < num_ranks; ++i) {
            MPI_Isend(message, MESSAGE_SIZE, MPI_CHAR, i, DEFAULT_TAG, MPI_COMM_WORLD, &request);
        }
    } else {
        MPI_Status status;
        MPI_Irecv(message, MESSAGE_SIZE, MPI_CHAR, 0, DEFAULT_TAG, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
        printf("Rank %d: message %s\n", my_rank, message);
    }

    return MPI_Finalize();
}
