---
title: "Introduction to Parallelism"
slug: "dirac-intro-to-mpi-distributed-memory"
teaching: 15
exercises: 0
questions:
- What is parallelisation?
- 
objectives:
- Understand the fundamentals of parallelisation and parallel programming
- Understand the shared and distributed memory 
- Understand the advantages and disadvantages of distributed memory parallelisation
keypoints:
- Processes do not share memory and can reside on the same or different computers.
- Threads share memory and reside in a process on the same computer. 
- MPI is an example of multiprocess programming.
- OpenMP is an example of multithreaded programming.

---
<p>&nbsp;</p>
Parallel programming has been important to scientific computing for decades as a way to decrease program run times, making more complex analyses possible 
(e.g. climate modeling, gene sequencing, pharmaceutical development, aircraft design). During this course you will learn to design parallel algorithms and write parallel programs using the 
<b>MPI</b> library. MPI stands for <b>Message Passing Interface</b>, and is a low level, minimal and extremely flexible set of commands for communicating between copies of a program. 
Before we dive into the details of MPI, let's first familiarize ourselves with key concepts that lay the groundwork for parallel programming. 

## What is Parallelisation?

At some point in your career, you’ve probably asked the question “How can I make my code run faster?”. Of course, the answer to this question will depend sensitively 
on your specific situation, but here are a few approaches you might try doing:

- Optimize the code.
- Move computationally demanding parts of the code from an interpreted language (Python, Ruby, etc.) to a compiled language (C/C++, Fortran, Julia, Rust, etc.).
- Use better theoretical methods that require less computation for the same accuracy.

Each of the above approaches is intended to reduce the total amount of work required by the computer to run your code. A different strategy for speeding up codes is 
parallelisation, in which you split the computational work among multiple processing units that labor simultaneously. The “_processing units_” might 
include central processing units (**CPU**s), graphics processing units (**GPU**s), vector processing units (**VPU**s), or something similar. 

Typical programming assumes that computers execute one operation at a time in the sequence specified by your program code. At any time step, the computer’s CPU 
core will be working on one particular operation from the sequence. In other words, a problem is broken into discrete series of instructions that are executed one for another. 
Therefore only one instruction can execute at any moment in time. We will call this traditional style of sequential computing.

In contrast, with parallel computing we will now be dealing with multiple CPU cores that each are independently and simultaneously working on a series of instructions. 
This can allow us to do much more at once, and therefore get results more quickly than if only running an equivalent sequential program. The act of changing sequential code to 
parallel code is called parallelisation.
<p>&nbsp;</p>
<table>
    <tr>
    <td style='text-align:center; font-size:1.1em;'>
    <b> Sequential Computing </b>
        <img src="fig/serial2_prog.png" alt='Serial Computing'
             style='zoom:40%;'/> 
    </td>
    <td>
    <p style='text-align: center; margin-right: 3em; font-size:1.1em;'><b> Parallel Computing </b></p> 
        <img src="fig/parallel_prog.png" alt='Parallel Computing' 
             style='zoom:40%;'/>
    </td>
    </tr>
</table>

> ## Analogy
>
> The basic concept of parallel computing is simple to understand: we divide our job in tasks that can be executed at the same 
> time so that we finish the job in a fraction of the time that it would have taken if the tasks are executed one by one.
>
> Suppose that we want to paint the four walls in a room. This is our problem. We can divide our problem in 4 different tasks: 
> paint each of the walls. In principle, our 4 tasks are independent from each other in the sense that we don't need to finish one
> to start another. However, this does not mean that the tasks can be executed simultaneously or in parallel. It all depends on 
> on the amount of resources that we have for the tasks.
> 
> If there is only one painter, they could work for a while in one wall, then start painting another one, then work a little bit  
> on the third one, and so on. The tasks are being executed concurrently **but not in parallel** and only one task is being 
> performed at a time. If we have 2 or more painters for the job, then the tasks can be performed in **parallel**.
>
> ## Key idea
>
> In our analogy, the painters represent CPU cores in the computers. The number of CPU cores available determines the maximum 
> number of tasks that can be performed in parallel. The number of concurrent tasks that can be started at the same time, however 
> is unlimited.
{: .prereq}

## Parallel Programming and Memory: Processes, Threads and Memory Models

Splitting the problem into computational tasks across different processors and running them all at once may conceptually seem like a straightforward solution to achieve the desired 
speed-up in problem-solving. However, in practice, parallel programming involves more than just task division and introduces various complexities and considerations.

Let's consider a scenario where you have a single CPU core, associated RAM (primary memory for faster data access), hard disk (secondary memory for slower data access), 
input devices (keyboard, mouse), and output devices (screen).

Now, imagine having two or more CPU cores. Suddenly, you have several new factors to take into account:

  1. If there are two cores, there are two possibilities: either these cores share the same RAM (shared memory) or each core has its own dedicated RAM (private memory).
  2. In the case of shared memory, what happens when two cores try to write to the same location simultaneously? This can lead to a race condition, which requires careful handling by the programmer to avoid conflicts.
  3. How do we divide and distribute the computational tasks among these cores? Ensuring a balanced workload distribution is essential for optimal performance.
  4. Communication between cores becomes a crucial consideration. How will the cores exchange data and synchronize their operations? Effective communication mechanisms must be established.
  5. After completing the tasks, where should the final results be stored? Should they reside in the storage of Core 1, Core 2, or a central storage accessible to both? Additionally, which core 
is responsible for displaying output on the screen?

These considerations highlight the interplay between parallel programming and memory. To efficiently utilize multiple CPU cores, we need to understand the concepts of processes and threads, 
as well as different memory models—shared memory and distributed memory. These concepts form the foundation of parallel computing and play a crucial role in achieving optimal parallel execution.

To address the challenges that arise when parallelising programs across multiple cores and achieve efficient use of available resources, parallel programming frameworks like MPI and OpenMP 
(Open Multi-Processing) come into play. These frameworks provide tools, libraries, and methodologies to handle memory management, workload distribution, communication, and synchronization in parallel environments.

Now, let's take a brief look at these fundamental concepts and explore the differences between MPI and OpenMP, setting the stage for a deeper understanding of MPI in the upcoming episodes

> ## Processes
> A process refers to an individual running instance of a software program. Each process operates independently and possesses its own set of 
> resources, such as memory space and open files. As a result, data within one process remains isolated and cannot be directly accessed by other processes. 
>
> In parallel programming, the objective is to achieve parallel execution by simultaneously running coordinated processes. This naturally introduces the need for communication and data 
> sharing among them. To facilitate this, parallel programming models like MPI come into effect. MPI provides a comprehensive set of libraries, tools, and methodologies 
> that enable processes to exchange messages, coordinate actions, and share data,  enabling parallel execution across a cluster or network of machines.
> <img src="fig/multiprocess.svg" height="250" alt='Processes'/>
>
> ## Threads 
> A thread is an execution unit that is part of a process. It operates within the context of a process and shares the process's resources, such as memory space and 
> open files. Unlike processes, multiple threads within a process can access and share the same data, enabling more efficient and faster parallel programming.
>
> Threads are lightweight and can be managed independently by a scheduler. They are units of execution in concurrent programming, allowing multiple threads to execute at 
> the same time, making use of available CPU cores for parallel processing. Threads can improve application performance by utilizing parallelism and allowing tasks to be 
> executed concurrently.
>
> One advantage of using threads is that they can be easier to work with compared to processes when it comes to parallel programming. When incorporating threads, 
> especially with frameworks like OpenMP, modifying a program becomes simpler. This ease of use stems from the fact that threads operate within the same process and can 
> directly access shared data, eliminating the need for complex inter-process communication mechanisms required by MPI. However, it's important to note that threads within 
> a process are limited to a single computer. While they provide an effective means of utilizing multiple CPU cores on a single machine, they cannot extend beyond the boundaries of that computer.
<img src="fig/multithreading.svg" height="250" alt='Threads'/>
{: .callout}

> ## Analogy
> Let's go back to our painting 4 walls analogy. Our example painters have two arms, and could potentially paint with both arms at the same time. 
> Technically, the work being done by each arm is the work of a single 
> painter. In this example, each painter would be a ***“process”*** (an 
> individual instance of a program). The painters’ arms represent a ***“thread”*** of a program. Threads are separate points of execution within 
> a single program, and can be executed either synchronously or 
> asynchronously.
{: .prereq}

> ## Shared vs Distributed Memory
> Shared memory refers to a memory model where multiple processors can directly access and modify 
> the same memory space. Changes made by one processor are immediately visible to all other 
> processors. Shared memory programming models, like OpenMP, simplify parallel programming by 
> providing mechanisms for sharing and synchronizing data.
>
> Distributed memory, on the other hand, involves memory resources that are physically separated 
> across different computers or nodes in a network. Each processor has its own private memory, and 
> explicit communication is required to exchange data between processors. Distributed memory 
> programming models, such as MPI, facilitate communication and synchronization in this memory model.
> <img src="fig/memory-pattern.png" height="280" style='zoom:40%;' alt='Shared Memory and Distributed Memory'/>
>
> <p style="text-align: center; font-size:18px"> <b>Differences/Advantages/Disadvantages of Shared and Distributed Memory</b> </p>
> {: .checklist}
> - **Accessibility:** Shared memory allows direct access to the same memory space by all processors, 
> while distributed memory requires explicit communication for data exchange between processors.
> - **Memory Scope:** Shared memory provides a global memory space, enabling easy data sharing and 
> synchronization. In distributed memory, each processor has its own private memory space, requiring 
> explicit communication for data sharing.
> - **Memory Consistency:** Shared memory ensures immediate visibility of changes made by one 
> processor to all other processors. Distributed memory requires explicit communication and 
> synchronization to maintain data consistency across processors.
> - **Scalability:** Shared memory systems are typically limited to a single computer or node, 
> whereas distributed memory systems can scale to larger configurations with multiple computers and 
> nodes.
> - **Programming Complexity:** Shared memory programming models offer simpler constructs and 
> require 
> less explicit communication compared to distributed memory models. Distributed memory programming 
> involves explicit data communication and synchronization, adding complexity to the programming 
> process.
{: .callout}

> ## Analogy
> Imagine that all workers have to obtain their paint form a central dispenser located at the middle of the room. If each worker 
> is using a different colour, then they can work asynchronously. However, if they use the same colour, and two of them run out of 
> paint at the same time, then they have to synchronise to use the dispenser — one should wait while the other is being serviced.
>
> Now let's assume that we have 4 paint dispensers, one for each worker. In this scenario, each worker can complete their task 
> totally on their own. They don’t even have to be in the same room, they could be painting walls of different rooms in the house, in 
> different houses in the city, and different cities in the country. We need, however, a communication system in place. Suppose 
> that worker A, for some reason, needs a colour that is only available in the dispenser of worker B, they must then synchronise: 
> worker A must request the paint of worker B and worker B must respond by sending the required colour.
>
> ## Key Idea
> In our analogy, the paint dispenser represents access to the memory in your computer. Depending on how a program is written,
> access to data in memory can be synchronous or asynchronous. For the different dispensers case for your workers, however,
> think of the memory distributed on each node/computer of a cluster.
{: .prereq}
