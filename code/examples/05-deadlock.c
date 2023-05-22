/* SOLUTION:
 * This numbers results in a deadlock because in both branches the ranks are
 * stuck in MPI_Ssend as there is no matching MPI_Recv to receive the data and
 * let MPI_Ssend return. This is because every rank sends something and waits
 * for a receive. But there are no ranks listening for a message, because they
 * are all stuck sending a message waiting for the rank to receive.
 *
 * To remove the deadlock, we would flip the order of MPI_Ssend and MPI_Recv in
 * one of the branches. That way, one of the ranks will be listening and one of
 * them will be sending data and so a synchronous blocking send/receive can
 * happen successfully.
 */

#include <mpi.h>

#define ARRAY_SIZE 3

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const int comm_tag = 1;
    int numbers[ARRAY_SIZE] = {1, 2, 3};
    MPI_Status recv_status;

    if (rank == 0) {
        /* synchronous send: returns when the destination has started to
           receive the message */
        MPI_Ssend(&numbers, ARRAY_SIZE, MPI_INT, 1, comm_tag, MPI_COMM_WORLD);
        MPI_Recv(&numbers, ARRAY_SIZE, MPI_INT, 1, comm_tag, MPI_COMM_WORLD, &recv_status);
    } else {
        MPI_Ssend(&numbers, ARRAY_SIZE, MPI_INT, 0, comm_tag, MPI_COMM_WORLD);
        MPI_Recv(&numbers, ARRAY_SIZE, MPI_INT, 0, comm_tag, MPI_COMM_WORLD, &recv_status);
    }

    return MPI_Finalize();
}
