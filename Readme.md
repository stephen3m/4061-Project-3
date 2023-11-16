**Project group number:** PA 27

**Group member names and x500s:**
1. Stephen Ma, ma000094
2. Robert Wang, wan00379
3. Robert Tan, tan00222

**The name of the CSELabs computer that you tested your code on:**
csel-kh1250-21.cselabs.umn.edu

**Members’ individual contributions:**  
Stephen Ma:
* Check main
* Write dequeue and enqueue request functions
* Work on processing
* Work on worker
* Write README

Robert Wang:
* Write main
* Check dequeue and helped write enqueue functions
* Work on worker
* Cleaned up code

Robert Tan:
* Complete structs in image_rotation.h
* Check processing
* Work on worker

**How you designed your program for Parallel image processing**  
In main, we will spawn one processing thread and n # of worker threads. We will do this by using one pthread_create for the processing thread and a for loop with pthread_create inside for the worker threads.
Next, in processing, it will traverse through the input directory, and for every .png image file it finds, it will add an entry to the global request queue. For our queue, we used a linked list implementation and we have created two functions: dequeue_request and enqueue_request. For both of these functions, because we are accessing the global request queue, we need a mutex lock. To ensure that the request queue is only being modified by one thread, we use pthread_mutex_lock and pthread_mutex_unlock.
When the processing thread finishes traversing the directory, we need to broadcast to all worker threads that it has finished the traversal. We do this by setting the global variable done_traversing to 1 and then using pthread_cond_broadcast to broadcast to all the worker threads that the processing thread has finished traversal. After, we have a while loop that continues to loop until all the workers are finished processing the request queue.
When it exits from the while loop, the processing thread will check that the number of image files passed into the queue is equal to the total number of images processed by the workers. Then the processing thread will broadcast to the worker threads using a semaphore that they can exit.
For the worker threads, we have an outer while(1) loop where we have an inner while loop where we account for the condition where the queue is empty, but the processing thread still needs to add files to the queue. This is done by having the worker thread wait for a signal from the processing thread indicating work to be done. If there is work to do, the worker thread will process the image from the queue to rotate it and put it in the output folder. The worker thread also calls log_pretty_print to print to the console. Once the queue is empty and the global variable done_traversing is 1, this indicates that the processing thread is done working and there is no more work left to do, so the worker thread can exit.

At the end of main, it will join the processing thread and all worker threads. We free our mallocs, close our opened directory and file, and destroy the initialized mutex locks, semaphores, and condition variables.
