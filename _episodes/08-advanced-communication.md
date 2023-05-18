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
> `malloc` *does not* guarantee contiguous memory when allocating multi-dimensional arrays (or pointer arrays). This
> makes life tricky to communicate them, since the communication functions in MPI assume the data we are communicating
> is contiguous in memory. There are workarounds though. One workaround is to only work with 1D arrays (with the same
> number of elements as the higher dimension array) and to map the multi-d coordinates into the 1D memory space. For
> example, the element `[2][4]` in a a 3 x 5 matrix mapped in 1D would be accessed as,
>
> ```c
> int index_for_2_4 = matrix1d[5 * 2 + 4];  // num_cols * row + col
> ```
>
> Another solution is to use a clever function [`arralloc.c`](code/arralloc.c) which can allocate multi-dimensional
> arrays into a single block of memory.
>
> ```c
> int **matrix = arralloc(sizeof int, 2, 5);
> /* but to send an arralloc'd array, we need to pass the address of the first element to MPI functions */
> MPI_Send(&matrix[0][0], 2 * 5, MPI_INT, ...);
> /* the pointer is free'd as you would free a 1d array */
> free(matrix);
> ```
>
{: .callout}

### Using vectors to send slices of an array

To send a single column for a matrix or tensor, we have to use a *vector*. A vector in MPI is a datatype that represents
a continuous sequence of elements which have a regular spacing between them. Using vectors, we can create columns
vectors, rows vectors or sub-arrays of a larger array, similar to how we can [create slices for Numpy arrays in
Python](https://numpy.org/doc/stable/user/basics.indexing.html), which can be sent in a single communication. To create
a vector, we have to create a new datatype to represent the vector using `MPI_Type_vector()`,

```c
int MPI_Type_vector(
   int count,              /* the number of 'blocks' which makes up the vector */
   int blocklength,        /* the number of contiguous elements in a block */
   int stride,             /* the number of elements between the start of each block */
   MPI_Datatype oldtype,   /* the datatype of the elements of the vector, e.g. MPI_INT, MPI_FLOAT */
   MPI_Datatype * newtype  /* the new datatype which represents the vector  - note that this is a pointer */
);
```

To understand what some of these arguments mean, consider the diagram below which is creating a vector to send two rows
with a row in between,

<img src="fig/vector_linear_memory.png" alt="How a vector is laid out in memory" height=210>

First of all, let's clarify some jargon: a *block* refers to a sequence of contiguous elements. In the diagrams above,
each sequence of contiguous purple or orange elements represents a block. The *block length* refers to the number of
elements within a block; in the above examples this is four. The *stride* is the distance between the start of each
block. When we define a vector, we create a datatype that includes one or more blocks of contiguous elements, with a
regular spacing between them.

Before we can use a vector, it has to first be committed using `MPI_Type_commit` which finalises the creation of a
custom datatype. Forgetting to do this step can lead to unexpected behaviour and potentially disastrous consequences!

```c
int MPI_Type_commit(
   MPI_Datatype * datatype  /* the datatype to commit - note that this is a pointer */
);
```

The following example code uses an vector to send the two rows from a 4 x 4 matrix.

```c
/* The vector is a MPI_Datatype */
MPI_Datatype rows_type;

/* Create the vector type */
const int count = 2;
const int blocklength = 4;
const int stride = 8;
MPI_Type_vector(count, blocklength, stride, MPI_INT, &rows_type);

/* Don't forget to commit it */
MPI_Type_commit(&rows_type);

/* Send the middle row of our 2d send_buffer array. Note that we are sending
 &send_buffer[1][0] and not send_buffer. This is because we are using an offset
 to change the starting point of where we begin sending memory
 */
int matrix[4][4] = {
   { 1,  2,  3,  4},
   { 5,  6,  7,  8},
   { 9, 10, 11, 12},
   {13, 14, 15, 16},
};

MPI_Send(&matrix[1][0], 1, rows_type, 1, 0, MPI_COMM_WORLD);

/* The receive function doesn't "work" with vector types, so we have to
   say that we are expecting 8 integers instead */
const int num_elements = count * blocklength;
int recv_buffer[num_elements];
MPI_Recv(recv_buffer, num_elements, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

There are two things above, which look quite innocent, but are actually vitally important. First of all, the send buffer
in `MPI_Send` is not `matrix` but `&matrix[1][0]`. In `MPI_Send`, the send buffer is a pointer to the memory
location where the start of the data is stored. In the above example, the intention is to only send the second and forth
rows, so the start location of the (contiguous) data is the memory address of element `[1][0]` instead. If we used
`matrix`, the first and third row would be sent instead.

The other thing to notice, which is not immediately clear why it's done this way, is that the receive datatype is
`MPI_INT` and the count is `num_elements = count * blocklength` elements instead of a single element `rows_type`.
This is because when a rank receives data, the data is contiguous. We don't need to use a vector to describe the layout
of the (non-contiguous) data like we do when we are sending it. We are really just receiving a 1D contiguous array of
`num_elements = count * blocklength` integers.

> ## Sending columns from an array
>
> Create a vector type to send a column in the following 2 x 3 array,
>
> ```c
> int matrix[2][3] = {
>     {1, 2, 3},
>     {4, 5, 6},
>  };
>```
>
> With that vector type, send the middle column of the matrix (elements `matrix[0][1]` and `matrix[1][1]`) from
> rank 0 to rank 1 and print the results.
>
> > ## Solution
> >
> > If your solution is correct you should see 2 and 5 printed to the screen. In the solution below, to send a 2 x 1
> > column of the matrix, we created a vector with `count = 2`, `blocklength = 1` and `stride = 3`. To send the correct
> > column our send buffer was `&matrix[0][1]` which is the address of the first element in column 1. To see why the
> > stride is 3, take a look at the diagram below,
> >
> > <img src="fig/stride_example_2x3.png" alt="Stride example for question" height="180"/>
> >
> > You can see that there are *three* contiguous elements between the start of each block of 1.
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
> By using a vector type, send the middle four elements (6, 7, 10, 11) in the following 4 x 4 matrix,
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
> > ## Solution
> >
> > The receiving rank(s) should receive the numbers 6, 7, 10 and 11 if your solution is correct. In the solution below,
> > we have created a `MPI_Type_vector` with a count and block length of 2 and with a stride of 4. The first two
> > arguments means two vectors of block length 2 will be sent. The stride of 4 results from that there are 4 elements
> > between the start of each distinct block as shown in the image below,
> >
> > <img src="fig/stride_example_4x4.png" alt="Stride example for question" height="180"/>
> >
> > You must always remember to send the address for the starting point of the *first* block as the send buffer, which
> > is why `&array[1][1]` is the first argument in `MPI_Send`.
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
