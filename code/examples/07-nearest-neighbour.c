#include <math.h>
#include <mpi.h>
#include <stdio.h>

#define ROWS 4
#define COLS 4

int main(int argc, char **argv)
{
    int rank, size, left, right, up, down;
    int data[ROWS][COLS];
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Compute the rank of the left, right, up, and down neighbors
    left = (rank - 1 + size) % size;
    right = (rank + 1) % size;
    up = (rank - sqrt(size) + size) % size;
    down = (rank + sqrt(size)) % size;

    // Assign values to the local subset of the array
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            data[i][j] = rank;
        }
    }

    // Send and receive data with the neighboring ranks
    MPI_Sendrecv_replace(&data, ROWS * COLS, MPI_INT, right, 0, left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Sendrecv_replace(&data, ROWS * COLS, MPI_INT, left, 0, right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Sendrecv_replace(&data, ROWS * COLS, MPI_INT, down, 0, up, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Sendrecv_replace(&data, ROWS * COLS, MPI_INT, up, 0, down, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Print the received data
    printf("Process %d received data:\n", rank);
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            printf("%d ", data[i][j]);
        }
        printf("\n");
    }

    MPI_Finalize();
    return 0;
}