---
title: Communication Patterns
slug: "dirac-intro-to-mpi-communication-patterns"
teaching: 0
exercises: 0
questions:
-
objectives:
-
-
keypoints:
-
---


## Communication Patterns in MPI

In MPI parallelization we can make use of several different patterns of communication between ranks.
If you decide it's worth your time to try to parallelise the problem,
the next step is to decide how the ranks will share the tasks or the data.
This should be done for each region separately, but taking into account the time it would take to reorganise the data if you decide to change the pattern between regions.

The parallelisation strategy is often based on the
underlying problem the algorithm is dealing with.
For example, in materials science it makes sense
to decompose the data into domains by splitting the
physical volume of the material.

Most programs will use the same pattern in every region.
There is a cost to reorganising data, mostly having to do with communicating large blocks between ranks.
This is really only a problem if done in a tight loop, many times per second.

Let's take a look at these, and then decide which pattern (or patterns) will best suit the paralellisable
regions of our code.

### Gather / Scatter

In **gather communication**, all ranks send a piece of information to
one rank.  Gathers are typically used when printing out information or
writing to disk.  For example, each could send the result of a
measurement to rank 0 and rank 0 could print all of them. This is
necessary if only one rank has access to the screen.  It also ensures
that the information is printed in order.

<img src="fig/gather.png" alt="Depiction of gather communication pattern, with each rank sending their data to a root rank"/>

Similarly, in a **scatter communication**, one rank sends a piece of data
to all the other ranks.  Scatters are useful for communicating
parameters to all the ranks doing the computation.  The parameter
could be read from disk but it could also be produced by a previous
computation.

<img src="fig/scatter.png" alt="Depiction of scatter communication pattern, with each rank sending a piece of data to root rank"/>

Gather and scatter operations require more communication as the number
of ranks increases.  The amount of messages sent usually increases
logarithmically with the number of ranks.  They have efficient
implementations in the MPI libraries.

### Halo Exchange

A common feature of a domain decomposed algorithm is that
communications are limited to a small number of other ranks that work
on a domain a short distance away.  For example, in a simulation of
atomic crystals, updating a single atom usually requires information
from a couple of its nearest neighbours.

<img src="fig/haloexchange.png" alt="Depiction of halo exchange communication pattern"/>

In such a case each rank only needs a thin slice of data from its
neighbouring rank and send the same slice from its own data to the
neighbour.  The data received from neighbours forms a "halo" around
the ranks own data.

### Reduction

A reduction is an operation that reduces a large amount of data, a
vector or a matrix, to a single number.

<img src="fig/reduction.png" alt="Depiction of reduction communication pattern, adding a series of data from each rank together"/>

The sum example above is a
reduction.  Since data is needed from all ranks, this tends to be a
time consuming operation, similar to a gather operation.  Usually each
rank first performs the reduction locally, arriving at a single
number.  They then perform the steps of collecting data from some of
the ranks and performing the reduction on that data, until all the
data has been collected.  The most efficient implementation depends on
several technical features of the system.  Fortunately many common
reductions are implemented in the MPI library and are often optimised
for a specific system.

### All-to-All

In other cases, some information needs to be sent from every rank to
every other rank in the system.  This is the most problematic
scenario; the large amount of communication required reduces the
potential gain from designing a parallel algorithm.  Nevertheless the
performance gain may be worth the effort if it is necessary to solve
the problem quickly.

> ## Selecting Communication Patterns for our Parallel Code
>
> Which communication pattern(s) should you choose for our parallel code, taking in account:
>
> - How would you divide the data between the ranks?
> - When does each rank need data from other ranks?
> - Which ranks need data from which other ranks?
>
>> ## Solution
>>
>> Only one of the loops requires data from the other ranks,
>> and these are only nearest neighbours.
>>
>> Parallelising the loops would actually be the same thing as splitting the physical volume.
>> Each iteration of the loop accesses one element
>> in the `rho` and `unew` fields and four elements in
>> the `u` field.
>> The `u` field needs to be communicated if the value
>> is stored on a different rank.
>>
>> There is also a global reduction pattern needed for calculating `unorm`.
>> Every node needs to know the result.
>>
>{: .solution}
>
{: .challenge}
