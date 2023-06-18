---
title: "Introduction to the Message Passing Interface"
slug: "dirac-intro-to-mpi-api"
teaching: 15
exercises: 0
questions:
- What is MPI?
objectives:
- Learn what the Message Passing Interface (MPI) is
- Understand how to use the MPI API
- Learn how to compile and run MPI applications
keypoints:
- The MPI standards define the syntax and semantics of a library of routines used for message passing.
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

In the example above, each copy of the program does not have knowledge of being started by `mpiexec` or `mpirun`, and each copy operates independently as if it were the only instance running. However, for the copies to collaborate and work together in a coordinated manner, they need to be aware of their respective roles in the computation. This typically involves knowing the total number of tasks running at the same time and achieved using set of MPI functions like `MPI_INIT` and `MPI_FINALIZE` which will be discussed in detail in the next episode.

## Submitting our program to the scheduler
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
