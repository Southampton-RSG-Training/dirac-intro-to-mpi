#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int my_rank;
    int num_ranks;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    int matrix[4][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16},
    };

    MPI_Datatype row_t;
    MPI_Type_vector(2, 4, 8, MPI_INT, &row_t);
    MPI_Type_commit(&row_t);

    if (my_rank == 0) {
        MPI_Send(matrix, 1, row_t, 1, 0, MPI_COMM_WORLD);
    } else {
        int buffer[8];
        MPI_Status status;
        MPI_Recv(buffer, 8, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        printf("Rank %d received the following: ", my_rank);
        for (int i = 0; i < 8; ++i) {
            printf(" %d", buffer[i]);
        }
        printf("\n");
    }

    return MPI_Finalize();
}
