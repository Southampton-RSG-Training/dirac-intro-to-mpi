---
title: "Introduction to the Message Passing Interface"
slug: "dirac-intro-to-mpi-api"
math: true
teaching: 15
exercises: 0
questions:
- What is MPI?
- What do we mean by "parallel"?
- Which parts of a program are amenable to parallelisation?
- How do we characterise the classes of problems to which parallelism can be applied?
- How should I approach parallelising my program?
objectives:
- Learn what the Message Passing Interface (MPI) is
- Understand how to use the MPI API
- Learn how to compile and run MPI applications
- Learn and understand different parallel paradigms and algorithm design.
- Describe the differences between the data parallelism and message passing paradigms.
- Use MPI to coordinate the use of multiple processes across CPUs.
keypoints:
- The MPI standards define the syntax and semantics of a library of routines used for message passing.
- Algorithms can have both parallelisable and non-parallelisable sections.
- There are two major parallelisation paradigms: data parallelism and message passing.
- MPI implements the Message Passing paradigm, and OpenMP implements data parallelism.
- By default, the order in which operations are run between parallel MPI processes is arbitrary.
---

Thinking back to shared vs distributed memory models, how to achieve a parallel computation
is divided roughly into two paradigms. Let's set both of these in context:

- In a shared memory model, a _data parallelism_ paradigm is typically used, as employed by OpenMP: the same operations are
  performed simultaneously on data that is _shared_ across each parallel operation.
  Parallelism is achieved by how much of the data a single operation can act on.
- In a distributed memory model, a _message passing_ paradigm is used, as employed by MPI: each CPU (or core) runs an
  independent program. Parallelism is achieved by _receiving data_ which it doesn't have,
  conducting some operations on this data, and _sending data_ which it has.

This division is mainly due to historical
development of parallel architectures: the first one follows from shared memory
architecture like SMP (Shared Memory Processors) and the second from
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

Parallelization achieved by just one additional line, handled by the preprocessor in the compile stage, where the
compiler "calculates" the data address off-set for each core and lets each one compute on a part of the whole data.
This approach provides a convenient abstraction, and hides the underlying parallelisation mechanisms.

Here, the catch word is *shared memory* which allows all cores to access
all the address space. We'll be looking into OpenMP later
in this course.

In Python, process-based parallelism is supported by the
[multiprocessing](https://docs.python.org/dev/library/multiprocessing.html#module-multiprocessing)
module.

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
message passing paradigm, each core operates independently from the other
cores. So each core needs to be sent the correct data to compute, which then returns the output from that computation. 
However, we also need a core to coordinate the splitting up of that data, send portions of that data to other cores, 
and to receive the resulting computations from those cores.

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
supports multiple GPU usage.


## What is MPI?

MPI stands for ***Message Passing Interface*** and was developed in the early 1990s as a response to the need for a standardised approach to parallel programming. During this time, parallel computing systems were gaining popularity, featuring powerful machines with multiple processors working in tandem. However, the lack of a common interface for communication and coordination between these processors presented a challenge.

To address this challenge, researchers and computer scientists from leading vendors and organizations, including Intel, IBM, and Argonne National Laboratory, collaborated to develop MPI. Their collective efforts resulted in the release of the first version of the MPI standard, MPI-1, in 1994. This standardisation initiative aimed to provide a unified communication protocol and library for parallel computing.

> ## MPI versions
>  Since its inception, MPI has undergone several revisions, each 
> introducing new features and improvements:
> - **MPI-1 (1994):** The initial release of the MPI standard provided 
> a common set of functions, datatypes, and communication semantics.
> It formed the foundation for parallel programming using MPI.
> - **MPI-2 (1997):** This version expanded upon MPI-1 by introducing 
> additional features such as dynamic process management, one-sided 
> communication, paralell I/O, C++ and Fortran 90 bindings. MPI-2 improved the 
> flexibility and capabilities of MPI programs. 
> - **MPI-3 (2012):** MPI-3 brought significant enhancements to the MPI standard, 
> including support for non-blocking collectives, improved multithreading, and 
> performance optimizations. It also addressed limitations from previous versions and 
> introduced fully compliant Fortran 2008 bindings. Moreover, MPI-3 completely 
> removed the deprecated C++ bindings, which were initially marked as deprecated in 
> MPI-2.2.
> - **MPI-4.0 (2021):** On June 9, 2021, the MPI Forum approved MPI-4.0, the latest 
> major release of the MPI standard. MPI-4.0 brings significant updates and new 
> features, including enhanced support for asynchronous progress, improved support 
> for dynamic and adaptive applications, and better integration with external 
> libraries and programming models.
>
> These revisions, along with subsequent updates and errata, have refined the MPI 
> standard, making it more robust, versatile, and efficient.
{: .solution} 
Today, various MPI implementations are available, each tailored to specific hardware architectures and systems. For example, [MPICH](https://www.mpich.org/), [Intel MPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library.html#gs.0tevpk), [IBM Spectrum MPI](https://www.ibm.com/products/spectrum-mpi), [MVAPICH](https://mvapich.cse.ohio-state.edu/) and [Open MPI](https://www.open-mpi.org/) are popular implementations that offer optimized performance, portability, and flexibility.

The key concept in MPI is **message passing**, which involves the explicit exchange of data between processes. Processes can send messages to specific destinations, broadcast messages to all processes, or perform collective operations where all processes participate. This message passing and coordination among parallel processes are facilitated through a set of fundamental functions provided by the MPI standard.Typically, their names start with `MPI_` followed by a specific function or datatype identifier. Here are some examples:
- **MPI_Init:** Initializes the MPI execution environment.
- **MPI_Finalize:** Finalises the MPI execution environment.
- **MPI_Comm_rank:** Retrieves the rank of the calling process within a communicator.
- **MPI_Comm_size:** Retrieves the size (number of processes) within a communicator.
- **MPI_Send:** Sends a message from the calling process to a destination process.
- **MPI_Recv:** Receives a message into a buffer from a source process.
- **MPI_Barrier:** Blocks the calling process until all processes in a communicator have reached this point.

It's important to note that these functions represent only a subset of the functions provided by the MPI standard. There are additional functions and features available for various communication patterns, collective operations, and more. In the following episodes, we will explore these functions in more detail, expanding our understanding of MPI and how it enables efficient message passing and coordination among parallel processes.

In general, an MPI program follows a basic outline that includes the following steps:

1. ***Initialization:*** The MPI environment is initialized using the `MPI_Init` function. This step sets up the necessary communication infrastructure and prepares the program for message passing.
2. ***Communication:*** MPI provides functions for sending and receiving messages between processes. The `MPI_Send` function is used to send messages, while the `MPI_Recv` function is used to receive messages.
3. ***Termination:*** Once the necessary communication has taken place, the MPI environment is finalised using the `MPI_Finalize` function. This ensures the proper termination of the program and releases any allocated resources.

## Getting Started with MPI

> ## MPI on HPC
>
> HPC clusters typically have **more than one version** of MPI available, so you may 
> need to tell it which one you want to use before it will give you access to it.
> First check the available MPI implementations/modules on the cluster using the command 
> below: 
> ~~~
> module avail
> ~~~
> {: .language-bash}
> This will display a list of available modules, including MPI implementations.
> As for the next step, you should choose the appropriate MPI implementation/module from the 
> list based on your requirements and load it using `module load <mpi_module_name>`. For 
> example, if you want to load open MPI version 4.0.5, you can use:
> ~~~
> module load openmpi/4.0.5
> ~~~
> {: .language-bash}
> This sets up the necessary environment variables and paths for the MPI implementation and 
> will give you access to the MPI library. If you are not sure which implementation/version 
> of MPI you should use on a particular cluster, ask a helper or consult your HPC 
> facility's  documentation. <span style="color:red"> **TODO: A link for docs for the 
> different clusters can be added, such as [DIaL](https://www630.lamp.le.ac.uk/Software/
> Compiling_code/#compiler-suites)? Which cluster/MPI implentation/version are we targeting?
> ** </span>
>
{: .callout}

## Running a code with MPI

Let's start with a simple C code that prints "Hello World!" to the console. Save the 
following code in a file named **`hello_world.c`**
~~~
#include <stdio.h>

int main (int argc, char *argv[]) {
    printf("Hello World!\n");
}
~~~
{: .language-c}

Although the code is not an MPI program, we can use the command `mpicc` to compile it. The 
`mpicc` command is essentially a wrapper around the underlying C compiler, such as 
**gcc**, providing additional functionality for compiling MPI programs. It simplifies the 
compilation process by incorporating MPI-specific configurations and automatically linking 
the necessary MPI libraries and header files. Therefore the below command generates an 
executable file named **hello_world** . 

~~~
mpicc -o hello_world hello_world.c 
~~~
{: .language-bash}
 
Now let's try the following command:
~~~
mpiexec -n 4 ./hello_world
~~~
{: .language-bash}

What did `mpiexec` do?
If `mpiexec` is not found, try `mpirun` instead. This is another common name for the 
command.

Just running a program with `mpiexec` or `mpirun` starts several copies of it. The number of copies is decided by the `-n` parameter, which specifies the number of instances or processes. The expected output would be as follows:

````
Hello World!
Hello World!
Hello World!
Hello World!
````

> ## `mpiexec` vs `mpirun`
> 
> When launching MPI applications and managing parallel processes, we often rely on commands 
> like `mpiexec` or `mpirun`. Both commands act as wrappers or launchers for MPI 
> applications, allowing us to initiate and manage the execution of multiple parallel 
> processes across nodes in a cluster. While the behavior and features of `mpiexec` and 
> `mpirun` may vary depending on the MPI implementation being used (such as OpenMPI, MPICH, 
> MS MPI, etc.), they are commonly used interchangeably and provide similar functionality.
>
> It is important to note that `mpiexec` is defined as part of the MPI standard, whereas 
> `mpirun` is not. While some MPI implementations may use one name or the other, or even 
> provide both as aliases for the same functionality, `mpiexec` is generally considered the 
> preferred command. Although the MPI standard does not explicitly require MPI 
> implementations to include `mpiexec`, it does provide guidelines for its implementation. 
> In contrast, the availability and options of `mpirun` can vary between different MPI 
> implementations. To ensure compatibility and adherence to the MPI standard, it is 
> recommended to primarily use `mpiexec` as the command for launching MPI applications and 
> managing parallel execution.
{: .callout}

In the example above, each copy of the program does not have knowledge of being started by `mpiexec` or `mpirun`, and each copy operates independently as if it were the only instance running. However, for the copies to collaborate and work together in a coordinated manner, they need to be aware of their respective roles in the computation. An important aspect of that
computation is how we plan to use parallelisation in the design our code, and that's dependent on the problem we are
trying to solve.


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

In general not all the parts of a
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

$$ A = \left[ \begin{array}{cc}A_{11} & A_{12} \\ A_{21} & A_{22}\end{array} \right] $$

$$ B = \left[ \begin{array}{cc}B_{11} & B_{12} \\ B_{21} & B_{22}\end{array} \right] $$

$$ A \cdot B = \left[ \begin{array}{cc}A_{11} \cdot B_{11} + A_{12} \cdot B_{21} & A_{11} \cdot B_{12} + A_{12} \cdot B_{22} \\ A_{21} \cdot B_{11} + A_{22} \cdot B_{21} & A_{21} \cdot B_{12} + A_{22} \cdot B_{22}\end{array} \right]$$

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

> ## Serial and Parallel Regions
>
> Identify the serial and parallel regions in the following algorithm:
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
>> ## Solution
>>
>> ~~~
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
>> ~~~
>> {: .source}
>>
>> In the first loop, every iteration depends on data from the previous two.
>> In the second two loops, nothing in a step depends on any of the other steps,
>> and therefore can be parallelised.
>{: .solution}
>
{: .challenge}

## Using MPI to Split Work Across Processes

As we saw earlier, running a program with `mpirun` starts several copies of it, e.g.
running our `hello_world` program across 4 processes:

~~~
mpirun -n 4 ./hello_world
~~~
{: .language-bash}

However, in the example above, the program does not know it was started by `mpirun`, and each copy just works as if 
they were the only one. For the copies to work together, they need to know about their role in
the computation, in order to properly take advantage of parallelisation. This usually also requires knowing the total
number of tasks running at the same time.

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

After MPI is initialized, you can find out the total number of ranks and the specific rank of the copy:

~~~
int size, rank;
int MPI_Comm_size(MPI_COMM_WORLD, &size);
int MPI_Comm_rank(MPI_COMM_WORLD, &rank);
~~~
{: .language-c}

Here, `MPI_COMM_WORLD` is a **communicator**, which is a collection of ranks that are able to exchange data
between one another. We'll look into these in a bit more detail in the next episode, but essentially we use
`MPI_COMM_WORLD` which is the default communicator which refers to all ranks.

Here's a more complete example:

~~~
#include <stdio.h>
#include <mpi.h>

int main(int argc, char** argv) {
    int size, rank;

    // First call MPI_Init
    MPI_Init(&argc, &argv);
    
    // Get total number of ranks and my rank
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("My rank number is %d out of %d\n", rank, size);

    // Call finalize at the end
    return MPI_Finalize();
}
~~~
{: .language-c}

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


## Submitting our Program to a Batch Scheduler

In practice, such parallel codes may well be executed on a local machine, particularly during development. However,
much greater computational power is often desired to reduce runtimes to tractable levels, by running such codes on
HPC infrastructures such as DiRAC. These infrastructures make use of batch schedulers, such as Slurm, to manage access 
to distributed computational resources. So give our simple hello world example, how would we run this on an HPC batch
scheduler such as Slurm?

Let’s create a slurm submission script called **`hello-world-job.sh`**. Open an editor (e.g. Nano) and type (or copy/paste) the following contents:
```
##!/usr/bin/env bash
SBATCH --account=yourProjectCode
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --time=00:01:00
#SBATCH --job-name=hello-world

mpirun -n 4 ./hello_world
```
We can now submit the job using the `sbatch` command and monitor its status with `squeue`:

```
sbatch hello_world-job.sh
squeue -u yourUsername
```
Note the job id and check your job in the queue. Take a look at the output file when the job completes.
