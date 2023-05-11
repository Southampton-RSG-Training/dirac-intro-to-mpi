---
title: Advanced Communication Techniques
slug: "dirac-intro-to-mpi-advanced-communication"
teaching: 0
exercises: 0
questions:
-
objectives:
- Know how to define and use custom data types for communication
- Understand the limitations of non-contiguous memory in MPI
keypoints:
-
---

TODO

## Vectors

<img src="fig/c_memory_layout.png" alt="Memory layout in C" height="250"/>

```c
int MPI_Type_vector(
   int count,              /* the number of 'blocks' to count in the vector */
   int blocklength,        /* the number of elements which makes up a block */
   int stride,             /* the number of elements between the start of each block */
   MPI_Datatype oldtype,   /* the datatype of the elements of the block */
   MPI_Datatype * newtype  /* the newly created datatype */
);
```

```c
int MPI_Type_commit(
   MPI_Datatype * datatype  /* the datatype to commit */
);
```

The following example shows how to send the middle row in a 3x3 array.

```c
MPI_Datatype row_t;      /* our new type is a MPI_Datatype */

MPI_Type_vector(3,       /* count is the length of the row */
                1,       /* blocklength says how many rows we want to include */
                3,       /* stride is the number of elements  */
                MPI_INT, /* oldtype states the datatype of the vector */
                &row_t   /* a pointer to initialize our new datatype */
);

MPI_Type_commit(&row_t); /* the new type has to be "commited" to prepare it for use */

int recv_buffer[3];
int send_buffer[3][3] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

/* send the middle row of our 2d send_buffer array */
MPI_Send(&send_buffer[0][1], 1, row_t, 1, 0, MPI_COMM_WORLD);

/* the receive function doesn't "work" with vector types, so we have to
   say that we are expecting 3 ints instead */
MPI_Recv(recv_buffer, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

> ## Communicating subarrays using vectors
>
> Create a vector type to send
>
> > ## Solution
> >
> > ```c
> > #include <mpi.h>
> >
> > int main(int argc, char **argv) {
> >
> > }
> > ```
> >
> {: .solution}
{: .challenge}

### Structures
