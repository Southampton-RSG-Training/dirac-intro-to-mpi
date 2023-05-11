#include <mpi.h>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int buffer[3][3] = {0, 1, 3, 4, 5, 6, 7, 8};

    MPI_Datatype row_t; /* our new type is a MPI_Datatype */

    MPI_Type_vector(3,       /* count is the length of the row */
                    1,       /* blocklength says how many rows we want to include */
                    3,       /* stride is the number of elements  */
                    MPI_INT, /* oldtype states the datatype of the vector */
                    &row_t   /* a pointer to initialize our new datatype */
    );

    MPI_Type_commit(&row_t); /* our new type has to be commited to prepare it for use */

    return MPI_Finalize();
}
