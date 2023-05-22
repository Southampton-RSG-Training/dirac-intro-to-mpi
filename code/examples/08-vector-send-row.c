#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int my_rank;
    int num_ranks;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    int matrix[3][3] = {
        {0, 1, 2},
        {3, 4, 5},
        {6, 7, 8},
    };

    int row_size = 3;
    MPI_Datatype row_t; /* our new type is a MPI_Datatype */
    MPI_Type_vector(1, 3, 3, MPI_INT, &row_t);
    MPI_Type_commit(&row_t); /* our new type has to be commited to prepare it for use */

    if (my_rank == 0) {
        MPI_Send(&matrix[1][0], 1, row_t, 1, 0, MPI_COMM_WORLD);
    } else {
        int buffer[3];
        MPI_Status status;
        MPI_Recv(buffer, row_size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        printf("Rank %d received the following: ", my_rank);
        for (int i = 0; i < row_size; ++i) {
            printf(" %d", buffer[i]);
        }
        printf("\n");
    }

    return MPI_Finalize();
}
