---
title: Communication Patterns
slug: "dirac-intro-to-mpi-communication-patterns"
teaching: 0
exercises: 0
math: true
questions:
- What are some common data communication patterns in MPI?
objectives:
- Learn and understand common communication patterns
- Be able to determine what communication pattern you should use for your own MPI applications
keypoints:
- There are many ways to communicate data, which we need to think about carefully
- It's better to use collective operations, rather than implementing similar behaviour yourself
---

We have now come across the basic building blocks we need to create an MPI application. The previous episodes have
covered how to split tasks between ranks to parallelise the workload, and how to communicate data between ranks; either
between two ranks (point-to-point) or multiple at once (collective). The next step is to build upon these basic blocks,
and to think about how we should structure our communication. The parallelisation and communication strategy we choose
will depend on the underlying problem and algorithm. For example, a grid based simulation (such as in computational
fluid dynamics) will need to structure its communication differently to a simulation which does not discretise
calculations onto a grid of points. In this episode, we will look at some of the most common communication patterns.

## Scatter and gather

Using the scatter and gather collective communication functions, to distribute work and bring the results back together,
is a common pattern and finds a wide range of applications. To recap: in **scatter communication**, the root rank
splits a piece of data into equal chunks and sends a chunk to each of the other ranks, as shown in the diagram below.

![Depiction of scatter communication pattern](fig/scatter.png)

In **gather communication** all ranks send data to the root rank which combines them into a single buffer.

![Depiction of gather communication pattern, with each rank sending their data to a root rank](fig/gather.png)

Scatter communication is useful for most algorithms and data access patterns, as is gather communication. At least for
scattering, this communication pattern is generally easy to implement especially for embarrassingly parallel problems
where the data sent to each rank is independent. Gathering data is useful for bringing results to a root rank to process
further or to write to disk, and is also helpful for bring data together to generate diagnostic output.

Since scatter and gather communications are collective, the communication time required for this pattern increases as
the number of ranks increases. The amount of messages that needs to be sent increases logarithmically with the number of
ranks. The most efficient implementation of scatter and gather communication are to use the collective functions
(`MPI_Scatter()` and `MPI_Gather()`) in the MPI library.

One method for parallelising matrix multiplication is with a scatter and gather communication. To multiply two matrices,
we follow the following equation,

$$ \left[ \begin{array}{cc} A_{11} & A_{12} \\ A_{21} & A_{22}\end{array} \right] \cdot \left[ \begin{array}{cc}B_{11} &
B_{12} \\ B_{21} & B_{22}\end{array} \right]   = \left[ \begin{array}{cc}A_{11} \cdot B_{11} + A_{12} \cdot B_{21} &
A_{11} \cdot
B_{12} + A_{12} \cdot B_{22} \\ A_{21} \cdot B_{11} + A_{22} \cdot B_{21} & A_{21} \cdot B_{12}
+ A_{22} \cdot B_{22}\end{array} \right]$$

Each element of the resulting matrix is a dot product between a row in the first matrix (matrix A) and a column in
the second matrix (matrix B). Each row in the resulting matrix depends on a single row in matrix A, and each column in
matrix B. To split the calculation across ranks, one approach would be to *scatter* rows from matrix A and calculate the
result for that scattered data and to combine the results from each rank to get the full result.

```c
/* Determine how many rows each matrix will compute and allocate space for a receive buffer
   receive scattered subsets from root rank. We'll use 1D arrays to store the matrices, as it
   makes life easier when using scatter and gather */
int rows_per_rank = num_rows_a / num_ranks;
double *rank_matrix_a = malloc(rows_per_rank * num_rows_a * sizeof(double));
double *rank_matrix_result = malloc(rows_per_rank * num_cols_b * sizeof(double));

/* Scatter matrix_a across ranks into rank_matrix_a. Each rank will compute a subset of
   the result for the rows in rank_matrix_a */
MPI_Scatter(matrix_a, rows_per_ranks * num_cols_a, MPI_DOUBLE, rank_matrix_a, rows_per_ranks * num_cols_a,
            MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD);

/* Broadcast matrix_b to all ranks, because matrix_b was only created on the root rank
   and each sub-calculation needs to know all elements in matrix_b */
MPI_Bcast(matrix_b, num_rows_b * num_cols_b, MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD);

/* Function to compute result for the subset of rows of matrix_a */
multiply_matrices(rank_matrix_a, matrix_b, rank_matrix_result);

/* Use gather communication to get each rank's result for rank_matrix_a * matrix_b into receive
   buffer `matrix_result`. Our life is made easier since rank_matrix and matrix_result are flat (and contiguous)
   arrays, so we don't need to worry about memory layout*/
MPI_Gather(rank_matrix_result, rows_per_rank * b_cols, MPI_DOUBLE, matrix_result, rows_per_ranks * b_cols,
           MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD);
```

## Reduction

A reduction operation is one that *reduces* multiple amounts of data into a single value, such as during a summation or
in finding the largest value in a collection of values. Similar to the gathering and scattering pattern, using reduction
operations to communicate data between ranks has a wide range of applications.

The sum example above is a reduction.  Since data is needed from all ranks, this tends to be a time consuming operation,
similar to a gather operation.  Usually each rank first performs the reduction locally, arriving at a single number.
They then perform the steps of collecting data from some of the ranks and performing the reduction on that data, until
all the data has been collected.  The most efficient implementation depends on several technical features of the system.
Fortunately many common reductions are implemented in the MPI library and are often optimised for a specific system.

TODO

The following code example shows a Monte Carlo algorithm to estimate the value of $\pi$. Millions of random points are
generated and checked to see if they are within or outside of a unit circle (a circle with a radius of 1). The ratio of
points within the unit circle and the total number of points generated is proportional to $\pi$.

Each iteration, or point, is independent of any other points or data in the algorithm. Therefore there needs to be no
additional communication. To parallelise the task, each rank will compute the number of points which fall within the
unit circle for a subset of the total number of points. The final ratio of points within the circle to the number of
points in total is calculated by using a reduction to get the total sum of points within the unit circle, and comparing
this to the total number of points generated across all ranks.

```c
/* 1 billion points is a lot, so we should parallelise this calculation */
int total_num_points = (int)1e9;

/* Each rank will check an equal number of points, with their own
   counter to track the number of points falling within the circle */
int points_per_rank = total_num_points / num_ranks;
int rank_points_in_circle = 0;

/* Seed each rank's RNG with a unique seed, otherwise each rank will have an
   identical result and it would be the same as using `points_per_rank` in total
   rather than `total_num_points` */
srand(time(NULL) + my_rank);

/* Generate a random x and y coordinate (between 0 - 1) and check to see if that
   point lies within the unit circle */
for (int i = 0; i < points_per_rank; ++i) {
    double x = (double)rand() / RAND_MAX;
    double y = (double)rand() / RAND_MAX;
    if (x * x + y * y <= 1.0) {
        rank_points_in_circle++;  /* It's in the circle, so increment */
    }
}

/* Perform a reduction to sum up `rank_points_in_circle` across all ranks, this
   will be the total number of points in a circle for `total_num_point` iterations */
int total_points_in_circle;
MPI_Reduce(&rank_points_in_circle, &total_points_in_circle, 1, MPI_INT, MPI_SUM, ROOT_RANK, MPI_COMM_WORLD);

/* The estimate for π is proportional to the ratio of the points in the circle and the number of
   point generated */
if (my_rank == ROOT_RANK) {
    double pi = 4.0 * total_points_in_circle / total_num_points;
    printf("Estimated value of π = %f\n", pi);
}
```

## Domain decomposition and halo exchange

The matrix example was already an example of domain decomposition, in 1 dimension.

[`MPI_Dims_create()`](https://www.open-mpi.org/doc/v4.1/man3/MPI_Dims_create.3.php)

```c
/* We have to first calculate the size of each rectangular region. In this example, we have
   assumed that the dimensions are perfectly divisible. We can determine the dimensions for the
   decomposition by using MPI_Dims_create() */
int rank_dims[2] = { 0, 0 };
MPI_Dims_create(num_ranks, 2, rank_dims);
int num_rows_per_rank = num_rows / rank_dims[0];
int num_cols_per_rank = num_cols / rank_dims[1];
int num_elements_per_rank = num_rows_per_rank * num_cols_per_rank;

/* The rectangular blocks we create are not contiguous in memory, so we have to use a
   derived data type for communication */
int count = num_rows_per_rank;
int blocklength = num_cols_per_rank;
int stride = num_cols;
MPI_Datatype subarray_t;
MPI_Type_vector(count, blocklength, stride, MPI_DOUBLE, &subarray_t);
MPI_Type_commit(&subarray_t);

/* MPI_Scatter (and similar collective functions) do not work with this sort of Cartesian
   topology, so we unfortunately have to scatter the array manually */
double *rank_array = malloc(num_elements_per_rank * sizeof(double));
scatter_subarrays_to_other_ranks(array, rank_array, num_rows_per_rank, num_cols_per_rank, subarray_t);
```

A common feature of a domain decomposed algorithm is that communications are limited to a small number of other ranks
that work on a domain a short distance away.  For example, in a simulation of atomic crystals, updating a single atom
usually requires information from a couple of its nearest neighbours.

![Depiction of halo exchange communication pattern](fig/haloexchange.png)

In such a case each rank only needs a thin slice of data from its neighbouring rank and send the same slice from its own
data to the neighbour.  The data received from neighbours forms a "halo" around the ranks own data.

[`MPI_Sendrecv()`](https://www.open-mpi.org/doc/v4.1/man3/MPI_Sendrecv_replace.3.php)

```c
const prev_rank = my_rank - 1 < 0 ? MPI_PROC_NULL : my_rank - 1;
const next_rank = my_rank + 1 > num_ranks - 1 ? MPI_PROC_NULL : my_rank + 1;

/* Top row to bottom row */
MPI_Sendrecv(&image[index_into_2d(0, 1, num_cols)], num_rows, MPI_DOUBLE, prev_rank, 0,
            &image[index_into_2d(num_rows - 1, 1, num_cols)], num_rows, MPI_DOUBLE, next_rank, 0,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

/* Bottom row into top row */
MPI_Sendrecv(&image[index_into_2d(num_rows - 2, 1, num_cols)], num_rows, MPI_DOUBLE, next_rank, 0,
            &image[index_into_2d(0, 1, num_cols)], num_rows, MPI_DOUBLE, prev_rank, 0, MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);
```

## All-to-All

In other cases, some information needs to be sent from every rank to every other rank in the system.  This is the most
problematic scenario; the large amount of communication required reduces the potential gain from designing a parallel
algorithm.  Nevertheless the performance gain may be worth the effort if it is necessary to solve the problem quickly.

> ## Exercise: what pattern to use
>
> This exercise should show some example code, and get the students thinking about how they might communicate data
>
> > ## Solution
> >
> > Here is the solution
> >
> {: .solution}
>
{: .challenge}