---
title: Communicating Data in MPI
slug: dirac-intro-to-mpi-communicating-data
teaching: 15
exercises: 10
questions:
- How do I exchange data between MPI ranks?
objectives:
- Understand how data is exchanged between MPI ranks
keypoints:
- Data is sent between ranks using "messages"
- Messages can either block the program or be sent/received asynchronously
- Knowing the exact amount of data you are sending is required
---

In the previous episodes we have seen that when we run an MPI application, multiple *independent* processes are created
which do their own work, on their own data, in their own private memory space. But at some point in our program, one
rank will want or need to know about the data another rank has. Since each rank's data is private to itself, we can't
just snoop at another rank's memory and get what we need from there. Instead, we have to explicitly *communicate* the
data between the ranks. Sending and receiving data between ranks make up the most basic building blocks in any MPI
application, and the success of your parallelisation often relies on how you communicate data.

## Communicating data using messages

As we know by now, MPI is a standardised framework for passing data and other messages between independently running
processes. If we want to share or access data from one rank to another, we have to use the MPI API to send that data in
a "message." A message contains a number of elements of some particular data type. For almost all scientific code, the
actual technical details of what a message is under-the-hood doesn't matter.

Sending and receiving data usually follows a set of standard communication patterns. We either want to send data
from one specific rank to another, known as point-to-point communication, or to/from multiple ranks all at once,
known as collective communication. In both cases, we need to *explicitly* "send" something and to *explicitly* "receive"
something. The reason for emphasis on *explicitly* is because data communication does not magically happen by itself in
the background. If we do not program data communication, data won't be shared. It should also be noted that none of this
communication happens for free, and there is an associated overhead which may negatively impact performance if you are
working with a large amount of data or are continuously communicating small amounts of data.

No matter which communication pattern you use, point-to-point or collective, the same process happens to communicate the
data. Imagine we have two ranks, rank A and rank B. If rank A wants to send data to rank B (point-to-point), it must
first call the appropriate MPI send function, which puts that data into an internal *buffer*; sometimes known as the
send buffer. Once the data is packed into the buffer, MPI then figures out how to route the message to rank B (usually
over some network) and sends the buffer to B. At the same time, rank B must call the data receiving function which
listens for messages being sent to it. When the message has been routed and sent, rank B now acknowledges that the
message has been received successfully, similar to how read receipts work in instant messaging.

### Communication modes

There are actually multiple "modes" on how data is sent in MPI: standard, buffered, synchronous and ready. When an MPI
communication function is called, control/execution of the program is passed from the calling program to the MPI
function. The main difference between the communication modes when when control is passed back from the MPI function to
the calling program.

| Mode | Description | MPI Function |
| - | - | - |
| Synchronous | Returns control to the program when the message has been sent and received successfully | `MPI_Ssend` |
| Buffered | Control is returned when the data has been copied into in the send buffer, regardless of the receive being completed or not | `MPI_Bsend` |
| Standard  | Either buffered or synchronous, depending on the size of the data being sent and what your specific MPI implementation chooses to use | `MPI_Send` |
| Ready | Will only return control if the receiving rank is already listening for a message | `MPI_Rsend` |

As the table suggests, we use different functions to use of the different communication modes, rather than specifying
the mode as an argument, and all offer differing levels of safety of when you can make changes to the data being
communicated. These functions will be covered in more detail in later episodes. In contrast to the four modes for
sending data, receiving data only has one mode.

| Mode |  Description | MPI Function |
| - | - | - |
| Receive | Returns control when data has been received successfully | `MPI_Recv` |

### Blocking vs. non-blocking communication

The communication modes described in the last section all exist in two additional modes: blocking and non-blocking. In
blocking mode, communication functions will only return when the send buffer is ready to be re-used. In terms of a
synchronous send, this means control will not be returned from the sending function until the receiving rank has
acknowledged that the data has been received. This can sometimes cause your program to go into something known as a
deadlock, where it hasn't crashed but execution is halted because an MPI function is unable to return. The blocking
nature of this communication means you can't do anything until the data has sent successfully. In the worst-case
scenarios where you have lot of data to pass around or are doing lots of blocking communications, the communication
overhead may be larger than the time spent doing calculations!

Conversely, non-blocking communication will hand control back before the communication has finished and the data has
been sent. So, instead of waiting around, you program can immediately get back to doing calculations and periodically
check that the data bas been successfully sent. The implementation of blocking communication is somewhat easier, but the
overlap of communication and computation in non-blocking communication patterns generally leads to lower communication
overheads and improved performance for your program. We will discuss this type of communication in greater detail in a
later episode.

## Communicators

All communication in MPI happens in something known as a *communicator*. We can think of a communicator as most
fundamentally being a collection of ranks which are able to exchange data with each other. Every communication is linked
to a communicator. When we run an MPI application, the ranks will belong to the default communicator known as
`MPI_COMM_WORLD`, as we've already seen in the earlier episodes when we've used something like `MPI_Comm_rank` to get
the rank number, e.g.,

```c
int my_rank;
MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);  /* MPI_COMM_WORLD is the communicator the rank belongs to */
```

For most applications, we never need to use anything other than `MPI_COMM_WORLD`. But it can sometimes be useful to
split ranks into different communicators, although with one *caveat*: messages can only be received from the same
communicator it was sent from. Splitting ranks into different communicators can be helpful depending on how calculations
are done in parallel and their data communicated. It's also helpful if you are writing a parallel framework, as you
can put your communication into a different communicator which users won't be able to access. Throughout this this
lesson, we'll stick to using only `MPI_COMM_WORLD`.

SOMETHING ABOUT GROUPS.

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
{: .challenge}

## Basic MPI data types

When we send a messages, the length of that message is expressed as the number of elements of some data type rather than
some amount of bytes of memory. That means when we send a message, we have to tell MPI how many elements of something we
are sending, which we do by stating how many elements of stuff we are sending and the data type of those elements.
If we are sending an array of values and we don't do this correctly, we'll either end up only sending *some* of the data
or more data than there is in the array! Neither of these cases are good, and the latter often results in undefined
behaviour or even a segmentation fault.

When it comes to it, there are two types of data type we need to worry about in MP: basic MPI data types and derived
data types. The basic data types are the same data types we would use in C (or Fortran), such as `int`, `float`, `bool`
and so on. The equivalent of these data types in the MPI API are essentially the same, following a standard naming
scheme. For example, an `int` is `MPI_INT` and a double is `MPI_DOUBLE`. Basic MPI types are not exactly the same as the
data types in C, e.g.,

```c
MPI_INT my_int;
```

is not valid C code. Under the hood, the MPI data types are actually compile-time constants which are used in the MPI
API internally. The following table shows the basic MPI data types and their C equivalent (Fortran types are available
in [the MPICH documentation](https://www.mpich.org/static/docs/v3.3/www3/Constants.html)),

| MPI basic datatype | C equivalent |
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

> ## Don't forget to update your types
>
> At some point in the development of your program, you might change an `int` to a `long` or a `float` to a
> `double`, or something to something else. Once you've going through your code and updated the types for, e.g.,
> variable declerations and function signatures, you must also do the same for MPI data types. If you don't, you'll end
> up running into communication errors. It may be beneficial to define compile-time constants for the data types and use
> those instead, which means you would only have to modify types in one place,
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
sort of analogous to defining structures or type definitions in C. They're often helpful to use when you want to group
together similar data together and send in a single send/receive operation, or when you need to communicate
non-contiguous data such as "vectors" or sub-arrays of an array. Derived data types will be covered in far more detail
in a later episode.
