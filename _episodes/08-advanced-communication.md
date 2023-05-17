---
title: Advanced Communication Techniques
slug: "dirac-intro-to-mpi-advanced-communication"
teaching: 15
exercises: 15
questions:
- How do I use complex data structures in MPI?
- What is contiguous memory, and why does it matter?
objectives:
- Understand the limitations of non-contiguous memory in MPI
- Know how to define and use custom data types for communication
keypoints:
- Communicating complex data structures in MPI requires a bit more work
- Any data being transferred should ideally be a single contiguous block of memory
- By defining vectors and other datatypes, we can send data for elements which are not-contiguous
- The functions `MPI_Pack` and `MPI_Unpack` can be used to create a contiguous memory block
---

In the previous episodes, we covered the basic building blocks for splitting work and communicating data between ranks,
meaning we're now dangerous enough to write a simple, and successful, MPI application. But we've only really worked
with communicating simple data structures, such as single variables or small one dimensional arrays. In reality, the
software we write is going to have use more complex data structures, such as structures or n-dimensional arrays, which
require a bit more work to communicate correctly and efficiently.

## Working with multi-dimensional arrays

Almost all scientific and programming problems nowadays require us to think in more than one dimension. We often find
that using multi-dimensional arrays such for using matrices of tensors, or discretising something onto a 2D or 3D grid
of points is fundamental to the software we write. But with the additional dimension comes additional complexity, not
just in the code we write but also in how we communicate the extra data.

To make sure we're all on the same page, there are two ways to define a multi-dimensional array in C. The first way is
an extension of defining a 1D array. To create a *static* 2 x 3 matrix and initialize some values, we use the following
syntax,

```c
int matrix[2][3] = { {1, 2, 3}, {4, 5, 6} };  // matrix[rows][cols]
```

The same can be done for any arbitrary sized rectangular array and we also do not need to initialize the array with
values. However, we are limited in the size of array we can allocate as the dimensions as static (usually at compile
time) and because memory for arrays defined this way are on the
[stack](https://en.wikipedia.org/wiki/Stack-based_memory_allocation), which is limited on most systems.

The other way is to use `malloc` to dynamically allocate memory on the
[heap](https://en.wikipedia.org/wiki/Memory_management#HEAP) instead. The main advantage of `malloc` is that the heap is
far larger than the stack so we can create larger arrays, and we can also dynamically set the size of arrays at run-time
for different requirements. But it is a bit more tricky to use. To create a 3 x 3 matrix this way, we create an array of
pointers (or pointers to pointers) as such:

```c
float **matrix = malloc(3 * sizeof float);
for (int i = 0; i < 3; ++i) {
   matrix[i] = malloc(3 * sizeof float);
}
```

### The importance of memory contiguity

When something is contiguous, it means there are multiple adjacent things without anything in between them. In the
context of MPI, when we talk about something being contiguous we are almost always talking about how arrays are mapped
in the computer's memory. The elements in array are contiguous when the next, or previous, element are stored in an
adjacent memory location.

The memory space of a computer is linear. When we create a multi-dimensional array, the compiler and operating system
have to decide how to map and store the elements into the linear memory space. There are two ways to do this: either
[row-major or column-major](https://en.wikipedia.org/wiki/Row-_and_column-major_order). The difference between the
orders is which elements of the array are contiguous in memory. Arrays are row-major in C and column-major in Fortran.
In a row-major array, the elements in each column of a row are contiguous so that in memory the element `x[i][j]` is
preceded by `x[i][j - 1]` and is followed by element `x[i][j +1]`. In Fortran, arrays are column-major so `x(i, j)` is
followed by `x(i + 1, j)`.

The diagram below shows how a 4 x 4 matrix is  mapped to the linear memory space. At the top of the diagram is the
representation of the linear memory space, where each number is the order of the element in linear memory. Below that
are two representations of the array in a 2D space: the left shows the coordinate of each element and the right shows
the order label in the linear representation.

<img src="fig/c_column_memory_layout.png" alt="Column memory layout in C" height=360>

What's important is what the coloured boxes are showing. The purple elements (5, 6, 7, 8) which map to the coordinates
`[1][0]`, `[1][1]`, `[1][2]` and `[1][3]` are contiguous in linear memory. The same applies for the orange boxes for the
elements in row 2 (elements 9, 10, 11 and 12). The next diagram below instead shows how elements in adjacent rows are
mapped in linear memory.

<img src="fig/c_row_memory_layout.png" alt="Row memory layout in C" height=360>

Looking first at the purple boxes (containing elements 2, 6, 10 and 14) which make up up the row elements for column 1,
we can see that the elements are no longer contiguous. Element `[0][1]` maps to the second element in linear space,
element `[1][1]` maps to the sixth element and so on. Elements in the same column but in a different row are separated
by four other elements. In other words, elements in other rows are not contiguous.

If we wanted to send all of the elements in row 1 of a 4 x 4 matrix, we can do something like this,

```c
MPI_Send(&matrix[1][0], 4, MPI_INT, ...);
```

In this case, the send buffer is `&matrix[1][0]` which is the memory address of the first element in row 1 and we've
only asked to send *four* integer elements. This is actually same we've done for 1D arrays. With the difference that we
have specified a different starting point (by passing the address of element `[1][0]`) and are asking for less elements
to be sent. The reason why this works is because, just like in a 1D array, the four elements in the row are contiguous.
We are not able do the same to send a column of the matrix, because the elements in a column are not contiguous. If we
tried the same operation for `&matrix[0][1]` to send column 1, then elements `[0][1]`, `[0][2]`, `[0][3]` and `[1][0]`
are sent (because those are contiguous) instead of `[0][1]`, `[1][1]`, `[2][1]` and `[3][1]`.

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

### Using vectors to send slices

To send a single column for a matrix or tensor, we have to use a *vector*. A vector in MPI is a datatype that represents
a continuous sequence of elements which have a regular spacing between them. Using vectors, we can send columns, rows or
sub-arrays of a larger array in a single communication.

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

<img src="fig/vector_linear_memory.png" alt="How a vector is laid out in memory" height=210>

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

/* send the middle row of our 2d send_buffer array. Note that we are sending
 &send_buffer[1][0] and not send_buffer. This is because we need to send the
 starting point of the vector type instead of the first element of the array
 */
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
>   { 1,  2,  3,  4},
>   { 5,  6,  7,  8},
>   { 9, 10, 11, 12},
>   {13, 14, 15, 16}
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
> >         { 1,  2,  3,  4},
> >         { 5,  6,  7,  8},
> >         { 9, 10, 11, 12},
> >         {13, 14, 15, 16}
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
