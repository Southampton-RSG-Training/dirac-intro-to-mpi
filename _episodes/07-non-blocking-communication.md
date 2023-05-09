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

In the previous two episodes, we introduced how to send messages between two specific ranks or collectively to multiple
ranks. In both cases, we used blocking communication functions which meant our program wouldn't progress until data has
been sent and received successfully. In this episodes, we'll introduce how to use non-blocking communication to
reduce the amount of dead time spent in communication functions.

## Why use non-blocking communication?

As we already know, communication doesn't happen for free in MPI. It takes time and computing power to happen. But the
CPU doesn't really need to take part in most of the communication, as it's usually a task for the network hardware to
deal with. To refresh what non-blocking communication is:

## Point-to-point communication

<https://cvw.cac.cornell.edu/mpip2p/waittestfree>

```c
/* identical to blocking send */
MPI_Isend();
MPI_Wait();

/* identical to blocking receive */
MPI_Irecv();
MPI_Wait();
```

* `MPI_Isend`
* `MPI_Irecv`
* `MPI_Wait`
* `MPI_Test`

```c

```

## Collective communication
