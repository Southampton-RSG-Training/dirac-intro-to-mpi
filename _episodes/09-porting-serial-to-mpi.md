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

In this section we will look at converting a complete code from serial to parallel in a number of steps.

## An Example Iterative Poisson Solver

This section is based on a code that solves the Poisson's equation using an iterative method.
Poisson's equation appears in almost every field in physics, and is frequently used to model many physical phenomena such as heat conduction, and applications of this equation exist for both two and three dimensions.
In this case, the equation is used in a simplified form to describe how heat diffuses in a one dimensional metal stick.

In the simulation the stick is split into a given number of small sections, each with a constant temperature.

FIXME: add diagram of stick split into sections here

The temperature of the stick itself across each section is initially set to zero, whilst at one boundary of the stick the amount of heat is set to 10.
The code applies steps that simulates heat transfer along it, bringing each section of the stick closer to a solution until it reaches a desired equilibrium in temperature along the whole stick.

Let's download the code, which can be found [here](/code/examples/poisson/poisson.c), and take a look at it now.

### At a High Level - main()

We'll begin by looking at the `main()` function at a high level.

~~~
#define MAX_ITERATIONS 25000
#define GRIDSIZE 12

...

int main(int argc, char** argv) {

  // The heat energy in each block
  float *u, *unew, *rho;
  float h, hsq;
  double unorm, residual;
  int i;

  u = malloc(sizeof(*u) * (GRIDSIZE+2));
  unew = malloc(sizeof(*unew) * (GRIDSIZE+2));
  rho = malloc(sizeof(*rho) * (GRIDSIZE+2));
~~~
{: .language-c}

It first defines two constants that govern the scale of the simulation:

- `MAX_ITERATIONS`: determines the maximum number of iterative steps the code will attempt in order to find a solution with sufficiently low equilibrium
- `GRIDSIZE`: the number of sections within our stick that will be simulated. Increasing this will increase the number of stick sections to simulate, which increases the processing required

Next, it declares some arrays used during the iterative calculations:

- `u`: each value represents the current temperature of a section in the stick
- `unew`: during an iterative step, is used to hold the newly calculated temperature of a section in the stick
- `rho`: holds a separate coefficient for each section of the stick, used as part of the iterative calculation to represent potentially different boundary conditions for each section of the stick. For simplicity, we'll assume completely homogeneous boundary conditions, so these potentials are zero

Note we are defining each of our array sizes with two additional elements, the first of which represents a touching 'boundary' before the stick, i.e. something with a potentially different temperature touching the stick. The second added element is at the end of the stick, representing a similar boundary at the opposite end.

The next step is to initialise the initial conditions of the simulation:

~~~
  // Set up parameters
  h = 0.1;
  hsq = h*h;
  residual = 1e-5;

  // Initialise the u and rho field to 0
  for (i = 0; i <= GRIDSIZE+1; i++) {
    u[i] = 0.0;
    rho[i] = 0.0;
  }

  // Create a start configuration with the heat energy
  // u=10 at the x=0 boundary for rank 1
  u[0] = 10.0;
~~~
{: .language-c}

`residual` here refers to the threshold of temperature equilibrium along the stick we wish to achieve. Once it's within this threshold, the simulation will end. 
Note that initially, `u` is set entirely to zero, representing a temperature of zero along the length of the stick.
As noted, `rho` is set to zero here for simplicity.

Remember that additional first element of `u`? Here we set it to a temperature of `10.0` to represent something with that temperature touching the stick at one end, to initiate the process of heat transfer we wish to simulate.

Next, the code iteratively calls `poisson_step()` to calculate the next set of results, until either the maximum number of steps is reached, or a particular measure of the difference in temperature along the stick returned from this function (`unorm`) is below a particular threshold.

~~~
  // Run iterations until the field reaches an equilibrium
  // and no longer changes
  for (i = 0; i < NUM_ITERATIONS; i++) {
    unorm = poisson_step(u, unew, rho, hsq, GRIDSIZE);
    if (sqrt(unorm) < sqrt(residual)) break;
  }
~~~
{: .language-c}

Finally, just for show, the code outputs a representation of the result - the end temperature of each section of the stick.

~~~
  printf("Final result:\n");
  for (int j = 1; j <= GRIDSIZE; j++) {
    printf("%d-", (int) u[j]);
  }
  printf("\n");
  printf("Run completed in %d iterations with residue %g\n", i, unorm);
}
~~~
{: .language-c}

### The Iterative Function - poisson_step()

The `poisson_step()` progresses the simulation by a single step. After it accepts its arguments, for each section in the stick it calculates a new value based on the temperatures of its neighbours:

~~~
  for (int i = 1; i <= points; i++) {
     float difference = u[i-1] + u[i+1];
     unew[i] = 0.5 * (difference - hsq*rho[i]);
  }
~~~
{: .language-c}

Next, it calculates a value representing the overall cumulative change in temperature along the stick compared to its previous state, which as we saw before, is used to determine if we've reached a stable equilibrium and may exit the simulation:

~~~
  unorm = 0.0;
  for (int i = 1;i <= points; i++) {
     float diff = unew[i]-u[i];
     unorm += diff*diff;
  }
~~~
{: .language-c}

And finally, the state of the stick is set to the newly calculated values, and `unorm` is returned from the function:

~~~
  // Overwrite u with the new field
  for (int i = 1;i <= points;i++) {
     u[i] = unew[i];
  }

  return unorm;
}
~~~
{: .language-c}

### Compiling and Running the Poisson Code

You may compile and run the code as follows:

~~~
gcc poisson.c -o poisson
./poisson
~~~
{: .language-bash}

And should see the following:

~~~
Final result:
9-8-7-6-6-5-4-3-3-2-1-0-
Run completed in 182 iterations with residue 9.60328e-06
~~~
{: .output}

Here, we can see a basic representation of the temperature of each section of the stick at the end of the simulation, and how the initial `10.0` temperature applied at the beginning of the stick has transferred along it to this final state.
Ordinarily, we might output the full sequence to a file, but we've simplified it for convenience here.


## Approaching Parallelism

So how should we make use of an MPI approach to parallelise this code?
A good place to start is to consider the nature of the data within this computation, and what we need to achieve.

For a number of iterative steps, currently the code computes the next set of values for the entire stick.
So at a high level one approach using MPI would be to split this computation by dividing the stick into sections, and have a separate rank responsible for each section which computes iterations for its given section.
Essentially then, for simplicity we may consider each section a stick on its own, with either two neighbours at touching boundaries (for middle sections of the stick), or one touching boundary neighbour (for sections at the beginning and end of the stick).

We might also consider subdividing the number of iterations, and parallelise across these instead.
However, this is far less compelling since each step is completely dependent on the results of the prior step,
so by its nature steps must be done serially.

FIXME: add diagram of subdivided stick

The next step is to consider in more detail this approach to parallelism with our code.

> ## Does it Make Sense?
>
> Looking at the code, which parts would benefit most from parallelisation,
> and are there any regions that require data exchange across its processes in order for
> the simulation to work as we intend?
>
>> ## Solution
>>
>> Potentially, the following regions could be executed in parallel:
>> 
>> * The setup, when initialising the fields
>> * The calculation of each time step, `unew` - this is the most computationally intensive of the loops
>> * Calculation of the cumulative temperature difference, `unorm`
>> * Overwriting the field `u` with the result of the new calculation
>>
>> As `GRIDSIZE` is increased, these will take proportionally more time to complete, so may benefit from parallelisation.
>>
>> However, there are a few regions in the code that will require exchange of data across the parallel executions
>> to work correctly:
>>
>> * Calculation of `unorm` is a sum that requires difference data from all sections of the stick, so we'd need to somehow communicate these difference values to a single rank that computes and receives the overall sum
>> * Each section of the stick does not compute a single step in isolation, it needs boundary data from neighbouring sections of the stick to arrive at its computed temperature value for that step, so we'd need to communicate temperature values between neighbours
>{: .solution}
{: .challenge}

We also need to identify any sizeable serial regions.
The sum of the serial regions gives the minimum amount of time it will take to run the program.
If the serial parts are a significant part of the algorithm, it may not be possible to write an efficient parallel version.

> ## Serial Regions
>
> Examine the code and try to identify any serial regions that can't be parallelised.
> 
>> ## Solution
>>
>> There aren't any large or time consuming serial regions, which is good from a parallelism perspective.
>> However, there are a couple of small regions that are not amenable to running in parallel:
>> 
>> * Setting the `10.0` initial temperature condition at the stick 'starting' boundary. We only need to set this once at the starting end of the stick, and not at the boundary of every section of the stick.
>> * Printing a representation of the final result, since this only needs to be done once.
>{: .solution}
{: .challenge}


### Write a Parallel Function Thinking About a Single Rank

In the message passing framework, all ranks execute the same code.
When writing a parallel code with MPI, a good place to start is to think about a single rank.
What does this rank need to do, and what information does it need to do it?

When considering communication, a good place to start is to communicate
data in the simplest possible way just after it's created or just before
it's needed. Use blocking or non-blocking communication, whichever you feel
is simpler. If you use non-blocking functions, call wait immediately.

The point is to write a simple code that works correctly.
You can always optimise further later!

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
>>   /* We will calculate unorm separately on each rank. It needs to be summed up at the end */
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

