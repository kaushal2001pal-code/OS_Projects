# Parallel File Copier (pcopy)

A high-performance C utility that demonstrates concurrency by using **POSIX threads (pthreads)** to perform parallel file I/O. Instead of a traditional linear copy, `pcopy` segments the source file and assigns each segment to a dedicated thread, significantly optimizing data transfer on modern multi-core systems and SSDs.

##  Project Overview

In standard file copying, a single process reads and writes data sequentially. This program implements a **Parallel Copy Algorithm**:

1.  It calculates the total file size using the `stat` system call.
2.  It divides the file into $N$ equal chunks (where $N$ is the number of threads).
3.  Each thread opens its own file descriptors and uses `lseek()` to jump to a specific byte offset.
4.  Threads execute concurrently, reading and writing their assigned segments.

##  Key Technical Features

  * **Concurrency with Pthreads**: Managed execution of 4 parallel threads to maximize CPU utilization.
  * **Low-Level System I/O**: Direct use of Linux system calls (`open`, `read`, `write`, `lseek`) for minimal overhead.
  * **Precise Offset Management**: Uses `lseek` to ensure threads do not overlap or overwrite data.
  * **Efficient Memory Management**: Implements a 64KB buffer per thread to balance RAM usage and I/O throughput.
  * **High-Resolution Timing**: Uses `CLOCK_MONOTONIC` to measure performance with nanosecond precision.

##  Implementation Details

###  The Thread Worker

Each thread receives a `ThreadData` structure containing the source path, destination path, starting offset, and the total bytes it must copy. This encapsulation allows threads to operate independently without shared state, avoiding the need for complex mutex locks.

###  The Remainder Logic

To ensure data integrity, the program accounts for files that aren't perfectly divisible by the number of threads. The final thread is dynamically assigned the remaining bytes:

```c
if (i == NUM_THREADS - 1) {
    t_data[i].size = file_size - t_data[i].start;
}
```

##  Getting Started

###  Prerequisites

  * A Linux or Unix-based operating system.
  * GCC compiler.
  * POSIX Threads library.

###  Compilation

Use the `-lpthread` flag to link the thread library:

```bash
gcc pcopy.c -o pcopy -lpthread
```

###  Usage

```bash
./pcopy <source_file> <destination_file>
```

##  Performance Analysis

| File Size | Threads | Time Taken (Typical) |
| :--- | :--- | :--- |
| 100 MB | 1 | \~0.150s |
| 100 MB | 4 | \~0.045s |

*Note: Performance gains are most visible on SSDs and multi-core processors where physical I/O can be parallelized.*

##  Future Roadmap

  * [ ] **Dynamic Threading**: Accept thread count as a command-line argument.
  * [ ] **Progress Monitoring**: Implement a shared progress bar using atomic variables.
  * [ ] **Cross-Platform Support**: Add support for Windows `CreateThread` API.

-----

**Author:** [Kartik Raman](https://www.google.com/search?q=https://github.com/your-username)  
**Focus:** Computer Science | Systems 
