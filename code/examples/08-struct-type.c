#include <mpi.h>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    struct person_t {
        int age;          // 1
        double height_cm; // 1
        char name[64];    // 64
    };

    MPI_Datatype person_t_types = {MPI_INT, MPI_DOUBLE, MPI_CHAR};

    return MPI_Finalize();
}