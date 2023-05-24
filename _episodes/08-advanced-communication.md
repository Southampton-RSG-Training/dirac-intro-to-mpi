---
title: Advanced Communication Techniques
slug: "dirac-intro-to-mpi-advanced-communication"
teaching: 20
exercises: 20
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

In the previous episodes, we've seen the basic building blocks for splitting work and communicating data between ranks,
meaning we're now dangerous enough to write a simple and successful MPI application. But we've only worked with
simple data structures so far, such as single variables or small 1D arrays. In reality, the software
we write uses more complex data structures, such as structures, n-dimensional arrays and other complex types. Working
with these in MPI require a bit more work to communicate them both correctly and efficiently.

To help with this, MPI provides an interface to create new types known as *derived datatypes*. A derived type acts
As a set of instructions which enable the translation of complex data structures into instructions, for MPI, for
efficient data communication.

## Working with multi-dimensional arrays

Almost all scientific and computing problems nowadays require us to think with more than one dimensional data. Using
multi-dimensional arrays, such for using matrices of tensors, or discretising something onto a 2D or 3D grid of points
are fundamental parts for most scientific software. The additional dimension comes with additional complexity, not just
in the code we write, but also in how data is communicated.

As a quite refresher, to create a 2 x 3 matrix in C and initialize it with some values, we use the following syntax,

```c
int matrix[2][3] = { {1, 2, 3}, {4, 5, 6} };  // matrix[rows][cols]
```

This creates an array with two rows and three columns. The first row contains the values `{1, 2, 3}` and the second row
contains `{4, 5, 6}`. The number of rows and columns can be any value, as long as there is enough memory available.

### The importance of memory contiguity

When a sequence of things is contiguous, it means there are multiple adjacent things without anything in between them.
In the context of MPI, when we talk about something being contiguous we are almost always talking about how arrays, and
other complex data structures are stored in the computer's memory. The elements in array are contiguous when the next,
or previous, element are stored in an adjacent memory location.

The memory space of a computer is linear. So when we create a multi-dimensional array, the compiler and operating system
have to decide how to map and store the elements into a linear memory space. There are two ways to do this:
[row-major or column-major](https://en.wikipedia.org/wiki/Row-_and_column-major_order). The difference between the
two ways is which elements of the array are contiguous in memory. Arrays are row-major in C and column-major in Fortran.
In a row-major array, the elements in each column of a row are contiguous so that element `x[i][j]` is
preceded by `x[i][j - 1]` and is followed by element `x[i][j +1]`. In Fortran, arrays are column-major so `x(i, j)` is
followed by `x(i + 1, j)` and so on.

The diagram below shows how a 4 x 4 matrix is mapped in a linear memory space, for a row-major array. At the top of the
diagram is the representation of the linear memory space, where each number is ID of the element in memory. Below that
are two representations of the array in 2D: the left shows the coordinate of each element and the right shows
the ID of the element.

<img src="fig/c_column_memory_layout.png" alt="Column memory layout in C" height=360>

The purple elements (5, 6, 7, 8) which map to the coordinates `[1][0]`, `[1][1]`, `[1][2]` and `[1][3]` are contiguous
in linear memory. The same applies for the orange boxes for the elements in row 2 (elements 9, 10, 11 and 12). Columns
in row-major arrays are contiguous. The next diagram instead shows how elements in adjacent rows are mapped in memory.

<img src="fig/c_row_memory_layout.png" alt="Row memory layout in C" height=360>

Looking first at the purple boxes (containing elements 2, 6, 10 and 14) which make up up the row elements for column 1,
we can see that the elements are not contiguous. Element `[0][1]` maps to element 2 and element `[1][1]` maps to element
6and so on. Elements in the same column but in a different row are separated by four other elements. In other words,
elements in other rows are not contiguous.

> ## Does memory contiguity affect performance?
>
> Do you think memory contiguity could impact the performance of our software, in a negative way?
>
> > ## Solution
> >
> > Yes, memory contiguity can affect how fast our programs run. When data is stored in a neat and organized way, the
> > computer can find and use it quickly. But if the data is scattered around randomly (fragmented), it takes more time
> > to locate and use it, which slows down our programs. So, keeping our data and data access patterns organized can
> > make our programs faster. However, we probably won't notice the difference for small arrays and data structures.
> {: .solution}
{: .challenge}

> ## What about if I use `malloc()`?
>
> More often than not, we will see `malloc()` being used to allocate memory for arrays. Especially if the code is using
> an older standard such as C90, which does not support [variable length
> arrays](https://en.wikipedia.org/wiki/Variable-length_array). When we use `malloc()`, we get a contiguous array of
> elements. To create a 2D array using `malloc()`, we  create an array of pointers (which are contiguous),
>
> ```c
> int num_rows = 3, num_cols = 5;
>
> float **matrix = malloc(num_rows * sizeof(float*));  /* Each pointer is the start of a row */
> for (int i = 0; i < num_rows; ++i) {
>    matrix[i] = malloc(num_cols * sizeof(float));     /* Here we allocate memory to store the  elements for row i */
> }
>
> for (int i = 0; i < num_rows; ++i) {
>    for (int j = 0; i < num_cols; ++j) {
>       matrix[i][j] = 3.14159;                        /* We index as usual */
>    }
> }
> ```
>
> There is one problem though. `malloc()` *does not* guarantee that subsequently allocated memory will be contiguous.
> When `malloc()` requests memory, the operating system will assign whatever memory is free. This is not always next to
> the block of memory from the previous allocation. This makes life tricky arrays allocated this way  since memory has
> to contiguous in memory for MPI. But there are workarounds. One is to only use 1D arrays (with the same number of
> elements as the higher dimension array) and to map coordinates into a linear coordinate system. For example, the
> element `[2][4]` in a a 3 x 5 matrix mapped in 1D would be accessed as,
>
> ```c
> int index_for_2_4 = matrix1d[5 * 2 + 4];  // num_cols * row + col
> ```
>
> Another solution is to move memory around, such as in [this example](code/examples/08-malloc-trick.c) or by using a
> more sophisticated method such as the [`arralloc()` function](code/arralloc.c) (not part of the standard library)
> which can allocate arbitrary n-dimensional arrays into a contiguous block.
>
{: .callout}

To send the elements of a row of a 4 x 4 matrix, we do something like this:

```c
MPI_Send(&matrix[1][0], 4, MPI_INT, ...);
```

The send buffer is `&matrix[1][0]`, which is the memory address of the first element in row 1. As the columns are four
elements long, we have specified to only sent four `MPI_INT`s. Even though we're working here with a 2D array, sending a
row of the array is essentially the same as with a 1D array. The main difference is the send buffer. Instead of using
a pointer to the start of the array, an address to the first element of the row (`&matrix[1][0]`) is used instead. We
can do this precisely because, just like in a 1D array, the elements in the row are contiguous. It's not possible to do
the same for a column, because the elements in the column are not contiguous.

### Using vectors to send slices of an array

To send a column of a matrix or tensor, we have to use a *vector*. A vector in MPI is a dervied datatype that represents
a continuous sequence of elements which have a regular spacing between them. By using vectors, we can create data types
for column vectors, row vectors or sub-arrays, similar to how we can [create slices for Numpy arrays in
Python](https://numpy.org/doc/stable/user/basics.indexing.html). All of which can be sent in a single, efficient,
communication. To create a vector, we have to create a new datatype using `MPI_Type_vector()`,

```c
int MPI_Type_vector(
   int count,              /* The number of 'blocks' which makes up the vector */
   int blocklength,        /* The number of contiguous elements in a block */
   int stride,             /* The number of elements between the start of each block */
   MPI_Datatype oldtype,   /* The datatype of the elements of the vector, e.g. MPI_INT, MPI_FLOAT */
   MPI_Datatype *newtype   /* The new datatype which represents the vector  - note that this is a pointer */
);
```

To understand what the arguments mean, look at the diagram below showing a vector to send two rows of a 4 x 4 matrix
with a row in between (rows 2 and 4),

<img src="fig/vector_linear_memory.png" alt="How a vector is laid out in memory" height=210>

A *block* refers to a sequence of contiguous elements. In the diagrams above, each sequence of contiguous purple or
orange elements represents a block. The *block length* is the number of elements within a block; in the above
this is four. The *stride* is the distance between the start of each block, which is eight. The count is the number of
blocks we want. When we create a vector, we're creating a new derived datatype which includes one or more blocks of
contiguous elements.

But fefore we can use the vector we create, it has to be committed using `MPI_Type_commit()`. This finalises the
creation of a derived datatype. Forgetting to do this step lead to unexpected behaviour, and potentially disastrous
consequences for our program!

```c
int MPI_Type_commit(
   MPI_Datatype *datatype  /* The datatype to commit - note that this is a pointer */
);
```

When a datatype is committed, resources which store information on how to handle it are internally allocated. This
contains data structures such as memory buffers as well as data used for bookkeeping. Failing to free those resources
after finishing with the vector leads to memory leaks, just like when we don't free memory created using `malloc()`. To
free up the resources, we use `MPI_Type_free()`,

```c
int MPI_Type_free (
   MPI_Datatype *datatype  /* The datatype to clean up -- note this is a pointer */
);
```

The following example code uses a vector to send two rows from a 4 x 4 matrix, as in the example diagram above.

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

/* The final thing to do is to free the new datatype when we no longer need it */
MPI_Type_free(&rows_type);
```

There are two things above, which look quite innocent, but are actually very important. First of all, the send buffer
in `MPI_Send()` is not `matrix` but `&matrix[1][0]`. In `MPI_Send()`, the send buffer is a pointer to the memory
location where the start of the data is stored. In the above example, the intention is to only send the second and forth
rows, so the start location of the data to send is the address for element `[1][0]`. If we used `matrix`, the first and
third row would be sent instead.

The other thing to notice, which is not immediately clear why it's done this way, is that the receive datatype is
`MPI_INT` and the count is `num_elements = count * blocklength` elements instead of a single element `rows_type`. This
is because when a rank receives data, the data is contiguous array. We don't need to use a vector to describe the layout
of the data like when sending it. We are really just receiving a 1D contiguous array of `num_elements = count *
blocklength` integers.

> ## Sending columns from an array
>
> Create a vector type to send a column in the following 2 x 3 array:
>
> ```c
> int matrix[2][3] = {
>     {1, 2, 3},
>     {4, 5, 6},
>  };
>```
>
> With that vector type, send the middle column of the matrix (elements `matrix[0][1]` and `matrix[1][1]`) from rank 0
> to rank 1 and print the results. You may want to use [this code](code/solutions/skeleton-example.c) as your starting
> point.
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
> >     MPI_Type_free(&col_t);
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
> By using a vector type, send the middle four elements (6, 7, 10, 11) in the following 4 x 4 matrix from rank 0 to rank
> 1,
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
> You can re-use most of your code from the previous exercise as your starting point, replacing the 2 x 3 matrix with
> the 4 x 4 matrix above and modifying the vector type and communication functions as required.
>
> > ## Solution
> >
> > The receiving rank(s) should receive the numbers 6, 7, 10 and 11 if your solution is correct. In the solution below,
> > we have created a vector with a count and block length of 2 and with a stride of 4. The first two
> > arguments means two vectors of block length 2 will be sent. The stride of 4 results from that there are 4 elements
> > between the start of each distinct block as shown in the image below,
> >
> > <img src="fig/stride_example_4x4.png" alt="Stride example for question" height="180"/>
> >
> > You must always remember to send the address for the starting point of the *first* block as the send buffer, which
> > is why `&array[1][1]` is the first argument in `MPI_Send()`.
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
> >     MPI_Type_free(&sub_array_t);
> >
> >     return MPI_Finalize();
> > }
> > ```
> >
> {: .solution}
{: .challenge}

## Structures in MPI

Structures, commonly known as structs, are custom datatypes which cotain multiple variables of different types. Some
common use cases of structs, in scientific code, include grouping together constants or global variables, or are used
to represent a physical thing, such as a particle, or something more abstract like a cell on a simulation grid. When we
use structs, we can write clearer, more concise and better structured code.

To communicate a struct, we need to define a derived datatype to tell MPI how understand the memory layout of the
struct. This is just like how we had to create a a derived type to describe a vector earlier. For a struct, we use
`MPI_Type_create_struct()`,

```c
int MPI_Type_create_struct(
   int count,                         /* The number of members/fields in the struct */
   int *array_of_blocklengths,        /* The length of the members/fields, as you would use in MPI_Send */
   MPI_Aint *array_of_displacements,  /* The relative positions of each member/field in bytes */
   MPI_Datatype *array_of_types,      /* The MPI type of each member/field */
   MPI_Datatype *newtype,             /* The new derived datatype */
);
```

The main difference between vector and struct derived types is that the arguments for structs expect arrays, since
structs are made up of multiple variables. Most of these arguments are straightforward, given what we've just seen for
defining vectors. But the `array_of_displacements` arguments is new to us.

When a struct is created, it occupies a single contiguous block of memory. But there is a catch. For
performance reasons, compilers insert arbitrary "padding" between each member for performance reasons. This padding,
known as [data structure alignment](https://en.wikipedia.org/wiki/Data_structure_alignment), optimises both the layout of
the memory and the access of it. As a result, the memory layout of a struct may look like this:

<img src="fig/struct_memory_layout.png" alt="Memory layout for a struct" height="384">

Although the memory used for padding and the struct's data exists in a contiguous block, the actual data we care about
is not contiguous any more. This is why we need the `array_of_displacements` argument, which specifies the distance, in
bytes, between each struct member relative to the start of the struct. In practise, it serves the same purpose to the
stride in vectors.

To calculate the byte displacement for each member, we need to know where in memory each member of a struct exists. To
do this, we can use the function `MPI_Get_address()`,

```c
int MPI_Get_address{
   const void *location,  /* A pointer to the variable we want the address for */
   MPI_Aint *address,     /* The address of the variable, as an MPI Address Integer -- returned via pointer */
};
```

In the following example, we use `MPI_Type_create_struct()` and `MPI_Get_address()` to create a derived type for a
struct with two members,

```c
/* Define and initialize a struct, named foo, with an int and a double */
struct MyStruct {
   int id;
   double value;
} foo = {.id = 0, .value = 3.1459};

/* Create arrays to describe the length of each member and their type */
int count = 2;
int block_lengths[2] = {1, 1};
MPI_Datatype block_types[2] = {MPI_INT, MPI_DOUBLE};

/* Now we calculate the displacement of each member, which are stored in an
   MPI_Aint designed for storing memory addresses */
MPI_Aint base_address;
MPI_Aint block_offsets[2];

MPI_Get_address(&foo, &base_address);            /* First of all, we find the address of the start of the struct */
MPI_Get_address(&foo.id, &block_offsets[0]);     /* Now the address of the first member "id" */
MPI_Get_address(&foo.value, &block_offsets[1]);  /* And the second member "value" */

/* Calculate the offsets, by subtracting the address of each field from the
   base address of the struct */
for (int i = 0; i < 2; ++i) {
   /* MPI_Aint_diff is a macro to calculate the difference between two
      MPI_Aints and is a replacement for:
      (MPI_Aint) ((char *) block_offsets[i] - (char *) base_address)
    */
   block_offsets[i] = MPI_Aint_diff(block_offsets[i], base_address);
}

/* We finally can create out struct data type */
MPI_Datatype struct_type;
MPI_Type_create_struct(count, block_lengths, block_offsets, block_types, &struct_type);
MPI_Type_commit(&struct_type);

/* Another difference between vector and struct derived types is that in
   MPI_Recv, we use the struct type. We have to do this because we aren't
   receiving a contiguous block of a single type of date. By using the type, we
   tell MPI_Recv how to understand the mix of data types and padding and how to
   assign those back to recv_struct
  */
if (my_rank == 0) {
   MPI_Send(&foo, 1, struct_type, 1, 0, MPI_COMM_WORLD);
} else {
   struct MyStruct recv_struct;
   MPI_Recv(&recv_struct, 1, struct_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

/* Remember to free the derived type */
MPI_Type_free(&struct_type);
```

> ## Sending a struct
>
> By using a derived data type, write a program to send the following struct `struct Node node` from one rank to
> another,
>
> ```c
> struct Node {
>     int id;
>     char name[32];
>     double temperature;
> };
>
> struct Node node = { .id = 0, .name = "Dale Cooper", .temperature = 42};
>```
>
> You may wish to use [this skeleton code](code/solutions/skeleton-example.c) as your stating point.
>
> > ## Solution
> >
> > Your solution should look something like the code block below. When sending a *static* array (`name[16]`), we have
> > to use a count of 16 in the `block_lengths` array for that member.
> >
> > ```c
> > #include <mpi.h>
> > #include <stdio.h>
> >
> > struct Node {
> >     int id;
> >     char name[16];
> >     double temperature;
> > };
> >
> > int main(int argc, char **argv)
> > {
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
> >     struct Node node = {.id = 0, .name = "Dale Cooper", .temperature = 42};
> >
> >     int block_lengths[3] = {1, 16, 1};
> >     MPI_Datatype block_types[3] = {MPI_INT, MPI_CHAR, MPI_DOUBLE};
> >
> >     MPI_Aint base_address;
> >     MPI_Aint block_offsets[3];
> >     MPI_Get_address(&node, &base_address);
> >     MPI_Get_address(&node.id, &block_offsets[0]);
> >     MPI_Get_address(&node.name, &block_offsets[1]);
> >     MPI_Get_address(&node.temperature, &block_offsets[2]);
> >     for (int i = 0; i < 3; ++i) {
> >         block_offsets[i] = MPI_Aint_diff(block_offsets[i], base_address);
> >     }
> >
> >     MPI_Datatype node_struct;
> >     MPI_Type_create_struct(3, block_lengths, block_offsets, block_types, &node_struct);
> >     MPI_Type_commit(&node_struct);
> >
> >     if (my_rank == 0) {
> >         MPI_Send(&node, 1, node_struct, 1, 0, MPI_COMM_WORLD);
> >     } else {
> >         struct Node recv_node;
> >         MPI_Recv(&recv_node, 1, node_struct, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
> >         printf("Received node: id = %d name = %s temperature %f\n", recv_node.id, recv_node.name,
> >                recv_node.temperature);
> >     }
> >
> >     MPI_Type_free(&node_struct);
> >
> >     return MPI_Finalize();
> > }
> > ```
> >
> {: .solution}
{: .challenge}

> ## What if I have a pointer in my struct?
>
> Suppose we have the following struct with a pointer named `position` and some other fields,
>
> ```c
> struct Grid {
>     double *position;
>     int num_cells;
> };
> grid.position = malloc(3 * sizeof(double));
>```
>
> If we use `malloc()` to allocate memory for `position`, how would we send data in the struct and the memory we
> allocated one rank to another? If you are unsure, try writing a short program to create a derived type for the struct.
>
> > ## Solution
> >
> > The short answer is that we can't do it using a derived type, and will have to *manually* communicate the data
> > separately. The reason why can't use a derived type is because the address of `*position` is the address of the
> > pointer. The offset between `num_cells` and `*position` is the size of the pointer and whatever padding the compiler
> > adds. The memory we allocated somewhere else in the memory, as shown in the diagram below.
> >
> > <img src="fig/struct_with_pointer.png" alt="Memory layout for a struct with a pointer" height="320">
> >
> >
> {: .solution}
>
{: .challenge}

> ## A different way to calculate displacements
>
> There are other ways to calculate the displacement, other than using what MPI provides for us. Another common way is
> to use the macro `offsetof()` macro part of `<stddef.h>`. `offsetof()` accepts two arguments, the first being the
> struct type and the second being the member to calculate the offset for.
>
> ```c
> #include <stddef.h>
> MPI_Aint displacements[2];
> displacements[0] = (MPI_Aint) offsetof(struct MyStruct, id);
> displacements[1] = (MPI_Aint) offsetof(struct MyStruct, value);
>```
>
> This method and the other shown in the previous examples both returns the same displacement values. It's mostly a
> personal choice which you choose to use. Some people prefer the "safety" of using `MPI_Get_address()` whilst others
> prefer to write more concise code with `offsetof()`. Of course, if you're a Fortran programmer then you can't use the
> macro
>
{: .callout}

## Packing and unpacking memory

The previous two sections covered how to communicate complex, but structured data between ranks using derived datatypes.
But we don't always have data which fits into the assumptions for a derived type. For example, in the last exercise
we've seen that pointers and struct derived types don't mix, and we've also seen that, without some pointer magic,
allocating arbitrary n-dimensional arrays using `malloc()` does not always return a contiguous block of memory.

- last case scenario, but can lead to less communication calls
- derived types more efficient - packing using more memory

Keep in mind that when packing and unpacking data with pointers, it's the data pointed to by the pointers that is being
packed, not the pointers themselves.

![Layout of packed memory](fig/packed_buffer_layout.png)

```c
int MPI_Pack(
   const void *inbuf,
   int incount,
   MPI_Datatype datatype,
   void *outbuf,
   int outsize,
   int *position,
   MPI_Comm comm
);
```

```c
int MPI_Pack_size(
   int incount,
   MPI_Datatype datatype,
   MPI_Comm comm,
   int *size
);
```

```c
int MPI_Unpack(
   const void *inbuf,
   int insize,
   int *position,
   void *outbuf,
   int outcount,
   MPI_Datatype datatype,
   MPI_Comm comm,
);
```

> ## What if the other rank doesn't know the size of the buffer?
>
> ```c
> MPI_Probe()
>```
>
{: .callout}

> ## Sending non-contiguous blocks with `MPI_Pack` and `MPI_Unpack`
>
> ```c
> #include <stdio.h>
> #include <stdlib.h>
>
> int main(int argc, char **argv)
> {
>    int num_rows = 3, num_cols = 3;
>
>    // Allocate and initialize a 2D array with the number of the element
>    int **matrix = malloc(num_rows * sizeof(int *));
>    for (int i = 0; i < num_rows; ++i) {
>       matrix[i] = malloc(num_cols * sizeof(int));
>       for (int j = 0; i < num_cols; ++j) {
>          matrix[i][j] = num_cols * i + j;
>       }
>    }
>
>    /*
>     * Add your code here
>     */
>
>    // Free all the memory for matrix
>    for (int i = 0; i < num_rows; ++i) {
>       free(matrix[i]);
>    }
>    free(matrix);
>
>    return 0;
> }
> ```
>
> > ## Solution
> >
> > ```c
> > #include <mpi.h>
> > #include <stdio.h>
> > #include <stdlib.h>
> >
> > int main(int argc, char **argv)
> > {
> >     int my_rank, num_ranks;
> >     MPI_Init(&argc, &argv);
> >     MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
> >     MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
> >
> >     int num_rows = 3, num_cols = 3;
> >
> >     /* Allocate and initialize a 2D array with the number of the element */
> >     int **matrix = malloc(num_rows * sizeof(int *));
> >     for (int i = 0; i < num_rows; ++i) {
> >         matrix[i] = malloc(num_cols * sizeof(int));
> >         for (int j = 0; j < num_cols; ++j) {
> >             matrix[i][j] = num_cols * i + j;
> >         }
> >     }
> >
> >     /* Calculate how big the MPI_Pack buffer should be */
> >     int pack_buffer_size;
> >     MPI_Pack_size(num_rows * num_cols, MPI_INT, MPI_COMM_WORLD, &pack_buffer_size);
> >
> >     if (my_rank == 0) {
> >         /* Create the pack buffer and pack each row of data into it buffer
> >            one by one */
> >         int position = 0;
> >         char *packed_data = malloc(pack_buffer_size * sizeof(char));
> >         for (int i = 0; i < num_rows; ++i) {
> >             MPI_Pack(matrix[i], num_cols, MPI_INT, packed_data, pack_buffer_size, &position, MPI_COMM_WORLD);
> >         }
> >
> >         /* Send the packed data to rank 1 and free memory we no longer need */
> >         MPI_Send(packed_data, pack_buffer_size, MPI_PACKED, 1, 0, MPI_COMM_WORLD);
> >         free(packed_data);
> >     } else {
> >         /* Create a receive buffer and get the packed buffer from rank 0 */
> >         char *received_data = malloc(pack_buffer_size * sizeof(char));
> >         MPI_Recv(received_data, pack_buffer_size + 1, MPI_PACKED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
> >
> >         /* allocate a matrix to put the receive buffer into -- this is for
> >            demonstration purposes */
> >         int **my_matrix = malloc(num_rows * sizeof(int *));
> >         for (int i = 0; i < num_cols; ++i) {
> >             my_matrix[i] = malloc(num_cols * sizeof(int));
> >         }
> >
> >         /* Unpack the received data row by row into my_matrix */
> >         int position = 0;
> >         for (int i = 0; i < num_rows; ++i) {
> >             MPI_Unpack(received_data, pack_buffer_size, &position, my_matrix[i], num_cols, MPI_INT, MPI_COMM_WORLD);
> >         }
> >         free(received_data);
> >
> >         /* Print the elements of my_matrix */
> >         printf("Rank 1 received the following array:\n");
> >         for (int i = 0; i < num_rows; ++i) {
> >             for (int j = 0; j < num_cols; ++j) {
> >                 printf(" %d", my_matrix[i][j]);
> >             }
> >             printf("\n");
> >         }
> >
> >         /* Free memory for temporary my_matrix */
> >         for (int i = 0; i < num_rows; ++i) {
> >             free(my_matrix[i]);
> >         }
> >         free(my_matrix);
> >     }
> >
> >     /* Free all the memory for matrix */
> >     for (int i = 0; i < num_rows; ++i) {
> >         free(matrix[i]);
> >     }
> >     free(matrix);
> >
> >     return MPI_Finalize();
> > }
> > ```
> >
> {: .solution}
{: .challenge}
