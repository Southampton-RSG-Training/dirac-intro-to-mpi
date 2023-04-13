---
title: Communicating Data in MPI
slug: dirac-intro-to-mpi-communicating-data
teaching: 0
exercises: 0
questions:
- How do I exchange data between MPI ranks?
objectives:
- Understand how data is passed between MPI ranks
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

## Exchanging data in MPI

As we know by now, MPI is a standardised framework for passing data and other messages between independently running
processes. If we want to share or access data from one rank to another, we have to use the MPI API to send that data in
a "message." A message contains a number of elements of some particular data type. For almost all programmers, the
technical details of what a message is don't really matter. We only need to worry about the mechanics of exchanging data
between ranks.

When we exchange data, we usually follow a set of standard communication patterns. We either want to send data
from one specific rank to another, known as point-to-point communication, or to/from multiple processes all at once,
known as collective communication. In both cases, we need to program a "send" operation, to say which data we want to
send, and a "receive" operation to listen for and acknowledge the data has been received, similar to a read receipt for
instant messages or e-mail.

### Communicators

All communications in MPI will take place in something known as a *communicator*. We can thank of a communicator as
fundamentally being a collection of ranks which are able to exchange data with one another. By default, ranks will
belong to the default communicator `MPI_COMM_WORLD`. For simple communication patterns, it's quite normal to keep
everything in `MPI_COMM_WORLD`. It can sometimes be useful to split ranks into different communicators depending
on how calculations are done or how data is going to be exchanged. However, it is important to note that only ranks in
the same communicator can exchange data with one another.

### Blocking vs. non-blocking

When data is communicated, we can either use *blocking* or *non-blocking* functions.

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
API internally. A complete list of available data types is available in
[the MPICH documentation](https://www.mpich.org/static/docs/v3.3/www3/Constants.html).

> ## Data type changes
>
> At some point in the development of your program, you might change an `int` to a `long` or a `float` to a
> `double`, or something to something else. Once you've going through your code and updated the types for, e.g.,
> variable declerations and function signatures, you must also do the same for MPI data types. If you don't, you'll end
> up running into communication errors. It may be beneficial to define compile-time constants for the data types and use
> those instead, which means you would only have to modify types in one place,
>
> ```c
> #define MPI_INT_TYPE MPI_INT
> #define INT_TYPE int
> ```
>
{: .callout}

Derived data types are data structures which you define, built upon the basic MPI data types. These derived types are
sort of analogous to defining structures or type definitions in C. They're helpful to use to group together similar data
together and send in a single send/receive operation, or when you need to communicate non-contiguous data such as
sending "vectors" in an array. Derived data types will be covered in more detail in a later episode.
