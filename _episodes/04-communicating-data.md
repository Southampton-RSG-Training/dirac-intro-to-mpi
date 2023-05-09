---
title: Communicating Data in MPI
slug: dirac-intro-to-mpi-communicating-data
teaching: 15
exercises: 0
questions:
- How do I exchange data between MPI ranks?
objectives:
- Understand how data is exchanged between MPI ranks
keypoints:
- Data is sent between ranks using "messages"
- Messages can either block the program or be sent/received asynchronously
- Knowing the exact amount of data you are sending is required
---

In previous episodes we've seen that when we run an MPI application, multiple *independent* processes are created which
do their own work, on their own data, in their own private memory space. At some point in our program, one rank will
probably need to know about the data another rank has, such as when combining a problem back together which was split
across ranks. Since each rank's data is private to itself, we can't just access another rank's memory and get what we
need from there. We have to instead explicitly *communicate* data between ranks. Sending and receiving data
between ranks form some of the most basic building blocks in any MPI application, and the success of your
parallelisation often relies on how you communicate data.

## Communicating data using messages

MPI is a standardised framework for passing data and other messages between independently running processes. If we want
to share or access data from one rank to another, we use the MPI API to transfer data in a "message." To put it simply,
a message is merely a data structure which contains the data, and is usually expressed as a collection of data elements
of a particular data type.

Sending and receiving data happens in one of two ways. We either want to send data from one specific rank to another,
known as point-to-point communication, or to/from multiple ranks all at once to a single target, known as collective
communication. In both cases, we have to *explicitly* "send" something and to *explicitly* "receive" something. We've
emphasised *explicitly* here to make clear that data communication can't happen by itself. That is a rank can't just
"pluck" data from one rank, because a rank doesn't automatically send and receive the data it needs. If we don't program
in data communication, data can't be shared. None of this communication happens for free, either. With every message
sent, there is an associated overhead which impacts the performance of your program. Often we won't notice this
overhead, as it is quite small. But if we communicate large amounts data or too often, those small overheads can rapidly
add up into a noticeable performance hit.

To get an idea of how communication typically happens, imagine we have two ranks: rank A and rank B. If rank A wants to
send data to rank B (e.g., point-to-point), it must first call the appropriate MPI send function which puts that data
into an internal *buffer*; sometimes known as the send buffer or envelope. Once the data is in the buffer, MPI figures
out how to route the message to rank B (usually over a network) and sends it to B. To receive the data, rank B must call
a data receiving function which will listen for any messages being sent. When the message has been successfully routed
and the data transfer complete, rank B sends an acknowledgement back to rank A to say that the transfer has finished,
similarly to how read receipts work in e-mails and instant messages.

### Communication modes

There are multiple "modes" on how data is sent in MPI: standard, buffered, synchronous and ready. When an MPI
communication function is called, control/execution of the program is passed from the calling program to the MPI
function. Your program won't continue until MPI is happy that the communication happened successfully. The difference
between the communication modes is the criteria of a successful communication.

To use the different modes, we don't pass a special flag. Instead, MPI uses different functions to separate the
different modes. The table below lists the four modes with a description and their associated functions (which will be
covered in detail in the following episodes).

| Mode | Description | MPI Function |
| - | - | - |
| Synchronous | Returns control to the program when the message has been sent and received successfully | `MPI_Ssend` |
| Buffered | Control is returned when the data has been copied into in the send buffer, regardless of the receive being completed or not | `MPI_Bsend` |
| Standard  | Either buffered or synchronous, depending on the size of the data being sent and what your specific MPI implementation chooses to use | `MPI_Send` |
| Ready | Will only return control if the receiving rank is already listening for a message | `MPI_Rsend` |

In contrast to the four modes for sending data, receiving data only has one mode and therefore only a single function.

| Mode |  Description | MPI Function |
| - | - | - |
| Receive | Returns control when data has been received successfully | `MPI_Recv` |

### Blocking vs. non-blocking communication

Communication can also be done in two additional ways: blocking and non-blocking. In blocking mode, communication
functions will only return once the send buffer is ready to be re-used, meaning that the message has been both sent and
received. In terms of a blocking synchronous send, control will not be passed back to the program until the message sent
by rank A has reached rank B, and rank B has sent an acknowledgement back. If rank B is never listening for messages,
rank A will become *deadlocked*. A deadlock happens when your program hangs indefinitely because the send (or receive)
is unable to complete. Deadlocks occur for a countless number of reasons. For example, we may forget to write the
corresponding receive function when sending data. Alternatively, a function may return earlier due to an error which
isn't handled properly, or a while condition may never be met creating an infinite loop. Furthermore, ranks can
sometimes crash silently making communication with them impossible, but this doesn't stop any attempts to send data to
crashed rank.

> ## Avoiding communication deadlocks
>
> A common piece of advice in C is that when allocating memory using `malloc`, always write the accompanying call to
> `free` to help avoid memory leaks by forgetting to deallocate the memory later. You can apply the same mantra to
> communication in MPI. When you send data, always write the code to receive the data as you may forget to later and
> accidentally cause a deadlock.
{: .callout}

Blocking communication works best when the work is balanced across ranks, so that each rank has an equal amount of
things to do. A common pattern in scientific pattern is to split a calculation across a grid and then to share the
results between all ranks before moving onto the next calculation. If the workload is well balanced, each rank will
finish at roughly the same time and be ready to transfer data at the same time. But, as shown in the diagram below, if
the workload is unbalanced, some ranks will finish their calculations earlier and begin to send their data to the other
ranks before they are ready to receive data. This means some ranks will be sitting around doing nothing whilst they wait
for the other ranks to become ready to receive data, wasting computation time.

<img src="fig/blocking-wait.png" alt="Blocking communication" height="250"/>

If most of the ranks are waiting around, or one rank is very heavily loaded in comparison, this could massively impact
the performance of your program. Instead of doing calculations, a rank will be waiting for other ranks to complete their
work.

Non-blocking communication hands back control, immediately, before the communication has finished. Instead of your
program being *blocked* by communication, ranks will immediately go back to the heavy work and instead periodically
check if there is data to receive (which you must remember to program) instead of waiting around. The advantage of this
communication pattern is illustrated in the diagram below, where less time is spent communicating.

<img src="fig/non-blocking-wait.png" alt="Non-blocking communication" height="250"/>

This is a common pattern where communication and calculations are interwoven with one another, decreasing the amount of
"dead time" where ranks are waiting for other ranks to communicate data. Unfortunately, non-blocking communication is
often more difficult to successfully implement and isn't appropriate for every algorithm. In most cases, blocking
communication is usually easier to implement and to conceptually understand, and is somewhat "safer" in the sense that
the program cannot continue if data is missing. However, the potential performance improvements of overlapping
communication and calculation is often worth the more difficult implementation and harder to read/more complex code.

> ## Should I use blocking or non-blocking communication?
>
> When you are first implementing communication into your program, it's advisable to first use blocking synchronous
> sends to start with, as this is arguably the easiest to use pattern. Once you are happy that the correct data is being
> communicated successfully, but you are unhappy with performance, then it would be time to start experimenting with the
> different communication modes and blocking vs. non-blocking patterns to balance performance with ease of use and code
> readability and maintainability.
>
{: .callout}

## Communicators

Communication in MPI happens in something known as a *communicator*. We can think of a communicator as fundamentally
being a collection of ranks which are able to exchange data with one another. What this means is that every
communication between two (or more) ranks is linked to a specific communicator in the program. When we run an MPI
application, the ranks will belong to the default communicator known as `MPI_COMM_WORLD`. We've seen this in earlier
episodes when, for example, we've used functions like `MPI_Comm_rank` to get the rank number,

```c
int my_rank;
MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);  /* MPI_COMM_WORLD is the communicator the rank belongs to */
```

In addition to `MPI_COMM_WORLD`, we can make sub-communicators and distribute ranks into them. Messages can only be
sent and received to and from the same communicator, effectively isolating messages to a communicator. For most
applications, we usually don't need anything other than `MPI_COMM_WORLD`. But organising ranks into communicators can be
helpful in some circumstances, as you can create small "work units" of multiple ranks to dynamically schedule the
workload, or to help compartmentalise the problem into smaller chunks by using a [virtual cartesian
topology](https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report/node192.htm#Node192). Throughout this lesson, we will
stick to using `MPI_COMM_WORLD`.

<!-- Within communicators there are groups, which are a collection of ranks within a communicator which you can somewhat
think of as being "sub-communicators" in the communicator.

> ## Creating communicators
>
> Creating a new communicator can be done in multiple ways. You could can, for example, create new communicators by
> splitting one into multiple or you could create a completely new communicator. By using the [documentation for
> `MPI_Comm_create`](https://www.open-mpi.org/doc/v4.1/man3/MPI_Comm_create.3.php), create a new communicator which
> contains only the first two ranks.
>
> > ## Solution
> >
> > ```c
> > MPI_Comm_create();
> >```
> >
> {: .solution}
>
{: .challenge} -->

## Basic MPI data types

To send a message, we need to know the size of it. The size is not the number of bytes of data that is being sent but is
instead expressed as the number of elements of a particular data type you want to send. So when we send a message, we
have to tell MPI how many elements of "something" we are sending and what type of data it is. If we don't do this
correctly, we'll either end up telling MPI to send only *some* of the data or try to send more data than we want! For
example, if we were sending an array and we specify too few elements, then only a subset of the array will be sent or
received. But if we specify too many elements, than we are likely to end up with either a segmentation fault or some
undefined behaviour! If we don't specify the correct data type, then bad things will happen under the hood when it
comes to communicating.

There are two types of data type in MPI: basic MPI data types and derived data types. The basic data types are in
essence the same data types we would use in C (or Fortran), such as `int`, `float`, `bool` and so on. When defining the
data type of the elements being sent, we don't use the primitive C types. MPI instead uses a set of compile-time
constants which internally represents the data types. These data types are in the table below:

| MPI basic data type | C equivalent |
| - | - |
| MPI_SHORT | short int |
| MPI_INT | int |
| MPI_LONG | long int |
| MPI_LONG_LONG | long long int |
| MPI_UNSIGNED_CHAR | unsigned char |
| MPI_UNSIGNED_SHORT |unsigned short int |
| MPI_UNSIGNED | unsigned int |
| MPI_UNSIGNED_LONG | unsigned long int |
| MPI_UNSIGNED_LONG_LONG | unsigned long long int |
| MPI_FLOAT | float |
| MPI_DOUBLE | double |
| MPI_LONG_DOUBLE | long double |
| MPI_BYTE | char |

These constants don't expand out to actual date types, so we can't use them in variable declarations, e.g.,

```c
MPI_INT my_int;
```

is not valid code because under the hood, these constants are actually special structs used internally. Therefore we can
only uses these expressions as arguments in MPI functions.

> ## Don't forget to update your types
>
> At some point during development, you might change an `int` to a `long` or a `float` to a `double`, or something to
> something else. Once you've gone through your codebase and updated the types for, e.g., variable declarations and
> function signatures, you must also do the same for MPI functions. If you don't, you'll end up running into
> communication errors. It may be helpful to define compile-time constants for the data types and use those instead. If
> you ever do need to change the type, you would only have to do it in one place.
>
> ```c
> /* define constants for your data types */
> #define MPI_INT_TYPE MPI_INT
> #define INT_TYPE int
> /* use them as you would normally */
> INT_TYPE my_int = 1;
> ```
>
{: .callout}

Derived data types are data structures which you define, built using the basic MPI data types. These derived types are
analogous to defining structures or type definitions in C. They're most often helpful to group together similar data to
send/receive multiple things in a single communication, or when you need to communicate non-contiguous data such as
"vectors" or sub-sets of an array. This will be covered in the [Advanced Communication
Techniques](dirac-intro-to-mpi-advanced-communication) episode.
