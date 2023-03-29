---
title: Communicating Data in MPI
slug: dirac-intro-to-mpi-communicating-data
teaching: 0
exercises: 0
questions:
- How do I share data between MPI ranks?
objectives:
- Understand how data is passed between MPI ranks
keypoints:
- Data is sent between ranks using "messages"
- Messages can either block the program or be sent/received asynchronously
- Knowing the exact amount of data you are sending is required
---

From previous episodes, we know that when we run an MPI applications, multiple *independent* processes are created which
each do their own work, working on their own data, in their own private memory space. At some point in our program, one
MPI rank is going to want to know what data another MPI rank has. Since each rank's data is private to itself, we can't
just "look" at another rank's memory space and get it from there. We have to, instead, explicitly *communicate* data
between ranks.

## Messages in MPI

Data is sent between ranks in "messages." A message

### Blocking vs. non-blocking

## Basic MPI data types

For a message to be sent and received successfully, we need to tell the various MPI communication functions in MPI what
type of data we are sending and how much of it we are sending. If we are sending an array of values and we don't do
this correctly, we'll either end up only sending *some* of the data or more data than there is in the array! Neither
of these cases are good, and the latter may result in undefined behaviour or even a segmentation fault.
