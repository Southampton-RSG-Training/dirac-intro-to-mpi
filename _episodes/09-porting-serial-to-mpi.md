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

