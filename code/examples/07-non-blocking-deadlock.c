#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int my_rank;
    int num_ranks;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    if (num_ranks != 2) {
        if (my_rank == 0) {
            printf("This example requires two ranks\n");
        }
        MPI_Finalize();
        return 1;
    }

    int numbers[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    MPI_Status status;
    MPI_Request send_req, recv_req;

    if (my_rank == 0) {
        MPI_Isend(numbers, 8, MPI_INT, 1, 0, MPI_COMM_WORLD, &send_req);
        MPI_Irecv(numbers, 8, MPI_INT, 1, 0, MPI_COMM_WORLD, &recv_req);
    } else {
        // MPI_Isend(numbers, 8, MPI_INT, 0, 0, MPI_COMM_WORLD, &send_req);
        MPI_Irecv(numbers, 8, MPI_INT, 0, 0, MPI_COMM_WORLD, &recv_req);
    }

    MPI_Wait(&send_req, &status);
    MPI_Wait(&recv_req, &status);

    return MPI_Finalize();
}