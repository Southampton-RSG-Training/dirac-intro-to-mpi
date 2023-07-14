---
title: Communication Patterns
slug: "dirac-intro-to-mpi-communication-patterns"
teaching: 0
exercises: 0
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

## Gather / Scatter

In **gather communication**, all ranks send a piece of information to one rank.  Gathers are typically used when
printing out information or writing to disk.  For example, each could send the result of a measurement to rank 0 and
rank 0 could print all of them. This is necessary if only one rank has access to the screen.  It also ensures that the
information is printed in order.

![Depiction of gather communication pattern, with each rank sending their data to a root rank](fig/gather.png)

Similarly, in a **scatter communication**, one rank sends a piece of data to all the other ranks.  Scatters are useful
for communicating parameters to all the ranks doing the computation.  The parameter could be read from disk but it could
also be produced by a previous computation.

![Depiction of scatter communication pattern](fig/scatter.png)

Gather and scatter operations require more communication as the number of ranks increases.  The amount of messages sent
usually increases logarithmically with the number of ranks.  They have efficient implementations in the MPI libraries.

## Halo Exchange

A common feature of a domain decomposed algorithm is that communications are limited to a small number of other ranks
that work on a domain a short distance away.  For example, in a simulation of atomic crystals, updating a single atom
usually requires information from a couple of its nearest neighbours.

![Depiction of halo exchange communication pattern](fig/haloexchange.png)

In such a case each rank only needs a thin slice of data from its neighbouring rank and send the same slice from its own
data to the neighbour.  The data received from neighbours forms a "halo" around the ranks own data.

## Reduction

A reduction is an operation that reduces a large amount of data, a vector or a matrix, to a single number.

![Depiction of reduction communication pattern](fig/reduction.png)

The sum example above is a reduction.  Since data is needed from all ranks, this tends to be a time consuming operation,
similar to a gather operation.  Usually each rank first performs the reduction locally, arriving at a single number.
They then perform the steps of collecting data from some of the ranks and performing the reduction on that data, until
all the data has been collected.  The most efficient implementation depends on several technical features of the system.
Fortunately many common reductions are implemented in the MPI library and are often optimised for a specific system.

## All-to-All

In other cases, some information needs to be sent from every rank to every other rank in the system.  This is the most
problematic scenario; the large amount of communication required reduces the potential gain from designing a parallel
algorithm.  Nevertheless the performance gain may be worth the effort if it is necessary to solve the problem quickly.
