---
title: "Introduction to the Message Passing Interface"
slug: "dirac-intro-to-mpi-api"
math: true
teaching: 15
exercises: 0
questions:
- What is MPI?
- How to run a code with MPI?
objectives:
- Learn what the Message Passing Interface (MPI) is
- Understand how to use the MPI API
- Learn how to compile and run MPI applications
- Use MPI to coordinate the use of multiple processes across CPUs.
keypoints:
- The MPI standards define the syntax and semantics of a library of routines used for message passing.
- By default, the order in which operations are run between parallel MPI processes is arbitrary.
---

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
Today, various MPI implementations are available, each tailored to specific hardware architectures and systems. Popular implementations like  [MPICH](https://www.mpich.org/), [Intel MPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library.html#gs.0tevpk), [IBM Spectrum MPI](https://www.ibm.com/products/spectrum-mpi), [MVAPICH](https://mvapich.cse.ohio-state.edu/) and [Open MPI](https://www.open-mpi.org/) offer optimized performance, portability, and flexibility. For instance, MPICH is known for its efficient scalability on HPC systems, while Open MPI prioritizes extensive portability and adaptability, providing robust support for multiple operating systems, programming languages, and hardware platforms.

The key concept in MPI is **message passing**, which involves the explicit exchange of data between processes. Processes can send messages to specific destinations, broadcast messages to all processes, or perform collective operations where all processes participate. This message passing and coordination among parallel processes are facilitated through a set of fundamental functions provided by the MPI standard. Typically, their names start with `MPI_` and followed by a specific function or datatype identifier. Here are some examples:
- **MPI_Init:** Initializes the MPI execution environment.
- **MPI_Finalize:** Finalises the MPI execution environment.
- **MPI_Comm_rank:** Retrieves the rank of the calling process within a communicator.
- **MPI_Comm_size:** Retrieves the size (number of processes) within a communicator.
- **MPI_Send:** Sends a message from the calling process to a destination process.
- **MPI_Recv:** Receives a message into a buffer from a source process.
- **MPI_Barrier:** Blocks the calling process until all processes in a communicator have reached this point.

It's important to note that these functions represent only a subset of the functions provided by the MPI standard. There are additional functions and features available for various communication patterns, collective operations, and more. In the following episodes, we will explore these functions in more detail, expanding our understanding of MPI and how it enables efficient message passing and coordination among parallel processes.

In general, an MPI program follows a basic outline that includes the following steps:

1. ***Initialization:*** The MPI environment is initialized using the `MPI_Init()` function. This step sets up the necessary communication infrastructure and prepares the program for message passing.
2. ***Communication:*** MPI provides functions for sending and receiving messages between processes. The `MPI_Send` function is used to send messages, while the `MPI_Recv` function is used to receive messages.
3. ***Termination:*** Once the necessary communication has taken place, the MPI environment is finalised using the `MPI_Finalize()` function. This ensures the proper termination of the program and releases any allocated resources.

## Getting Started with MPI: MPI on HPC

HPC clusters typically have **more than one version** of MPI available, so you may
need to tell it which one you want to use before it will give you access to it.
First check the available MPI implementations/modules on the cluster using the command below:

~~~
 module avail
~~~
{: .language-bash}

This will display a list of available modules, including MPI implementations.
As for the next step, you should choose the appropriate MPI implementation/module from the
list based on your requirements and load it using `module load <mpi_module_name>`. For
example, if you want to load OpenMPI version 4.1.4, you can use (for example on COSMA):

~~~
module load openmpi/4.1.4
~~~
{: .language-bash}

You may also need to load a compiler depending on your environment, and may get a warning
as such. On COSMA for example, you need to do something like this beforehand:

~~~
module load gnu_comp/13.1.0
~~~
{: .language-bash}

This sets up the necessary environment variables and paths for the MPI implementation and
will give you access to the MPI library. If you are not sure which implementation/version
of MPI you should use on a particular cluster, ask a helper or consult your HPC
facility's  documentation.

## Running a code with MPI

Let's start with a simple C code that prints "Hello World!" to the console. Save the
following code in a file named **`hello_world.c`**
~~~
#include <stdio.h>

int main(int argc, char **argv) {
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

> ## What if `mpiexec` doesn't exist?
>
> If `mpiexec` is not found, try `mpirun` instead. This is another common name for the command.
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

The expected output would be as follows:

````
Hello World!
Hello World!
Hello World!
Hello World!
````
> ## What did `mpiexec` do?
>
> Just running a program with `mpiexec` creates several instances of our application. The number
> of instances is determined by the `-n` parameter, which specifies the desired number of
> processes. These instances are independent and execute different parts of the program
> simultaneously. Behind the scenes, `mpiexec` undertakes several critical tasks. It sets up
> communication between the processes, enabling them to exchange data and synchronize their
> actions. Additionally, `mpiexec` takes responsibility for allocating the instances across the
> available hardware resources, deciding where to place each process for optimal performance. It
> handles the spawning, killing, and naming of processes, ensuring proper execution and
> termination.
{: .callout}

## Using MPI in a Program

As we've just learned, running a program with `mpiexec` or `mpirun` results in the initiation of multiple instances, e.g. running our `hello_world` program across 4 processes:

~~~
mpirun -n 4 ./hello_world
~~~
{: .language-bash}

However, in the example above, the program does not know it was started by `mpirun`, and each copy just works as if they were the only one. For the copies to work together, they need to know about their role in the computation, in order to properly take advantage of parallelisation. This usually also requires knowing the total number of tasks running at the same time.

- The program needs to call the `MPI_Init()` function.
- `MPI_Init()` sets up the environment for MPI, and assigns a number (called the _rank_) to each process.
- At the end, each process should also cleanup by calling `MPI_Finalize()`.

~~~
int MPI_Init(&argc, &argv);
int MPI_Finalize();
~~~
{: .language-c}

Both `MPI_Init()` and `MPI_Finalize()` return an integer.
This describes errors that may happen in the function.
Usually we will return the value of `MPI_Finalize()` from the main function

> ## I don't use command line arguments
>
> If your main function has no arguments because your program doesn't use any command line arguments, you can instead
> pass `NULL` to `MPI_Init()` instead.
>
> ~~~c
> int main(void) {
>     MPI_Init(NULL, NULL);
>     return MPI_Finalize();
> }
> ~~~
>
{: .callout}



After MPI is initialized, you can find out the total number of ranks and the specific rank of the copy:

~~~
int num_ranks, my_rank;
int MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
int MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
~~~
{: .language-c}

Here, `MPI_COMM_WORLD` is a **communicator**, which is a collection of ranks that are able to exchange data
between one another. We'll look into these in a bit more detail in the next episode, but essentially we use
`MPI_COMM_WORLD` which is the default communicator which refers to all ranks.

Here's a more complete example:

~~~
#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv) {
    int num_ranks, my_rank;

    // First call MPI_Init
    MPI_Init(&argc, &argv);

    // Get total number of ranks and my rank
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    printf("My rank number is %d out of %d\n", my_rank, num_ranks);

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
> > My rank number is 1 out of 4
> > My rank number is 2 out of 4
> > My rank number is 3 out of 4
> > My rank number is 0 out of 4
> > ~~~
> >  {: .output}
> > The reason why the results are not returned in order is because the order in which the processes run
> > is arbitrary. As we'll see later, there are ways to synchronise processes to obtain a desired ordering!
>>
> {: .solution}
{: .challenge}

## Using MPI to Split Work Across Processes

We've seen how to use MPI within a program to determine the total number of ranks, as well as our own ranks. But how
should we approach using MPI to split up some work between ranks so the work can be done in parallel? Let's consider
one simple way of doing this.

Let's assume we wish to count the number of prime numbers between 1 and 100,000, and wished to split this workload
evenly between the number of CPUs we have available. We already know the number of total ranks `num_ranks`, our own
rank `my_rank`, and the total workload (i.e. 100,000 iterations), and using the information we can split the workload
evenly across our ranks. Therefore, given 4 CPUs, for each rank the work would be divided into 25,000 iterations per
CPU, as:

~~~
Rank 0: 1 - 25,000
Rank 1: 25,001 - 50,000
Rank 2: 50,001 - 75,000
Rank 3: 75,001 - 100,000
~~~
{: .output}

Note we start with rank 0, since MPI ranks start at zero.

We can work out the iterations to undertake for a given rank number, by working out:
- Work out the number of iterations per rank by dividing the total number of iterations we want to calculate by `num_ranks`
- Determine the start of the work iterations for this rank by multiplying our rank number by the iterations per rank, and adding 1
- Determine the end of the work iterations for this rank by working out the point just before the hypothetical start of the next rank

We could write this in C as:

~~~
// calculate how many iterations each rank will deal with
int iterations_per_rank = NUM_ITERATIONS / num_ranks;
int rank_start = (my_rank * iterations_per_rank) + 1;
int rank_end = (my_rank + 1) * iterations_per_rank;
~~~
{: .language-c}

We also need to cater for the case where work may not be distributed evenly across ranks, where the total work isn't
directly divisible by the number of CPUs. In which case, we adjust the last rank's end of iterations to be the total
number of iterations. This ensures the entire desired workload is calculated:

~~~
// catch cases where the work can't be split evenly
if (rank_end > NUM_ITERATIONS || (my_rank == (num_ranks-1) && rank_end < NUM_ITERATIONS)) {
    rank_end = NUM_ITERATIONS;
}
~~~
{: .language-c}

Now we have this information, within a single rank we can perform the calculation for counting primes between our assigned
subset of the problem, and output the result, e.g.:

~~~
// each rank is dealing with a subset of the problem between rank_start and rank_end
int prime_count = 0;
for (int n = rank_start; n <= rank_end; ++n) {
    bool is_prime = true;  // remember to include <stdbool.h>

    // 0 and 1 are not prime numbers
    if (n == 0 || n == 1) {
        is_prime = false;
    }

    // if we can only divide n by i, then n is not prime
    for (int i = 2; i <= n / 2; ++i) {
        if (n % i == 0) {
            is_prime = false;
            break;
        }
    }

    if (is_prime) {
        prime_count++;
    }
}
printf("Rank %d - primes between %d-%d is: %d\n", my_rank, rank_start, rank_end, prime_count);
~~~
{: .language-c}

To try this out, copy the full [example code](code/examples/02-count_primes.c), compile and then run it:

~~~
mpicc -o count_primes count_primes.c
mpiexec -n 2 count_primes
~~~
{: .language-bash}

Of course, this solution only goes so far. We can add the resulting counts from each rank together to get our final
number of primes between 1 and 100,000, but what would be useful would be to have our code somehow retrieve the results
from each rank and add them together, and output that overall result. More generally, ranks may need results from other
ranks to complete their own computations. For this we would need ways for ranks to communicate - the primary benefit of
MPI - which we'll look at in subsequent episodes.

> ## What About Python?
>
> In [MPI for Python (mpi4py)](https://mpi4py.readthedocs.io/en/stable/), the
initialization and finalization of MPI are handled by the library, and the user
can perform MPI calls after ``from mpi4py import MPI``.
{: .callout}


## Submitting our Program to a Batch Scheduler

In practice, such parallel codes may well be executed on a local machine particularly during development. However,
much greater computational power is often desired to reduce runtimes to tractable levels, by running such codes on
HPC infrastructures such as DiRAC. These infrastructures make use of batch schedulers, such as Slurm, to manage access
to distributed computational resources. So give our simple hello world example, how would we run this on an HPC batch
scheduler such as Slurm?

Let’s create a slurm submission script called `count_primes.sh`. Open an editor (e.g. Nano) and type
(or copy/paste) the following contents:

~~~
#!/usr/bin/env bash
#SBATCH --account=yourProjectCode
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --time=00:01:00
#SBATCH --job-name=count_primes

module load gnu_comp/13.1.0
module load openmpi/4.1.4

mpiexec -n 4 ./count_primes
~~~
{: .language-bash}

We can now submit the job using the `sbatch` command and monitor its status with `squeue`:

~~~
sbatch count_primes.sh
squeue -u yourUsername
~~~
{: .language-bash}

Note the job id and check your job in the queue. Take a look at the output file when the job completes. You should see
something like:

~~~
====
Starting job 6378814 at Mon 17 Jul 11:05:12 BST 2023 for user yourUsername.
Running on nodes: m7013
====
Rank 0 - count of primes between 1-25000: 2762
Rank 1 - count of primes between 25001-50000: 2371
Rank 2 - count of primes between 50001-75000: 2260
Rank 3 - count of primes between 75001-100000: 2199
~~~
{: .output}
