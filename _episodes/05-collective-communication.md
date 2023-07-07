---
title: Collective Communication
slug: "dirac-intro-to-mpi-collective-communication"
teaching: 0
exercises: 0
questions:
- How do I get data to more than one rank?
objectives:
- Understand what collective communication is and its advantages
- Learn how to use collective communication functions
keypoints:
- Using point-to-point communication to send/receive data to/from all ranks is inefficient
- It's far more efficient to send/receive data to/from multiple ranks by using collective operations
---

The previous episode showed how to send data from one rank to another, using point-to-point communication functions such
as `MPI_Ssend` and `MPI_Send`. If we wanted to send data from multiple ranks to a single rank to, for example, perform a
reduction to sum a number calculated on multiple ranks, we have to manually loop over each rank to send and receive the
data. This type of communication, where multiple ranks talk to one another, is called *collective communication*. The
code example below shows an example below of summing the number of each rank (on rank 0) and sending the sum to every
rank.

```c
MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

int sum;
MPI_Status status;

/* Rank 0 is the "root" rank, where we'll receive data and sum it up */
if (my_rank == 0) {
    sum = my_rank;
    /* Start by receiving the rank number from every rank, other than itself */
    for (int i = 1; i < num_ranks; ++i) {
        int recv_num;
        MPI_Recv(&recv_num, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
        sum += recv_num;  /* Increment sum */
    }
    /* Now sum has been calculated, send it back to every rank other than the root */
    for (int i = 1; i < num_ranks; ++i) {
        MPI_Send(&sum, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
} else {  /* All other ranks will send their rank number and receive sum */
    MPI_Send(&my_rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(&sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
}

printf("Rank %d has a sum of %d\n", my_rank, sum);
```

The above code works perfectly fine for its use case, but isn't very efficient when you start needing to transfer large
amounts of data, have lots of ranks, or when the workload across ranks is uneven with blocking communication. It's also
a lot of code to do not much, and is easy to make a mistake. A common mistake would be starting the loops over ranks
starting at rank 0, which would create a deadlock!

But we don't need to write code like this, not unless we want complete control over all communication. MPI has
implemented collective communications since it's earliest versions. The code above can be reproduced by a single
function call to a collective operation, which abstracts away the communication and simplifies the code we write.
Collective operations are also implemented far more efficiently than we could with the point-to-point communication
functions.

There are several collective operations that are implemented in the MPI standard. The most commonly-used are:

| Type | Description |
| - | - |
| Synchronisation | Wait until all processes have reached the same point in the program. |
| One-To-All | One rank sends the same message to all other ranks. |
| All-to-One | All ranks send data to a single rank. |
| All-to-All | All ranks have data and all ranks receive data. |

## Synchronisation

### Barrier

The most simple form of collective communication is a barrier. Barriers are used to synchronise ranks by adding a point
in a program where ranks *must* wait until all ranks have reached the same point. A barrier is a collective operation
because all ranks need to communicate with one another to know when they can leave the barrier. To create a barrier, we
use the `MPI_Barrier()` function, which has the following arguments,

```c
int MPI_Barrier(
    MPI_Comm communicator  /* The communicator we want to add a barrier for */
);
```

When a rank reaches a barrier, it will stop executing any code and wait for all the other ranks to catch up and hit the
barrier as well. As ranks waiting at a barrier aren't doing anything, barriers should be used sparingly to avoid large
synchronisation overheads which will affect the scalability of our program. We should also avoid using barriers in parts
of our program has have complicated branches, as we may introduce a deadlock by having a barrier in only one branch and
not all branches.

In practise, there are not that many practical use cases for a barrier in an MPI application. In a shared-memory
environment, such as with OpenMP, synchronisation is important to ensure consistent and controlled access to shared
data. But in MPI, where each rank has its own private memory space and often resources, it's very rare that we need to
care about ranks becoming out-of-sync. However, there is one situation where a barrier useful which is when multiple
ranks need to write *sequentially* to the same file. The code example below shows how you may handle this using a
barrier.

```c
for (int i = 0; i < num_ranks; ++i) {
    if (i == my_rank) {           /* One rank writes to the file */
        write_to_file();
    }
    MPI_Barrier(MPI_COMM_WORLD);  /* Wait for data to be written, so it is sequential and ordered */
}
```

## One-To-All

### Broadcast

There are lot of situations where we'll need to get data from one rank to multiple ranks. One approach, which is not
very efficient, is to create a loop and use `MPI_Send()` to send the data to each rank one by one (like in the example
shown at the start of this episode). But a far more efficient approach is to *broadcast* the data to all ranks all at
once. We can do this by using the `MPI_Bcast()` function,

```c
int MPI_Bcast(
    void* data,             /* The data to be sent to all ranks */
    int count,              /* The number of elements of data */
    MPI_Datatype datatype,  /* The data type of the data */
    int root,               /* The rank which the data should be sent from */
    MPI_Comm comm           /* The communicator to handle the communication */
);
```

`MPI_Bcast()` is very similar to the `MPI_Send()` function. The only functional difference is that `MPI_Bcast()` sends
the data to all ranks (other than itself, where the data already is) instead of a single rank. This is shown in the
diagram below.

![Each rank sending a piece of data to root rank](fig/broadcast.png)

There are many use cases for broadcasting data from one rank to every other rank. One such case if when data is sent
back to a "root" rank to process, which then broadcasts the results back out to all ranks. Another example, shown in the
code exert below, is to read data in on a single rank and to broadcast the data from that file to other ranks. This can
be a useful pattern on some systems where there are not enough resources (filesystem bandwidth, limited concurrent I/O
operations) for all ranks to read the file at once.

```c
int data_from_file[NUM_POINTS]

/* Read in data from file, and put it into data_from_file. We are only reading data
   from the root rank (rank 0), as multiple ranks reading from the same file at the
   same time can sometimes result in problems or be slower */
if (my_rank == 0) {
    get_data_from_file(data_from_file);
}

/* Use MPI_Bcast to send the data to every other rank */
MPI_Bcast(data_from_file, NUM_POINTS, MPI_INT, 0, MPI_COMM_WORLD);
```

### Scatter

If we need to process a large amount of data, one way to parallelise the workload is to have each rank in our program
process a subset set of the data. But to do this, we need to send each rank the subset of data it needs to process. One
approach to doing this would be to have a "root" process which prepares the data and sends it to each rank. The data
could be communicated *manually* using point-to-point communication, but it's much easier, and faster, to use a single
collective communication function for doing this: `MPI_Scatter()`. We can use `MPI_Scatter()` to split and send
**equal** amounts of data from one rank to every rank in a communicator, as shown in the diagram below.

![Each rank sending a piece of data to root rank](fig/scatter.png)

`MPI_Scatter()` has the following arguments,

```c
int MPI_Scatter(
    void* sendbuf,          /* The data to be split across ranks (only important for the root rank) */
    int sendcount,          /* The number of elements of data to send to each rank (only important for the root rank) */
    MPI_Datatype sendtype,  /* The data type of the data being sent (only important for the root rank) */
    void* recvbuffer,       /* A buffer to receive the data, including the root rank */
    int recvcount,          /* The number of elements of data to receive, usually the same as sendcount */
    MPI_Datatype recvtype,  /* The data types of the data being received, usually the same as sendtype */
    int root,               /* The ID of the rank where data is being "scattered" from */
    MPI_Comm comm           /* The communicator involved */
);
```

The data to be *scattered* is split into even chunks of size `sendcount`. So, for example, if `sendcount` is 2 and
`sendtype` is `MPI_INT`, then each rank will receive two integers. The values for `recvcount` and `recvtype` are the
same as `sendcount` and `sendtype`. If the total amount of data is not evenly divisible by the number of processes,
`MPI_Scatter()` will not work. In this case, we need to use
[`MPI_Scatterv()`](https://www.open-mpi.org/doc/v4.0/man3/MPI_Scatterv.3.php) instead to specify the amount of data each
rank will receive. The code example below shows `MPI_Scatter()` being used to send data which has been initialised only
on the root rank.


```c
#define ROOT_RANK 0

int send_data[NUM_DATA_POINTS]

if (my_rank == ROOT_RANK) {
    initialise_send_data(send_data);  /* The data which we're going to scatter only needs to exist in the root rank */
}

/* Calculate the elements of data each rank will get, and allocate space for
   the receive buffer -- we are assuming NUM_DATA_POINTS is divisible by num_ranks */
int num_per_rank = NUM_DATA_POINTS / num_ranks;
int *scattered_data_for_rank = malloc(num_per_rank * sizeof(int));

/* Using a single function call, the data has been split and communicated evenly between all ranks */
MPI_Scatter(send_data, num_per_rank, MPI_INT, scattered_data_for_rank, num_per_rank, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
```

## All-To-One

### Gather

The opposite of *scattering* data across ranks, is to gather data to a single rank. To do such a thing, we can use the
collective function `MPI_Gather()`. We can think of `MPI_Gather()` as really being the inverse of `MPI_Scatter()`. This
is shown in the diagram below, where data from each rank on the left is sent to the root rank (rank 0) on the right.

![Each rank sending a piece of data to root rank](fig/gather.png)

The similarity between `MPI_Gather()` and `MPI_Scatter()` is also reflected by the fact that both functions have the
same arguments,

```c
int MPI_Gather(
    void* sendbuf,          /* */
    int sendcount,          /* */
    MPI_Datatype sendtype,  /* */
    void* recvbuffer,       /* */
    int sendcount,          /* */
    MPI_Datatype recvtype,  /* */
    int root,               /* */
    MPI_Comm comm           /* */
);
```

```c
int rank_data[NUM_DATA_POINTS];

/* Each rank generates some data, including the root rank */
for (int i = 0; i < NUM_DATA_POINTS; ++i) {
    rank_data[i] = (rank + 1) * (i + 1);
}

/* To gather all of the data, we need a buffer to store it. To make sure we have enough
   space, we need to make sure we allocate enough memory on the root rank */
int *gathered_data = malloc(NUM_DATA_POINTS * num_ranks * sizeof(int));

/* */
MPI_Gather(rank_data, NUM_DATA_POINTS, MPI_INT, gathered_data, NUM_DATA_POINTS, MPI_INT, 0, MPI_COMM_WORLD);
```


### Reduce

```c
int MPI_Reduce(
    void* sendbuf,          /* */
    void* recvbuffer,       /* */
    int count,              /* */
    MPI_Datatype datatype,  /* */
    MPI_Op op,              /* */
    int root,               /* */
    MPI_Comm comm           /* */
);
```

![Each rank sending a piece of data to root rank]({{ page.root }}{% link fig/reduction.png %})

Each rank sends a piece of data, which are combined on their way to rank `root` into a single piece of data.
For example, the function can calculate the sum of numbers distributed across
all the ranks.

Possible operations include:

| Operation | Description |
| - | - |
| `MPI_SUM`| Calculate the sum of numbers sent by each rank. |
| `MPI_MAX`| Return the maximum value of numbers sent by each rank. |
| `MPI_MIN`| Return the minimum of numbers sent by each rank. |
| `MPI_PROD` | Calculate the product of numbers sent by each rank. |
| `MPI_MAXLOC` | Return the maximum value and the number of the rank that sent the maximum value. |
| `MPI_MINLOC` | Return the minimum value of the number of the rank that sent the minimum value. |

In Python, these operations are named ``MPI.SUM``, ``MPI.MAX``, ``MPI.MIN``, and so on.
{: .show-python}

The `MPI_Reduce` operation is usually faster than what you might write by hand.
It can apply different algorithms depending on the system it's running on to reach the best
possible performance.
This is particularly the case on systems designed for high performance computing,
where the `MPI_Reduce` operations
can use the communication devices to perform reductions en route, without using any
of the ranks to do the calculation.

> ## In-place operations
>
> ```c
> MPI_Reduce(&sendbuf, MPI_IN_PLACE, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
> ```
>
{: .callout}

## All-to-All

### Allreduce

```c
int MPI_Allreduce(
    void* sendbuf,          /* */
    void* recvbuffer,       /* */
    int count,              /* */
    MPI_Datatype datatype,  /* */
    MPI_Op op,              /* */
    MPI_Comm comm           /* */
);
```

![Each rank sending a piece of data to root rank]({{ page.root }}{% link fig/allreduce.png %})

`MPI_Allreduce` performs essentially the same operations as `MPI_Reduce`,
but the result is sent to all the ranks.

```c
int sum;
int my_rank;
MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

/* This one function replaces the entire if statement in the first example */
MPI_Allreduce(&my_rank, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
```

---

> ## Sending and Receiving
>
> In the morning we wrote a hello world program where each rank
> sends a message to rank 0.
> Write this using a gather instead of send and receive.
>
>> ## Solution
>>
>> ```c
>>#include <stdio.h>
>>#include <stdlib.h>
>>#include <mpi.h>
>>
>>int main(int argc, char** argv) {
>>  int rank, n_ranks, numbers_per_rank;
>>  char send_message[40], *receive_message;
>>
>>  // First call MPI_Init
>>  MPI_Init(&argc, &argv);
>>  // Get my rank and the number of ranks
>>  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
>>  MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);
>>
>>  // Allocate space for all received messages in receive_message
>>  receive_message = malloc( n_ranks*40*sizeof(char) );
>>
>>  //Use gather to send all messages to rank 0
>>  sprintf(send_message, "Hello World, I'm rank %d\n", rank);
>>  MPI_Gather( send_message, 40, MPI_CHAR, receive_message, 40, MPI_CHAR, 0, MPI_COMM_WORLD );
>>
>>  if(rank == 0){
>>     for( int i=0; i<n_ranks; i++){
>>       printf("%s", receive_message + i*40);
>>     }
>>  }
>>
>>  // Free memory and finalise
>>  free( receive_message );
>>  return MPI_Finalize();
>>}
>> ```
>>
> {: .solution}
>
{: .challenge}

> ## Reductions
>
> The following program creates an array called `vector` that contains a list
> of `n_numbers` on each rank. The first rank contains the numbers from
> 1 to n_numbers, the second rank from n_numbers to 2*n_numbers2 and so on.
> It then calls the `find_max` and `find_sum` functions that should calculate the
> sum and maximum of the vector.
>
> These functions are not implemented in parallel and only return the sum and the
> maximum of the local vectors.
> Modify the `find_sum` and `find_max` functions to work correctly in parallel
> using `MPI_Reduce` or `MPI_Allreduce`.
>
> ```c
> #include <stdio.h>
> #include <mpi.h>
>
> // Calculate the sum of numbers in a vector
> double find_sum( double * vector, int N ){
>    double sum = 0;
>    for( int i=0; i<N; i++){
>       sum += vector[i];
>    }
>    return sum;
> }
>
> // Find the maximum of numbers in a vector
> double find_maximum( double * vector, int N ){
>    double max = 0;
>    for( int i=0; i<N; i++){
>       if( vector[i] > max ){
>          max = vector[i];
>       }
>    }
>    return max;
> }
>
>
> int main(int argc, char** argv) {
>    int n_numbers = 1024;
>    int rank;
>    double vector[n_numbers];
>    double sum, max;
>    double my_first_number;
>
>    // First call MPI_Init
>    MPI_Init(&argc, &argv);
>
>    // Get my rank
>    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
>
>    // Each rank will have n_numbers numbers,
>    // starting from where the previous left off
>    my_first_number = n_numbers*rank;
>
>    // Generate a vector
>    for( int i=0; i<n_numbers; i++){
>       vector[i] = my_first_number + i;
>    }
>
>    //Find the sum and print
>    sum = find_sum( vector, n_numbers );
>    printf("The sum of the numbers is %f\n", sum);
>
>    //Find the maximum and print
>    max = find_maximum( vector, n_numbers );
>    printf("The largest number is %f\n", max);
>
>    // Call finalize at the end
>    return MPI_Finalize();
> }
> ```
>
>> ## Solution
>>
>> ```c
>> // Calculate the sum of numbers in a vector
>> double find_sum( double * vector, int N ){
>>    double sum = 0;
>>    double global_sum;
>>
>>    // Calculate the sum on this rank as before
>>    for( int i=0; i<N; i++){
>>       sum += vector[i];
>>    }
>>
>>    // Call MPI_Allreduce to find the full sum
>>    MPI_Allreduce( &sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
>>
>>    return global_sum;
>> }
>>
>> // Find the maximum of numbers in a vector
>> double find_maximum( double * vector, int N ){
>>    double max = 0;
>>    double global_max;
>>
>>    // Calculate the sum on this rank as before
>>    for( int i=0; i<N; i++){
>>       if( vector[i] > max ){
>>          max = vector[i];
>>       }
>>    }
>>
>>    // Call MPI_Allreduce to find the maximum over all the ranks
>>    MPI_Allreduce( &max, &global_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );
>>
>>    return global_max;
>> }
>> ```
>>
> {: .solution}
>
{: .challenge}
