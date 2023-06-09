---
title: "Writing Parallel Algorithms using MPI"
slug: "dirac-intro-to-mpi-parallel-algorithms"
teaching: 0
exercises: 0
questions:
- What do we mean by "parallel"?
- Which parts of a program are amenable to parallelisation?
- How do we characterise the classes of problems to which parallelism can be applied?
- How should I approach parallelising my program?
objectives:
- Learn and understand different parallel paradigms and algorithm design.
- Describe the differences between the data parallelism and message passing paradigms.
- Use MPI to coordinate the use of multiple processes across CPUs.
keypoints:
- Many problems can be distributed across several processors and solved faster.
- Algorithms can have both parallelisable and non-parallelisable sections.
- There are two major parallelisation paradigms: data parallelism and message passing.
- MPI implements the Message Passing paradigm, and OpenMP implements data parallelism.
- By default, the order in which operations are run between parallel MPI processes is arbitrary.
---

TODO: section intro

## Serial and Parallel Execution

An algorithm is a series of steps to solve a problem.  Let us imagine
these steps as a familiar scene in car manufacturing:
- Each step adds or adjusts a component to an existing structure
  to form a car as the conveyor belt moves.
- As a designer of these processes, you would carefully order the way
  you add components so that you don't have to disassemble
  already constructed parts.
- How long does it take to build a car in this way?

Now, you would like to build cars faster since there are impatient
customers waiting in line!  What do you do? You build two lines of
conveyor belts and hire twice number of people to do the work.  Now
you get two cars in the same amount of time!

- If your problem can be solved like this, you are in luck.
  You just need more CPUs (or cores) to do the work.
- This is called an "Embarrassingly Parallel (EP)" problem and is
  the easiest problem to parallelize.

But what if you're a boutique car manufacturer, making only a handful
of cars, and constructing a new assembly line would cost more time and
money than it would save? How can you produce cars more quickly
without constructing an extra line? The only way is to have multiple
people work on the same car at the same time:
- You identify the steps which don't interfere with each other and can
  be executed at the same time (e.g., the front part of the car can be
  constructed independently of the rear part of the car).
- If you find more steps which can be done without interfering with each other,
  these steps can be executed at the same time.
- Overall time is saved. More cars for the same amount of time!

In this case, the most important thing to consider is the
independence of a step (that is, whether a step can be executed
without interfering with other steps). This independency is
called "atomicity" of an operation.

In the analogy with car manufacturing, the speed of conveyor belt is
the "clock" of CPU, parts are "input", doing something is an
"operation", and the car structure on the conveyor belt is "output".
The algorithm is the set of operations that constructs a car from the
parts.  

<img src="fig/serial_task_flow.png" alt="Sequence of input data being processed by an algorithm to generate output data "/>

It consists of individual steps, adding parts or adjusting them.  
These could be done one after the other by a single worker.  

<img src="fig/serial_multi_task_flow.png" alt="Same sequential set of steps being done twice"/>

If we want to produce a car faster, maybe we can do some of the work
in parallel.  Let's say we can hire four workers to attach each of the
tires at the same time.  All of these steps have the same input, a car
without tires, and they work together to create an output, a car with
tires.

<img src="fig/parallel_simple_flow.png" alt="Reorganise steps into parallel execution"/>

The crucial thing that allows us to add the tires in parallel is that
they are independent.  Adding one tire does not prevent you from
adding another, or require that any of the other tires are added.  The
workers operate on different parts of the car.

### Data Dependency

Another example, and a common operation in scientific computing, is
the calculation of quantities that evolve in time or in an otherwise
iterative manner (e.g. molecular dynamics, gradient descent).
A simple implementation might look like this:
~~~
old_value = starting_point
for iteration in 1 ... 10000
   new_value = function(old_value)
   old_value = new_value
~~~
{: .source}
This is a very bad parallel algorithm! Every step, or iteration of
the for loop, depends on the value of the previous step.

The important factor that determines whether steps can be run in
parallel is data dependencies.  In our example above, every step
depends on data from the previous step, the value of the old_value
variable. When attaching tires to a car, attaching one tire does not
depend on attaching the others, so these steps can be done at the same
time.

However, attaching the front tires both require that the axis is
there.  This step must be completed first, but the two tires can then
be attached at the same time.

<img src="fig/parallel_complicated_flow.png" alt="Depiction of process with two steps being dependent on output from a previous step"/>

A part of the program that cannot be run in parallel is called a
"serial region" and a part that can be run in parallel is called a
"parallel region".  Any program will have some serial regions.  In a
good parallel program, most of the time is spent in parallel regions.
The program can never run faster than the sum of the serial regions.

>## Serial and Parallel regions
>
> Identify serial and parallel regions in the following algorithm
>
> ~~~
>  vector_1[0] = 1;
>  vector_1[1] = 1;
>  for i in 2 ... 1000
>    vector_1[i] = vector_1[i-1] + vector_1[i-2];
>
>  for i in 0 ... 1000
>    vector_2[i] = i;
>
>  for i in 0 ... 1000
>    vector_3[i] = vector_2[i] + vector_1[i];
>    print("The sum of the vectors is.", vector_3[i]);
>~~~
>{: .source}
>
>>## Solution
>>~~~
>> serial   | vector_0[0] = 1;
>>          | vector_1[1] = 1;
>>          | for i in 2 ... 1000
>>          |   vector_1[i] = vector_1[i-1] + vector_1[i-2];
>>
>> parallel | for i in 0 ... 1000
>>          |   vector_2[i] = i;
>>
>> parallel | for i in 0 ... 1000
>>          |   vector_3[i] = vector_2[i] + vector_1[i];
>>          |   print("The sum of the vectors is.", vector_3[i]);
>>
>> The first and the second loop could also run at the same time.
>>~~~
>>{: .source}
>>
>> In the first loop, every iteration depends on data from the previous two.
>> In the second two loops, nothing in a step depends on any of the other steps.
>{: .solution}
>
{: .challenge}

## Parallel Paradigms

How to achieve a parallel computation is divided roughly into two paradigms:

- In the _message passing paradigm_, each CPU (or core) runs an
  independent program. Parallelism is achieved by receiving data which it doesn't have,
  conducting some operations on this data, and sending data which it has.
- In the _data parallelism paradigm_, there are many different data,
  and the same operations are performed on these data at the same time.
  Parallelism is achieved by how much of the data a single
  operation can act on.

This division is mainly due to historical
development of parallel architectures: the first one follows from shared memory
architecture like SMP (Shared Memory Processor) and the second from
distributed computer architecture. A familiar example of the shared
memory architecture is GPU (or multi-core CPU) architecture, and an
example of the distributed computing architecture is a
cluster of distributed computers. Which architecture is more useful depends on what
kind of problems you have. Sometimes, one has to use both!

Consider a simple loop which can be sped up if we have many cores for illustration:

~~~
for(i=0; i<N; i++) {

  a[i] = b[i] + c[i];

}
~~~
{: .language-c}

If we have `N` or cores, each element of the loop can be computed in
just one step (for a factor of $$N$$ speed-up). Let's look into both paradigms
in a little more detail, and focus on key characteristics.

### Data Parallelism Paradigm

One standard method for programming using data parallelism is called
"OpenMP" (for "Open MultiProcessing"). 

To understand what data parallelism means, let's consider the following
bit of OpenMP code which parallelizes the above loop:

~~~
#pragma omp parallel for
for(i=0; i<N; i++) {

  a[i] = b[i] + c[i];

}
~~~
{: .language-c}

- Parallelization achieved by just one additional line,
  handled by the preprocessor in the compile stage.
- Possible since the computer system architecture supports OpenMP and all the
  complicated mechanisms for parallelization are hidden.
- Means that the system architecture has a shared memory view of
  variables and each core can access all of the memory address.
- The compiler "calculates" the address off-set for each core and let each
  one compute on a part of the whole data.

Here, the catch word is *shared memory* which allows all cores to access
all the address space. We'll be looking into OpenMP later
in this course.

In Python, process-based parallelism is supported by the
[multiprocessing](https://docs.python.org/dev/library/multiprocessing.html#module-multiprocessing)
module

### Message Passing Paradigm

In the message passing paradigm, each processor runs its own program
and works on its own data.  To work on the same problem in parallel,
they communicate by sending messages to each other.  Again using the
above example, each core runs the same program over a portion of the
data.  For example, using this paradigm to parallelise the above loop
instead:

~~~
for(i=0; i<m; i++) {

  a[i] = b[i] + c[i];

}
~~~
{: .language-c}

- Other than changing the number of loops from `N` to `m`, the code is
  exactly the same.
- `m` is the reduced number of loops each core needs to do (if there
  are `N` cores, `m` is 1 (= `N`/`N`)).

But the parallelization by message passing is not complete yet. In the
message passing paradigm, each core is independent from the other
cores. 

- Must make sure that each core has correct data to compute
  and output the results in correct order.
- Depends on the computer system.
- For example in a cluster computer, sometimes only one core has an access
  to the file system. This core reads in the whole data and sends the
  correct data to each core (including itself). MPI communications!
- After the computation, each core sends the result to that particular
  core, which core writes out the received data in a file in the
  correct order.
- If the cluster computer supports a parallel file system, each core
  reads the correct data from one file, computes and writes out the
  result to one file in correct order.

### Summary

In the end, both data parallelism and message passing logically achieve
the following:

<img src="fig/dataparallel.png" alt="Each rank has its own data"/>

Therefore, each rank essentially operates on its own set of data, regardless
of paradigm.

In some cases, there are advantages to combining data parallelism and 
message passing methods together, e.g. when there
are problems larger than one GPU can handle. In this case, _data
parallelism_ is used for the portion of the problem contained within
one GPU, and then _message passing_ is used to employ several GPUs (each
GPU handles a part of the problem) unless special hardware/software
supports multiple GPU usage. We'll be looking into this hybrid approach
later in this course.


## Algorithm Design

Designing a parallel algorithm that determines which of the two
paradigms above one should follow rests on the actual understanding of
how the problem can be solved in parallel. This requires some thought
and practice.

To get used to "thinking in parallel", we discuss "Embarrassingly
Parallel" (EP) problems first and then we consider problems which are
not EP problems.

### Embarrassingly Parallel Problems

Problems which can be parallelized most easily are EP problems, which
occur in many Monte Carlo simulation problems and in many big database
search problems. In Monte Carlo simulations, random initial conditions
are used in order to sample a real situation. So, a random number is
given and the computation follows using this random number. Depending
on the random number, some computation may finish quicker and some
computation may take longer to finish. And we need to sample a lot
(like a billion times) to get a rough picture of the real
situation. The problem becomes running the same code with a different
random number over and over again! In big database searches, one needs
to dig through all the data to find wanted data.  There may be just
one datum or many data which fit the search criterion. Sometimes, we
don't need all the data which satisfies the condition. Sometimes, we
do need all of them. To speed up the search, the big database is
divided into smaller databases, and each smaller databases are
searched independently by many workers!

#### Queue Method

Each worker will get tasks from a predefined queue (a random number in
a Monte Carlo problem and smaller databases in a big database search
problem).  The tasks can be very different and take different amounts
of time, but when a worker has completed its tasks, it will pick the
next one from the queue.

<img src="fig/queue.png" alt="Each rank taking one task from the top of a queue"/>

In an MPI code, the queue approach requires the ranks to communicate
what they are doing to all the other ranks, resulting in some
communication overhead (but negligible compared to overall task time).

#### Manager / Worker Method

The manager / worker approach is a more flexible version of the queue
method.  We hire a manager to distribute tasks to the workers.  The
manager can run some complicated logic to decide which tasks to give
to a worker.  The manager can also perform any serial parts of the
program like generating random numbers or dividing up the big
database. The manager can become one of the workers after finishing
managerial work.

<img src="fig/manager.png" alt="A manager rank controlling the queue"/>

In an MPI implementation, the main function will usually contain an
`if` statement that determines whether the rank is the manager or a
worker.  The manager can execute a completely different code from the
workers, or the manager can execute the same partial code as the
workers once the managerial part of the code is done. It depends on
whether the managerial load takes a lot of time to finish or
not. Idling is a waste in parallel computing!

Because every worker rank needs to communicate with the manager, the
bandwidth of the manager rank can become a bottleneck if
administrative work needs a lot of information (as we can observe in
real life).  This can happen if the manager needs to send smaller
databases (divided from one big database) to the worker ranks.  This
is a waste of resources and is not a suitable solution for an EP
problem.  Instead, it's better to have a parallel file system so that
each worker rank can access the necessary part of the big database
independently.

### General Parallel Problems (Non-EP Problems)

As we discussed in the 2nd lesson, in general not all the parts of a
serial code can be parallelized.  So, one needs to identify which part
of a serial code is parallelizable. In science and technology, many
numerical computations can be defined on a regular structured data
(e.g., partial differential equations in a 3D space using a finite
difference method). In this case, one needs to consider how to
decompose the domain so that many cores can work in parallel.

#### Domain Decomposition

When the data is structured in a regular way, such as when
simulating atoms in a crystal, it makes sense to divide the space
into domains. Each rank will handle the simulation within its
own domain.

<img src="fig/domaindecomposition.png" alt="Data points divided into four ranks"/>

Many algorithms involve multiplying very large matrices.  These
include finite element methods for computational field theories as
well as training and applying neural networks.  The most common
parallel algorithm for matrix multiplication divides the input
matrices into smaller submatrices and composes the result from
multiplications of the submatrices.  If there are four ranks, the
matrix is divided into four submatrices.

TODO: consider build support for matrix rendering

$$ A = \left[ \begin{array}{cc}A_{11} & A_{12} \\ A_{21} & A_{22}\end{array} \right] $$

$$ B = \left[ \begin{array}{cc}B_{11} & B_{12} \\ B_{21} & B_{22}\end{array} \right] $$

$$ A \cdot B = \left[ \begin{array}{cc}A_{11} \cdot B_{11} + A_{12} \cdot B_{21} & A_{11} \cdot B_{12} + A_{12} \cdot B_{22} \\ A_{21} \cdot B_{11} + A_{22} \cdot B_{21} & A_{21} \cdot B_{12} + A_{22} \cdot B_{22}\end{array} \right] $$

If the number of ranks is higher, each rank needs data from one row and one column to complete its operation.

#### Load Balancing

Even if the data is structured in a regular way and the domain is
decomposed such that each core can take charge of roughly equal
amounts of the sub-domain, the work that each core has to do may not
be equal. For example, in weather forecasting, the 3D spatial domain
can be decomposed in an equal portion. But when the sun moves across
the domain, the amount of work is different in that domain since more
complicated chemistry/physics is happening in that domain. Balancing
this type of loads is a difficult problem and requires a careful
thought before designing a parallel algorithm.


## Using MPI to Split Work Across Processes

As we saw earlier, running a program with `mpirun` starts several copies of it, e.g.:

~~~
mpirun -n 4 echo Hello World!
~~~
{: .language-bash}

- The number of copies is decided by the `-n` parameter.
- In the example above, the program does not know it was started by `mpirun`
- Each copy just works as if they were the only one.

For the copies to work together, they need to know about their role in
the computation.  This usually also requires knowing the total number
of tasks running at the same time.

- The program needs to call the `MPI_Init` function.
- `MPI_Init` sets up the environment for MPI, and assigns a number (called the _rank_) to each process.
- At the end, each process should also cleanup by calling `MPI_Finalize`.

~~~
int MPI_Init(&argc, &argv);
int MPI_Finalize();
~~~
{: .language-c}

Both `MPI_Init` and `MPI_Finalize` return an integer.
This describes errors that may happen in the function.
Usually we will return the value of `MPI_Finalize` from the main function

After MPI is initialized, you can find out the rank of the copy using the `MPI_Comm_rank` function:

~~~
int rank;
int MPI_Comm_rank(MPI_COMM_WORLD, &rank);
~~~
{: .language-c}

Here's a more complete example:

~~~
#include <stdio.h>
#include <mpi.h>

int main(int argc, char** argv) {
    int rank;

    // First call MPI_Init
    MPI_Init(&argc, &argv);
    
    // Get my rank
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("My rank number is %d\n", rank);

    // Call finalize at the end
    return MPI_Finalize();
}
~~~
{: .language-c}

TODO: consider getting COMM_size to example above, adding to printf

> ## Compile and Run
>
> Compile the above C code with `mpicc`, then run the code with `mpirun`. You may find the output for
> each rank is returned out of order. Why is this?
>
> > ## Solution
> > 
> > ~~~
> > mpicc mpi_rank.c -o mpi_rank
> > mpirun -n 4 mpi_rank
> > ~~~
> > {: .language-bash}
> >
> > You should see something like (although the ordering may be different):
> > 
> > ~~~
> > My rank number is 1
> > My rank number is 2
> > My rank number is 0
> > My rank number is 3
> > ~~~
> > 
> > The reason why the results are not returned in order is because the order in which the processes run
> > is arbitrary. As we'll see later, there are ways to synchronise processes to obtain a desired ordering!
> > 
> > {: .output}
> {: .solution}
{: .challenge}


> ## What About Python?
> 
> In [MPI for Python (mpi4py)](https://mpi4py.readthedocs.io/en/stable/), the
initialization and finalization of MPI are handled by the library, and the user
can perform MPI calls after ``from mpi4py import MPI``.
{: .callout}
