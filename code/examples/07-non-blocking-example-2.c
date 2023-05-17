#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MESSAGE_SIZE 32
#define DEFAULT_TAG 0

int main(int argc, char **argv)
{
    int my_rank;
    int num_ranks;
    char send_message[MESSAGE_SIZE];
    char recv_message[MESSAGE_SIZE];
    MPI_Request send_request;
    MPI_Request recv_request;
    MPI_Status recv_status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (num_ranks != 2) {
        printf("This example requires at least two ranks\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    const int num_chars = sprintf(send_message, "Hello from rank %d!", my_rank);
    const int right_rank = (my_rank + 1) % num_ranks;

    int left_rank = my_rank - 1;
    if (left_rank < 0)
        left_rank = num_ranks - 1;

    MPI_Irecv(recv_message, MESSAGE_SIZE, MPI_CHAR, left_rank, DEFAULT_TAG, MPI_COMM_WORLD, &recv_request);
    MPI_Isend(send_message, MESSAGE_SIZE, MPI_CHAR, right_rank, DEFAULT_TAG, MPI_COMM_WORLD, &send_request);

    int recv_flag = false;
    MPI_Test(&recv_request, &recv_flag, &recv_status);
    while (recv_flag == false) {
        MPI_Test(&recv_request, &recv_flag, &recv_status);
    }

    printf("Rank %d: message received -- %s\n", my_rank, recv_message);

    return MPI_Finalize();
}
