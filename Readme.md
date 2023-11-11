**Project group number:** PA 27

**Group member names and x500s:**
1. Stephen Ma, ma000094
2. Robert Wang, wan00379
3. Robert Tan, tan00222

**The name of the CSELabs computer that you tested your code on:**
csel-kh1250-21.cselabs.umn.edu

**Members’ individual contributions Planning:**  
Stephen Ma:
* Check main
* Write dequeue and enqueue request functions
* Work on processing
* Work on worker
* Write README

Robert Wang:
* Write main
* Check dequeue and enqueue functions
* Work on worker

Robert Tan:
* Complete structs in image_rotation.h
* Check processing
* Work on worker

**How you designed your program for Parallel image processing (pseudocode)**  
In main, we will spawn one processing thread and n # of worker threads. We will do this by using one pthread_create for the processing thread and a for loop with pthread_create inside for the worker threads.
Next, in processing, it will traverse through a directory and for every .png image file it finds, it will add an entry to the global request queue. For our queue, we have created two functions: dequeue_request and enqueue_request. For both of these functions, because we are accessing the global request queue, we need a mutex lock. We added pthread_mutex_lock(&queue_mut); at the beginning and pthread_mutex_unlock(&queue_mut); at the end of each function. 
When the processing thread finishes traversing the directory, we need to broadcast to all worker threads that it has finished traversal. We do this by using pthread_cond_broadcast (condition variable will be used here). After, it will enter a while loop that calls pthread_cond_wait while the length of the queue is greater than 0. Here, another condition variable is needed for wait. 
When it exits from the while loop, it will call pthread_cond_broadcast to let the workers know they should exit. 
For the worker threads, each one will call pthread_cond_wait to wait for there to be an entry in the request queue. There will be two condition variables to let it know if it should either exit or if there is work to be done.
When the worker accesses the request queue to grab an entry, we need a mutex lock here to ensure that no other threads are making changes to the queue at the same time. 
Each worker thread will also call log_pretty_print. Inside this function, we need a mutex lock since we are printing information to one output file. This ensures that the file isn't being accessed and changed at the same time by another thread. 
At the end of main, it will join the processing thread and all worker threads. We free our mallocs, close our opened directory, and destroy the initialized mutex locks and condition variables.
