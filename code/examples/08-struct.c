#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int my_rank;
    int num_ranks;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    if (num_ranks != 2) {
        if (my_rank == 0) {
            printf("This example only works with 2 ranks\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    struct MyStruct {
        int id;
        double value;
    } foo = {.id = 0, .value = 3.1459};

    int block_lengths[2] = {1, 1};
    MPI_Datatype block_types[2] = {MPI_INT, MPI_DOUBLE};

    MPI_Aint base_address;
    MPI_Aint block_offsets[2];
    MPI_Get_address(&foo, &base_address);
    MPI_Get_address(&foo.id, &block_offsets[0]);
    MPI_Get_address(&foo.value, &block_offsets[1]);
    for (int i = 0; i < 2; ++i) {
        block_offsets[i] = MPI_Aint_diff(block_offsets[i], base_address);
    }

    MPI_Datatype struct_type;
    MPI_Type_create_struct(2, block_lengths, block_offsets, block_types, &struct_type);
    MPI_Type_commit(&struct_type);

    if (my_rank == 0) {
        MPI_Send(&foo, 1, struct_type, 1, 0, MPI_COMM_WORLD);
    } else {
        struct MyStruct recv_struct;
        MPI_Recv(&recv_struct, 1, struct_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Received DataPoint: id = %d value = %f\n", recv_struct.id, recv_struct.value);
    }

    MPI_Type_free(&struct_type);

    return MPI_Finalize();
}
