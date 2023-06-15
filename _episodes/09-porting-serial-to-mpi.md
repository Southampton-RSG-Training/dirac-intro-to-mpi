---
title: Porting Serial Code to MPI
slug: "dirac-intro-to-mpi-porting-serial-to-mpi"
teaching: 0
exercises: 0
questions:
- What is the best way to write a parallel code?
- How do I parallelise my existing serial code?
objectives:
- Learn how to identify which parts of a codebase should be parallelised
- Convert a serial code into a parallel code
- Differentiate between choices of communication pattern and algorithm design
keypoints:
- Start from a working serial code
- Write a parallel implementation for each function or parallel region
- Connect the parallel regions with a minimal amount of communication
- Continuously compare the developing parallel code with the working serial code
---

In this section we will look at converting a complete code rom serial to
parallel in a couple of steps.

The exercises and solutions are based on a code that solves the Poisson's equation using an iterative method.
In this case the equation describes how heat diffuses in a metal stick.
In the simulation the stick is split into small sections with a constant
temperature.
At one end the amount of heat is set to 10 and at the other to 0.
The code applies steps that bring each point closer to a solution
until it reaches an equilibrium.

~~~
/* A serial code for Poisson equation 
 * This will apply the diffusion equation to an initial state
 * until an equilibrium state is reached. */

/* contact seyong.kim81@gmail.com for comments and questions */


#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define GRIDSIZE 10


/* Apply a single time step */
double poisson_step( 
     float *u, float *unew, float *rho,
     float hsq, int points
   ){
   double unorm;
 
   // Calculate one timestep
   for( int i=1; i <= points; i++){
      float difference = u[i-1] + u[i+1];
      unew[i] = 0.5*( difference - hsq*rho[i] );
   }
 
   // Find the difference compared to the previous time step
   unorm = 0.0;
   for( int i = 1;i <= points; i++){
      float diff = unew[i]-u[i];
      unorm += diff*diff;
   }
 
   // Overwrite u with the new value
   for( int i = 1;i <= points;i++){
      u[i] = unew[i];
   }
 
   return unorm;
}

int main(int argc, char** argv) {

   // The heat energy in each block
   float u[GRIDSIZE+2], unew[GRIDSIZE+2], rho[GRIDSIZE+2];
   int i;
   float h, hsq;
   double unorm, residual;

   /* Set up parameters */
   h = 0.1;
   hsq = h*h;
   residual = 1e-5;
 
   // Initialise the u and rho field to 0 
   for(i=0; i <= GRIDSIZE+1; i++) {
      u[i] = 0.0;
      rho[i] = 0.0;
   }

   // Create a start configuration with the heat energy
   // u=10 at the x=0 boundary
   u[0] = 10.0;
 
   // Run iterations until the field reaches an equilibrium
   // and no longer changes
   for( i=0; i<10000; i++ ) {
     unorm = poisson_step( u, unew, rho, hsq, GRIDSIZE );
     printf("Iteration %d: Residue %g\n", i, unorm);
     
     if( sqrt(unorm) < sqrt(residual) ){
       break;
     }
   }
 
   printf("Run completed with residue %g\n", unorm);
}
~~~
{: .language-c}


### Parallel Regions

The first step is to identify parts of the code that
can be written in parallel.
Go through the algorithm and decide for each region if the data can be partitioned for parallel execution,
or if certain tasks can be separated and run in parallel.

Can you find large or time consuming serial regions?
The sum of the serial regions gives the minimum amount of time it will take to run the program.
If the serial parts are a significant part of the algorithm, it may not be possible to write an efficient parallel version.
Can you replace the serial parts with a different, more parallel algorithm?


> ## Parallel Regions
>
> Identify any parallel and serial regions in the code.
> What would be the optimal parallelisation strategy?
>
>> ## Solution
>>
>> The loops over space can be run in parallel.
>> There are parallisable loops in:
>> * the setup, when initialising the fields.
>> * the calculation of the time step, `unew`.
>> * the difference, `unorm`.
>> * overwriting the field `u`.
>> * writing the files could be done in parallel.
>>
>> The best strategy would seem to be parallelising the loops.
>> This will lead to a domain decomposed implementation with the
>> elements of the fields `u` and `rho` divided across the ranks.
>>
>{: .solution}
>
{: .challenge}

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


### Write a Parallel Function Thinking About a Single Rank

In the message passing framework, all ranks execute the same code.
When writing a parallel code with MPI, you should think of a single rank.
What does this rank need to do, and what information does it need to do it?

Communicate data in the simplest possible way
just after it's created or just before it's needed.
Use blocking or non-blocking communication, whichever you feel is simpler.
If you use non-blocking functions, call wait immediately.

The point is to write a simple code that works correctly.
You can optimise later.

> ## Parallel Execution
>
> First, just implement a single step.
> Write a program that performs the iterations from
> `j=rank*(GRIDSIZE/n_ranks)` to `j=(rank+1)*(GRIDSIZE/n_ranks)`.
> For this, you should not need to communicate the field `u`.
> You will need a reduction.
>
>> ## Solution
>>
>>~~~
>> double poisson_step( 
>>     float *u, float *unew, float *rho,
>>     float hsq, int points
>>   ){
>>   /* We will calculate unorm separately on each rank. It needs to be summed up at the end*/
>>   double unorm;
>>   /* This will be the sum over all ranks */
>>   double global_unorm;
>> 
>>   // Calculate one timestep
>>   for( int i=1; i <= points; i++){
>>      float difference = u[i-1] + u[i+1];
>>      unew[i] = 0.5*( difference - hsq*rho[i] );
>>   }
>> 
>>   // Find the difference compared to the previous time step
>>   unorm = 0.0;
>>   for( int i = 1;i <= points; i++){
>>      float diff = unew[i]-u[i];
>>      unorm += diff*diff;
>>   }
>> 
>>   // Use Allreduce to calculate the sum over ranks
>>   MPI_Allreduce( &unorm, &global_unorm, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
>> 
>>   // Overwrite u with the new field
>>   for( int i = 1;i <= points;i++){
>>      u[i] = unew[i];
>>   }
>> 
>>   // Return the sum over all ranks
>>   return global_unorm;
>> }
>>~~~
>>{: .source .language-c}
>{: .solution}
{: .challenge}

> ## Communication
>
> Add in the nearest neighbour communication.
>
>> ## Solution
>> Each rank needs to send the values at `u[1]` down to `rank-1` and
>> the values at `u[my_j_max]` to `rank+1`.
>> There needs to be an exception for the first and the last rank.
>>
>>~~~
>> double poisson_step( 
>>   float *u, float *unew, float *rho,
>>   float hsq, int points
>> ){
>>   double unorm, global_unorm;
>>   float sendbuf, recvbuf;
>>   MPI_Status mpi_status;
>>   int rank, n_ranks;
>>  
>>   /* Find the number of x-slices calculated by each rank */
>>   /* The simple calculation here assumes that GRIDSIZE is divisible by n_ranks */
>>   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
>>   MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);
>>    
>>   // Calculate one timestep
>>   for( int i=1; i <= points; i++){
>>      float difference = u[i-1] + u[i+1];
>>      unew[i] = 0.5*( difference - hsq*rho[i] );
>>   }
>> 
>>   // Find the difference compared to the previous time step
>>   unorm = 0.0;
>>   for( int i = 1;i <= points; i++){
>>      float diff = unew[i]-u[i];
>>      unorm += diff*diff;
>>   }
>>  
>>   // Use Allreduce to calculate the sum over ranks
>>   MPI_Allreduce( &unorm, &global_unorm, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
>>  
>>   // Overwrite u with the new field
>>   for( int i = 1;i <= points;i++){
>>      u[i] = unew[i];
>>   }
>>  
>>   // The u field has been changed, communicate it to neighbours
>>   // With blocking communication, half the ranks should send first
>>   // and the other half should receive first
>>   if ((rank%2) == 1) {
>>     // Ranks with odd number send first
>>  
>>     // Send data down from rank to rank-1
>>     sendbuf = unew[1];
>>     MPI_Send(&sendbuf,1,MPI_FLOAT,rank-1,1,MPI_COMM_WORLD);
>>     // Receive dat from rank-1
>>     MPI_Recv(&recvbuf,1,MPI_FLOAT,rank-1,2,MPI_COMM_WORLD,&mpi_status);
>>     u[0] = recvbuf;
>>      
>>     if ( rank != (n_ranks-1)) {
>>       // Send data up to rank+1 (if I'm not the last rank)
>>       MPI_Send(&u[points],1,MPI_FLOAT,rank+1,1,MPI_COMM_WORLD);
>>       // Receive data from rank+1
>>       MPI_Recv(&u[points+1],1,MPI_FLOAT,rank+1,2,MPI_COMM_WORLD,&mpi_status);
>>     }
>>   
>>   } else {
>>     // Ranks with even number receive first
>>  
>>     if (rank != 0) {
>>       // Receive data from rank-1 (if I'm not the first rank)
>>       MPI_Recv(&u[0],1,MPI_FLOAT,rank-1,1,MPI_COMM_WORLD,&mpi_status);
>>       // Send data down to rank-1
>>       MPI_Send(&u[1],1,MPI_FLOAT,rank-1,2,MPI_COMM_WORLD);
>>     }
>>  
>>     if (rank != (n_ranks-1)) {
>>       // Receive data from rank+1 (if I'm not the last rank)
>>       MPI_Recv(&u[points+1],1,MPI_FLOAT,rank+1,1,MPI_COMM_WORLD,&mpi_status);
>>       // Send data up to rank+1
>>       MPI_Send(&u[points],1,MPI_FLOAT,rank+1,2,MPI_COMM_WORLD);
>>     }
>>   }
>>  
>>   return global_unorm;
>> }
>>~~~
>>{: .source .language-c}
>{: .solution}
{: .challenge}

