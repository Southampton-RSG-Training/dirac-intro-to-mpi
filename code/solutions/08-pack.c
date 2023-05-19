#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define PACK_BUFFER_SIZE 50

int main(int argc, char **argv)
{
    int my_rank, num_ranks;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    int num_rows = 3, num_cols = 3;

    /* Allocate and initialize a 2D array with the number of the element */
    int **matrix = malloc(num_rows * sizeof(int *));
    for (int i = 0; i < num_rows; ++i) {
        matrix[i] = malloc(num_cols * sizeof(int));
        for (int j = 0; j < num_cols; ++j) {
            matrix[i][j] = num_cols * i + j;
        }
    }

    if (my_rank == 0) {
        /* Create the pack buffer and pack each row of data into it buffer
           one by one */
        int position = 0;
        int *packed_data = malloc(PACK_BUFFER_SIZE * sizeof(int));
        for (int i = 0; i < num_rows; ++i) {
            MPI_Pack(matrix[i], num_cols, MPI_INT, packed_data, PACK_BUFFER_SIZE, &position, MPI_COMM_WORLD);
        }

        /* Send the packed data to rank 1 and free memory we no longer need */
        MPI_Send(packed_data, PACK_BUFFER_SIZE, MPI_PACKED, 1, 0, MPI_COMM_WORLD);
        free(packed_data);
    } else {
        /* Create a receive buffer and get the packed buffer from rank 0 */
        int *received_data = malloc(PACK_BUFFER_SIZE * sizeof(int));
        MPI_Recv(received_data, PACK_BUFFER_SIZE, MPI_PACKED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        /* allocate a matrix to put the receive buffer into -- this is for
           demonstration purposes */
        int **my_matrix = malloc(num_rows * sizeof(int *));
        for (int i = 0; i < num_cols; ++i) {
            my_matrix[i] = malloc(num_cols * sizeof(int));
        }

        /* Unpack the received data row by row into my_matrix */
        int position = 0;
        for (int i = 0; i < num_rows; ++i) {
            MPI_Unpack(received_data, PACK_BUFFER_SIZE, &position, my_matrix[i], num_cols, MPI_INT, MPI_COMM_WORLD);
        }
        free(received_data);

        /* Print the elements of my_matrix */
        printf("Rank 1 received the following array:\n");
        for (int i = 0; i < num_rows; ++i) {
            for (int j = 0; j < num_cols; ++j) {
                printf(" %d", my_matrix[i][j]);
            }
            printf("\n");
        }
        for (int i = 0; i < num_rows; ++i) {
            free(my_matrix[i]);
        }
        free(my_matrix);
    }

    /* Free all the memory for matrix */
    for (int i = 0; i < num_rows; ++i) {
        free(matrix[i]);
    }
    free(matrix);

    return MPI_Finalize();
}