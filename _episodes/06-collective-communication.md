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
-
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

```c
int MPI_Barrier(
    MPI_Comm communicator
);
```

Wait (doing nothing) until all ranks have reached this line.

## One-To-All

### Broadcast

```c
int MPI_Bcast(
    void* data,
    int count,
    MPI_Datatype datatype,
    int root,
    MPI_Comm communicator
);
```

![Each rank sending a piece of data to root rank]({{ page.root }}{% link fig/broadcast.png %})

Very similar to `MPI_Send`, but the same data is sent from rank `root` to all ranks.
This function will only return once all processes have reached it,
meaning it has the side-effect of acting as a barrier.

### Scatter

```c
int MPI_Scatter(
    void* sendbuf,
    int sendcount,
    MPI_Datatype sendtype,
    void* recvbuffer,
    int recvcount,
    MPI_Datatype recvtype,
    int root,
    MPI_Comm communicator
);
```

![Each rank sending a piece of data to root rank]({{ page.root }}{% link fig/scatter.png %})

The data in the `sendbuf` on rank `root` is split into chunks
and each chunk is sent to a different rank.
Each chunk contains `sendcount` elements of type `sendtype`.
So if `sendtype` is `MPI_Int`, and `sendcount` is 2,
each rank will receive 2 integers.
The received data is written to the `recvbuf`, so the `sendbuf` is only
needed by the `root`.
The next two parameters, `recvcount` and `recvtype` describe the receive buffer.
Usually `recvtype` is the same as `sendtype` and `recvcount` is `Nranks*sendcount`.

## All-To-One

### Gather

```c
int MPI_Gather(
    void* sendbuf,
    int sendcount,
    MPI_Datatype sendtype,
    void* recvbuffer,
    int sendcount,
    MPI_Datatype recvtype,
    int root,
    MPI_Comm communicator
);
```

![Each rank sending a piece of data to root rank]({{ page.root }}{% link fig/gather.png %})

Each rank sends the data in the `sendbuf` to rank `root`.
The `root` collects the data into the `recvbuffer` in order of the rank
numbers.

---

### Reduce

```c
int MPI_Reduce(
    void* sendbuf,
    void* recvbuffer,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPI_Comm communicator
);
```

![Each rank sending a piece of data to root rank]({{ page.root }}{% link fig/reduction.png %})

Each rank sends a piece of data, which are combined on their way to rank `root` into a single piece of data.
For example, the function can calculate the sum of numbers distributed across
all the ranks.

Possible operations include:

* `MPI_SUM`: Calculate the sum of numbers sent by each rank.
* `MPI_MAX`: Return the maximum value of numbers sent by each rank.
* `MPI_MIN`: Return the minimum of numbers sent by each rank.
* `MPI_PROD`: Calculate the product of numbers sent by each rank.
* `MPI_MAXLOC`: Return the maximum value and the number of the rank that sent the maximum value.
* `MPI_MINLOC`: Return the minimum value of the number of the rank that sent the minimum value.

In Python, these operations are named ``MPI.SUM``, ``MPI.MAX``, ``MPI.MIN``, and so on.
{: .show-python}

The `MPI_Reduce` operation is usually faster than what you might write by hand.
It can apply different algorithms depending on the system it's running on to reach the best
possible performance.
This is particularly the case on systems designed for high performance computing,
where the `MPI_Reduce` operations
can use the communication devices to perform reductions en route, without using any
of the ranks to do the calculation.

## All-to-All

### Allreduce

```c
int MPI_Allreduce(
    void* sendbuf,
    void* recvbuffer,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPI_Comm communicator
);
```

![Each rank sending a piece of data to root rank]({{ page.root }}{% link fig/allreduce.png %})

`MPI_Allreduce` performs essentially the same operations as `MPI_Reduce`,
but the result is sent to all the ranks.

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
