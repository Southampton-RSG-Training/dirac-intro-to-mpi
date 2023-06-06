---
title: Non-blocking Communication
slug: "dirac-intro-to-mpi-non-blocking-communication"
teaching: 0
exercises: 0
questions:
- What are the advantages of non-blocking communication?
- How do I use non-blocking communication?
objectives:
- Understand the advantages and disadvantages of non-blocking communication
- Know how to use non-blocking communication functions
keypoints:
- Non-blocking communication can lead to performance improvements over blocking communication
- However, it is usually more difficult to use non-blocking communication
---

In the previous episodes, we learnt how to send messages between two ranks or collectively to multiple ranks. In both
cases, we used blocking communication functions which meant our program wouldn't progress until data had been sent and
received successfully. It takes time, and computing power, to transfer data into buffers, to send that data around
(over the network) and to receive the data into another rank. For the most part, the CPU isn't actually doing anything,
which is a waste of CPU cycles.

## A brief (re-)introduction

Hello, this will be an introduction to non-blocking.

<img src="fig/non-blocking-wait.png" alt="Non-blocking communication" height="250"/>

<img src="fig/non-blocking-wait-data.png" alt="Non-blocking communication with data dependency" height="250"/>

> ## Advantages and disadvantages
>
> What are the main advantages of using non-blocking communication, compared to blocking? What about any disadvantages?
>
> > ## Solution
> >
> > Some of the advantages of non-blocking communication over blocking communication include:
> >
> > - Non-blocking communication gives us the ability to interleave communication with computation. By being able to use
> >   the CPU whilst the network is transmitting data, we create algorithms with more efficient hardware usage.
> > - Non-blocking communication also improve the scalability of our program, due to the smaller communication overhead
> >   and latencies associated with communicating between a large number of ranks.
> > - Non-blocking communication is more flexible, which allows for more sophisticated parallel and communication
> >   algorithms.
> >
> > On the other hand, some  disadvantages are:
> >
> > - It is more difficult to use non-blocking communication. Not only is it harder to handle errors and recover, you
> >   additionally have to worry about data synchronisation and dependency.
> > - Whilst typically using non-blocking communication, where appropriate, improves performance, it's not always clear
> >   cut or predictable if non-blocking will result in sufficient performance gains to justify the increased
> >   complexity.
> >
> {: .solution}
{: .challenge}

## Point-to-point communication

For each blocking communication function we've seen, a non-blocking variant exists. For example, if we take
`MPI_Send()`, the non-blocking variant is `MPI_Isend()` which has the arguments,

```c
int MPI_Isend(
    const void *buf,        /* The data to be sent */
    int count,              /* The number of elements of data to be sent */
    MPI_Datatype datatype,  /* The datatype of the data */
    int dest,               /* The rank to send data to */
    int tag,                /* The communication tag */
    MPI_Comm comm,          /* The communicator to use */
    MPI_Request *request,   /* The communication request handle */
);
```

The arguments are identical to `MPI_Send()`, other than the addition of the `*request` argument. This argument is known
as an *handle* (because it "handles" a communication request) which is used to track the progress of a (non-blocking)
communication.

When we use non-blocking communication, we have to follow it up with `MPI_Wait()` to synchronise the
program and make sure `*buf` is ready to be re-used. This is incredibly important to do. Suppose we are sending an array
of integers,

```c
MPI_Isend(some_ints, 5, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
some_ints[1] = 5;  /* !!! don't do this !!! */
```

Modifying `some_ints` before the send has completed is undefined behaviour, and can result in breaking results! For
example, if `MPI_Isend` decides to use its buffered mode then modifying `some_ints` before it's finished being copied to
the send buffer will means the wrong data is sent. Every non-blocking communication has to have a corresponding
`MPI_Wait()`, to wait and synchronise the program to ensure that the data being sent is ready to be modified again.
`MPI_Wait()` is a blocking function which will only return when a communication has finished.

```c
int MPI_Wait(
    MPI_Request *request,  /* The request handle for the communication to wait for */
    MPI_Status *status,    /* The status handle for the communication */
);
```

Once we have used `MPI_Wait()` and the communication has finished, we can safely modify `some_ints` again. To receive
the data send using a non-blocking send, we can use either the blocking `MPI_Recv()` or it's non-blocking variant,

```c
int MPI_Irecv(
    void *buf,              /* The buffer to receive data into */
    int count,              /* The number of elements of data to receive */
    MPI_Datatype datatype,  /* The datatype of the data being received */
    int source,             /* The rank to receive data from */
    int tag,                /* The communication tag */
    MPI_Comm comm,          /* The communicator to use */
    MPI_Request *request,   /* The communication request handle */
);
```

> ## Naming conventions
>
> Non-blocking functions have the same name as their blocking counterpart, but prefixed with "I". The "I" stands for
> "immediate", indicating that the function returns immediately and does not block the program whilst data is being
> communicated in the background. The table below shows some examples of blocking functions and their non-blocking
> counterparts.
>
> | Blocking | Non-blocking|
> | -------- | ----------- |
> | `MPI_Bsend()` | `MPI_Ibsend()` |
> | `MPI_Barrier()` | `MPI_Ibarrier()` |
> | `MPI_Reduce()` | `MPI_Ireduce()` |
>
{: .callout}

> ## True or false?
>
> Is the following statement true or false? Non-blocking communication guarantees immediate completion of data transfer.
>
> > ## Solution
> >
> > False. Just because the communication function has returned,  does not mean the communication has finished and the
> > communication buffer is ready to be re-used or read from. Before we access, or edit, any data which has been used in
> > non-blocking communication, we always have to test/wait for the communication to finish using `MPI_Wait()` before it
> > is safe to use it.
> >
> {: .solution}
{: .challenge}

In the example below, an array of integers (`some_ints`) is sent from rank 0 to rank 1 using non-blocking communication.

```c
MPI_Status status;
MPI_Request request;

if (my_rank == 0) {
    MPI_Isend(some_ints, 5, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
    some_ints[1] = 5;  /* After MPI_Wait(), some_ints has been sent and can be modified again */
} else {
    MPI_Irecv(recv_ints, 5, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
    rank_1_some_ints[1] = recv_ints[2];  /* recv_ints isn't guaranteed to have the correct data until after MPI_Wait()*/
}
```

The code above is functionally identical to blocking communication, because of `MPI_Wait()` is blocking. The program
will not continue until `MPI_Wait()` returns. Since there is no additional work between the communication call and
blocking wait, this is a poor example of how non-blocking communication should be used. It doesn't take advantage of the
asynchronous nature of non-blocking communication at all. To really make use of non-blocking communication, we need to
interleave computation (or any busy work we need to do) with communication, such as as in the next example.

```c
if (my_rank == 0) {
    /* This send important_data without being blocked and move into the next work */
    MPI_Isend(important_data, 16, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
} else {
    /* Start listening for the message from the other rank, but isn't blocked */
    MPI_Irecv(important_data, 16, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
}

/* Whilst the message is still sending or received, we should do some other work
   to keep using the CPU (which isn't required for most of the communication.
   IMPORTANT: the work here cannot modify or rely on important_data */
clear_model_parameters();
initialise_model();

/* Once we've done the work which doesn't require important_data, we need to wait until the
   data is sent/received if it hasn't already */
MPI_Wait(&request, &status);

/* Now the model is ready and important_data has been sent/received, the simulation
   carries on */
simulate_something(important_data);
```

> ## Deadlocks
>
> Deadlocks are easily created when using blocking communication. The code snippet below shows an example of deadlock
> from one of the previous episodes.
>
> ```c
> if (my_rank == 0) {
>     MPI_Send(&numbers, 8, MPI_INT, 1, 0, MPI_COMM_WORLD);
>     MPI_Recv(&numbers, 8, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
> } else {
>     MPI_Send(&numbers, 8, MPI_INT, 0, 0, MPI_COMM_WORLD);
>     MPI_Recv(&numbers, 8, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
> }
> ```
>
> If we changed to non-blocking communication, do you think there would still be a deadlock? Try writing your own
> non-blocking version.
>
> > ## Solution
> >
> > The non-blocking version of the code snippet may look something like this:
> >
> > ```c
> > MPI_Request send_req, recv_req;
> >
> > if (my_rank == 0) {
> >     MPI_Isend(&numbers, 8, MPI_INT, 1, 0, MPI_COMM_WORLD, &send_req);
> >     MPI_Irecv(&numbers, 8, MPI_INT, 1, 0, MPI_COMM_WORLD, &recv_req);
> > } else {
> >     MPI_Isend(&numbers, 8, MPI_INT, 0, 0, MPI_COMM_WORLD, &send_req);
> >     MPI_Irecv(&numbers, 8, MPI_INT, 0, 0, MPI_COMM_WORLD, &recv_req);
> > }
> >
> > MPI_Status statuses[2];
> > MPI_Request requests[2] = { send_req, recv_req };
> > MPI_Waitall(2, requests, statuses);  /* Wait for both requests in one call */
> > ```
> >
> > This version of the code will not deadlock, because the non-blocking functions return immediately. So even though
> > rank 0 and 1 one both send, meaning there is no corresponding receive, the immediate return from send means the
> > receive function is still called. Thus a deadlock cannot happen.
> >
> {: .solution}
>
>
{: .challenge}

## To wait, or not to wait

<https://cvw.cac.cornell.edu/mpip2p/waittestfree>

`MPI_Wait()` is blocking. `MPI_Test()` is the non-blocking counterpart.

The `MPI_Test()` function is used to test if a communication has finished, without blocking the execution of the
program.

```c
int MPI_Test(
    MPI_Request *request,  /* The request handle for the communication to test */
    int *flag,             /* A flag to indicate if the communication has completed */
    MPI_Status *status,    /* The status handle for the communication to test */
);
```

```c
MPI_Status status;
MPI_Request request;
MPI_Irecv(recv_buffer, 16, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);

/* We need to define a flag, to track when the communication has completed */
int flag = false;

/* One example use case is keep checking if the communication has finished, and continuing
   to do CPU work until it has */
while (!flag) {
    do_some_other_stuff();
    /* MPI_Test will return flag == true when the communication has finished */
    MPI_Test(&request, &flag, &status);
}
```

Another use case of `MPI_Test()` is to emulate an event driven scheduler.

- do some spare work whilst waiting for data for your main work

> ## Try it yourself
>
> In the MPI program below, a chain of ranks has been set up so one rank will receive a message from the rank to its
> left and send a message to the one on its right, as shown in the diagram below:
>
> <img src="fig/rank_chain.png" alt="A chain of ranks" height="100">
>
> For for following skeleton below, use non-blocking communication to send `send_message` to the right right and
> receive a message from the left rank. Create two programs, one using `MPI_Wait()` and the other using `MPI_Test()`.
>
> ```c
> #include <mpi.h>
> #include <stdio.h>
>
> #define MESSAGE_SIZE 32
>
> int main(int argc, char **argv)
> {
>     int my_rank;
>     int num_ranks;
>     MPI_Init(&argc, &argv);
>     MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
>     MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
>
>     if (num_ranks < 2) {
>         printf("This example requires at least two ranks\n");
>         MPI_Abort(1);
>     }
>
>     char send_message[MESSAGE_SIZE];
>     char recv_message[MESSAGE_SIZE];
>     sprintf(send_message, "Hello from rank %d!", my_rank);
>
>     int right_rank = (my_rank + 1) % num_ranks;
>     int left_rank = my_rank < 1 ? num_ranks - 1 : my_rank - 1;
>
>     /*
>      * Your code goes here
>      */
>
>     return MPI_Finalize();
> }
> ```
>
> > ## Solution
> >
> > Note that in the solution below, we started listening for the message before `send_message` even had its message
> > ready!
> >
> > ```c
> > #include <mpi.h>
> > #include <stdbool.h>
> > #include <stdio.h>
> >
> > #define MESSAGE_SIZE 32
> >
> > int main(int argc, char **argv)
> > {
> >     int my_rank;
> >     int num_ranks;
> >     MPI_Init(&argc, &argv);
> >     MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
> >     MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
> >
> >     if (num_ranks < 2) {
> >         printf("This example requires at least two ranks\n");
> >         return MPI_Finalize();
> >     }
> >
> >     char send_message[MESSAGE_SIZE];
> >     char recv_message[MESSAGE_SIZE];
> >
> >     int right_rank = (my_rank + 1) % num_ranks;
> >     int left_rank = my_rank < 1 ? num_ranks - 1 : my_rank - 1;
> >
> >     MPI_Status send_status, recv_status;
> >     MPI_Request send_request, recv_request;
> >
> >     MPI_Irecv(recv_message, MESSAGE_SIZE, MPI_CHAR, left_rank, 0, MPI_COMM_WORLD, &recv_request);
> >
> >     sprintf(send_message, "Hello from rank %d!", my_rank);
> >     MPI_Isend(send_message, MESSAGE_SIZE, MPI_CHAR, right_rank, 0, MPI_COMM_WORLD, &send_request);
> >     MPI_Wait(&send_request, &send_status);
> >
> >     MPI_Wait(&recv_request, &recv_status);
> >
> >     /* But maybe you prefer MPI_Test()
> >
> >     int recv_flag = false;
> >     while (recv_flag == false) {
> >         MPI_Test(&recv_request, &recv_flag, &recv_status);
> >     }
> >
> >     */
> >
> >     printf("Rank %d: message received -- %s\n", my_rank, recv_message);
> >
> >     return MPI_Finalize();
> > }
> > ```
> >
> {: .solution}
{: .challenge}

## Collective communication

Since MPI-3.0, non-blocking collective communication has been possible.

```c
MPI_Request request;
MPI_Status status;

int recv_data;
int send_data = my_rank + 1;

/* */
MPI_Iallreduce(&send_data, &recv_data, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
/* */
MPI_Wait(&request, &status);
```

> ## Try it yourself
>
>
> > ## Solution
> >
> {: .solution}
{: .challenge}
