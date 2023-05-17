---
title: Advanced Communication Techniques
slug: "dirac-intro-to-mpi-advanced-communication"
teaching: 0
exercises: 15
questions:
-
objectives:
- Know how to define and use custom data types for communication
- Understand the limitations of non-contiguous memory in MPI
keypoints:
-
---

In the previous episodes, we covered the basic building blocks for splitting work and communicating data between ranks,
meaning we're now dangerous enough to write a simple, and successful, MPI application. But we've only really worked
with communicating simple data structures, such as single variables or small one dimensional arrays. In reality, the
software we write is going to have use more complex data structures, such as `structs` or n-dimensional arrays, which
require a bit more work to communicate efficiently.

## Working with multi-dimensional arrays

Almost all scientific and programming problems nowadays require us to think in more than one dimension. We often find
that using multi-dimensional arrays such for using matrices of tensors, or discretising something onto a 2D or 3D grid
of points is fundamental to the software we write. But with the additional dimension comes additional complexity, not
just in the code we write but also in how we communicate the extra data.

To make sure we're all on the same page, there are two ways to define a multi-dimensional array in C. The first way is
an extension of defining a 1D array. To create a *static* 3 x 3 matrix and initialize some values, we use the following
syntax,

```c
int matrix[3][3] = { {1, 2, 3}, {4, 5, 6}, {7, 8, 9} };
```

The same can be done for any arbitrary sized rectangular array and we also do not need to initialize the array with
values. However, we are limited in the size of array we can allocate as the dimensions as static (usually at compile
time) and because memory for arrays defined this way are on the
[stack](https://en.wikipedia.org/wiki/Stack-based_memory_allocation), which is limited on most systems.

The other way is to use `malloc` to dynamically allocate memory on the
[heap](https://en.wikipedia.org/wiki/Memory_management#HEAP) instead. The main advantage of using `malloc` is that the
heap is far larger so we can create larger arrays, and we can dynamically set the size at run-time for different
requirements. But it is a bit more tricky to use. To create a 3 x 3 matrix this way, we actually need to create an array
of pointers (or pointers to pointers):

```c
float **matrix = malloc(3 * sizeof float);
for (int i = 0; i < 3; ++i) {
   matrix[i] = malloc(3 * sizeof float);
}
```

### The importance of memory contiguity

When we say something is contiguous, all we mean is that we have multiple things next to each other without anything in
between. In the context of MPI, when we talk about memory contiguity we are almost always talking about how arrays are
laid out in memory and if elements in arrays are adjacent to one another or not.

![Column memory layout in C](fig/c_column_memory_layout.png)

![Column row layout in C](fig/c_row_memory_layout.png)

Memory in C is contiguous in the column direction, `x[i][j]` is followed by `x[i][j + 1]`. So the next element

> ## What about if I use `malloc`?
>
> Rather unfortunately, `malloc` *does not* guarantee contiguous memory when used to allocate multi-dimension arrays.
> This makes our life tricky when we want to communicate them, because the basic communication functions require that
> the data structure to be communicate is one contiguous block of memory. One workaround is to allocate only 1D arrays
> with the same number of elements as the higher dimension array, and to map the coordinates into a 1D coordinate
> system. For example, elements in a a 3 x 5 matrix mapped onto a 1D array would be accessed as such,
>
> ```c
> int index_for_2_4 = matrix1d[5 * 2 + 4];  // num_cols * row + col
> ```
>
> Another solution is to use something like
> [`arralloc.c`](https://www.maths.tcd.ie/~bouracha/college/Edinburgh/term1/programming_skills/code_development/project/src/arralloc.c),
> which is a clever function which can allocate multi-dimensional arrays into a single block of memory.
>
> ```c
> int **matrix = arralloc(sizeof int, 2, 5);
> /* but to send an arralloc'd array, we need to pass the address of the first element to MPI functions */
> MPI_Send(&matrix[0][0], 2 * 5, MPI_INT, ...);
> ```
>
{: .callout}

### Using vectors to send sub-arrays

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

![Linear memory layout in C](fig/vector_linear_memory.png)

The following example shows how to send the middle column in a 3x3 array.

```c
MPI_Datatype row_t;      /* our new type is a MPI_Datatype */


MPI_Type_vector(1,       /* count is number of blocks, e.g. the number of rows we want */
                3,       /* blocklength is the number of contiguous elements which form a block */
                3,       /* stride is the number of elements between each contiguous block */
                MPI_INT, /* oldtype states the datatype of the vector */
                &row_t   /* a pointer to initialize our new datatype */
);

MPI_Type_commit(&row_t); /* the new type has to be "committed" to ready for use */

int recv_buffer[3];
int send_buffer[3][3] = {
   {0, 1, 2},
   {3, 4, 5},
   {6, 7, 8},
};

/* send the middle row of our 2d send_buffer array */
MPI_Send(&send_buffer[1][0], 1, row_t, 1, 0, MPI_COMM_WORLD);

/* the receive function doesn't "work" with vector types, so we have to
   say that we are expecting 3 ints instead */
MPI_Recv(recv_buffer, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

> ## Sending columns from an array
>
> For the following 2 x 3 array,
>
> ```c
> int matrix[2][3] = {
>     {1, 2, 3},
>     {4, 5, 6},
>  };
>```
>
> create a vector type to send a single column and communicate the middle column (elements `matrix[0][1]` and
> `matrix[1][1]`) from one rank to another.
>
> > ## Solution
> >
> > ```c
> > #include <mpi.h>
> > #include <stdio.h>
> >
> > int main(int argc, char **argv)
> > {
> >     int my_rank;
> >     int num_ranks;
> >     MPI_Init(&argc, &argv);
> >     MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
> >     MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
> >
> >     int matrix[2][3] = {
> >         {1, 2, 3},
> >         {4, 5, 6},
> >     };
> >
> >     if (num_ranks != 2) {
> >         if (my_rank == 0) {
> >             printf("This example only works with 2 ranks\n");
> >         }
> >         MPI_Abort(MPI_COMM_WORLD, 1);
> >     }
> >
> >     MPI_Datatype col_t;
> >     MPI_Type_vector(2, 1, 3, MPI_INT, &col_t);
> >     MPI_Type_commit(&col_t);
> >
> >     if (my_rank == 0) {
> >         MPI_Send(&matrix[0][1], 1, col_t, 1, 0, MPI_COMM_WORLD);
> >     } else {
> >         int buffer[2];
> >         MPI_Status status;
> >
> >         MPI_Recv(buffer, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
> >
> >         printf("Rank %d received the following:", my_rank);
> >         for (int i = 0; i < 2; ++i) {
> >             printf(" %d", buffer[i]);
> >         }
> >         printf("\n");
> >     }
> >
> >     return MPI_Finalize();
> > }
> > ```
> >
> {: .solution}
>
{: .challenge}

> ## Sending sub-arrays of an array
>
> By using a vector type, send the middle four elements (6, 7, 10, 11) in the following 4x4 matrix,
>
> ```c
> int matrix[4][4] = {
>    1,  2,  3,  4,
>    5,  6,  7,  8,
>    9, 10, 11, 12,
>   13, 14, 15, 16
> };
> ```
>
<!-- > ![Sub array](fig/vector_type_exercise.png) -->
>
>
> > ## Solution
> >
> > The receiving rank(s) should receive the numbers 6, 7, 10 and 11 if your solution is correct. In the solution below,
> > we have created a `MPI_Type_vector` with a count and block length of 2 and with a stride of 4. The first two
> > arguments means two vectors of block length 2 will be sent. The stride of 4 results from that there are 4 elements
> > between the start of each distinct block as shown in the image below,
> >
> > <img src="fig/stride_example.png" alt="Stride example for question" height="180"/>
> >
> > You must always remember to send the address for the starting point of the *first* block as the send buffer, which
> > is why `&array[1][1]` is the first argument in `MPI_Ssend`.
> >
> > ```c
> > #include <mpi.h>
> > #include <stdio.h>
> >
> > int main(int argc, char **argv)
> > {
> >     int matrix[4][4] = {
> >          1,  2,  3,  4,
> >          5,  6,  7,  8,
> >          9, 10, 11, 12,
> >         13, 14, 15, 16
> >     };
> >
> >     int my_rank;
> >     int num_ranks;
> >     MPI_Init(&argc, &argv);
> >     MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
> >     MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
> >
> >     if (num_ranks != 2) {
> >         if (my_rank == 0) {
> >             printf("This example only works with 2 ranks\n");
> >         }
> >         MPI_Abort(MPI_COMM_WORLD, 1);
> >     }
> >
> >     MPI_Datatype sub_array_t;
> >     MPI_Type_vector(2, 2, 4, MPI_INT, &sub_array_t);
> >     MPI_Type_commit(&sub_array_t);
> >
> >     if (my_rank == 0) {
> >         MPI_Send(&matrix[1][1], 1, sub_array_t, 1, 0, MPI_COMM_WORLD);
> >     } else {
> >         int buffer[4];
> >         MPI_Status status;
> >
> >         MPI_Recv(buffer, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
> >
> >         printf("Rank %d received the following:", my_rank);
> >         for (int i = 0; i < 4; ++i) {
> >             printf(" %d", buffer[i]);
> >         }
> >         printf("\n");
> >     }
> >
> >     return MPI_Finalize();
> > }
> > ```
> >
> {: .solution}
{: .challenge}

## Structures in MPI

## Packing and unpacking memory
