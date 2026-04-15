#include <stdio.h>
#include <stdlib.h>



/*The library for POSIX threads. 
It allows the program to create and manage multiple execution paths.*/
#include <pthread.h> 



/*fcntl.h & unistd.h: 
Low-level system calls for file handling (open, read, write, lseek, close).*/
#include <fcntl.h>
#include <unistd.h>



/*
This header defines a special "container" (a struct) called stat. 
When we call the library, it fills this container with everything the Operating System knows about a file:
    ->File Size: How many bytes the file contains (the most important part for your code).
    ->Permissions: Who can read, write, or execute the file.
    ->Timestamps: When the file was last accessed or modified.
    ->Inode: The file's unique ID on the physical disk.
*/
#include <sys/stat.h>


#include <time.h>

#define NUM_THREADS 1 /*Set to 4, meaning the file will be split into 4 parts*/

/*Set to 64KB. 
Each thread reads/writes in these small bites to avoid overwhelming the system memory (RAM).*/ 
#define BUFFER_SIZE 65536


/*
Threads in C can only take one argument (a void* pointer). 
To pass multiple pieces of information
(source name, destination name, where to start, and how much to copy), 
we bundle them into this struct
*/
typedef struct 
{
    const char *src;   // Path to source file
    const char *dest;  // Path to destination file
    off_t start;       // Byte offset where this thread begins
    off_t size;        // Total bytes this thread is responsible for
} ThreadData;



// The Thread Worker Function
void* copy_chunk(void* arg) 
{
    ThreadData* data = (ThreadData*)arg;
    

    /*
    Independent File Descriptors: Each thread opens its own src_fd and dest_fd. 
    This is vital because file descriptors track the current position (offset). 
    If threads shared one descriptor, they would constantly overwrite each other's progress.
    */
    int src_fd = open(data->src, O_RDONLY);
    // O_WRONLY: write, O_CREAT: create if missing
    int dest_fd = open(data->dest, O_WRONLY | O_CREAT, 0644);

    if (src_fd < 0 || dest_fd < 0) 
    {
        perror("File open error");
        pthread_exit(NULL);
    }

    // Move to the assigned starting position
    /*
    lseek() (The Key to Parallelism): 
    This moves the "read/write head" to the specific start position assigned to that thread.
    for instance : Thread 0 starts at 0,Thread 1 might start at 1,000,000 bytes.
    */
    lseek(src_fd, data->start, SEEK_SET);
    lseek(dest_fd, data->start, SEEK_SET);

    char buffer[BUFFER_SIZE];
    off_t bytes_to_copy = data->size;
    ssize_t bytes_read, bytes_written;

    /*
    The Copy Loop:
        ->  It reads a chunk from the source into a local buffer.
        ->  It writes that buffer into the destination.
        ->  It subtracts the written amount from bytes_to_copy until it reaches zero.
    */

    while (bytes_to_copy > 0) 
    {
        size_t read_len = (bytes_to_copy > BUFFER_SIZE) ? BUFFER_SIZE : bytes_to_copy;
        bytes_read = read(src_fd, buffer, read_len);
        
        if (bytes_read <= 0) break;

        bytes_written = write(dest_fd, buffer, bytes_read);
        bytes_to_copy -= bytes_written;
    }

    close(src_fd);
    close(dest_fd);
    return NULL;
}

int main(int argc, char *argv[]) 
{
    if (argc != 3) 
    {
        printf("Usage: %s <source> <destination>\n", argv[0]);
        return 1;
    }

    /*
    We use stat() to find the total size of the source file. 
    This is necessary to calculate how to divide the work.
    */
    struct stat st;
    if (stat(argv[1], &st) != 0) 
    {
        perror("Stat error");
        return 1;
    }

    /*  
    The program calculates chunk_size = file_size / NUM_THREADS.
    The Remainder Problem: If you have a 10-byte file and 3 threads, 10/3 = 3 (integer division). 
    Threads 1 and 2 take 3 bytes each. 
    The last thread is manually assigned file_size - start to ensure it picks up the final 4th byte.
    */
    off_t file_size = st.st_size;
    pthread_t threads[NUM_THREADS];
    ThreadData t_data[NUM_THREADS];
    off_t chunk_size = file_size / NUM_THREADS;


    struct timespec start, end;
    
    // Start the timer
    clock_gettime(CLOCK_MONOTONIC, &start);

    /*
    The for loop calls pthread_create(). 
    This tells the OS to start a new thread and begin executing copy_chunk using the specific data for that segment.
    */
    for (int i = 0; i < NUM_THREADS; i++) 
    {
        t_data[i].src = argv[1];
        t_data[i].dest = argv[2];
        t_data[i].start = i * chunk_size;
        
        // The last thread handles the remainder
        if (i == NUM_THREADS - 1) 
        {
            t_data[i].size = file_size - t_data[i].start;
        } 
        else 
        {
            t_data[i].size = chunk_size;
        }

        pthread_create(&threads[i], NULL, copy_chunk, &t_data[i]);
    }


    /*
    The main() function cannot just finish; 
    if it did, the entire process would exit while the workers are still copying. 
    pthread_join() makes the main thread wait (pause) until every worker thread has finished its task.
    */
    for (int i = 0; i < NUM_THREADS; i++) 
    {
        pthread_join(threads[i], NULL);
    }

    // Stop the timer
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate total seconds and nanoseconds
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("File copied successfully using %d threads.\n", NUM_THREADS);
    printf("Time taken: %.6f seconds\n", time_taken);
    return 0;
}

/*
Summary:
    ->Main Thread: "The file is 4GB. I'll make 4 threads, each handling 1GB."

    ->Thread 1: "I'm starting at byte 0. Copying until 1GB."

    ->Thread 2: "I'm jumping to byte 1,000,000,001. Copying until 2GB."

    ->Hardware: The CPU assigns these threads to different cores. 
                On an SSD, these writes happen simultaneously.

Main Thread: "Is everyone done? Yes. Printing 'Success' and closing."
*/