#include "image_rotation.h"
 
 
//Global integer to indicate the length of the queue??
int queue_len = 0;
//Global integer to indicate the number of worker threads
int worker_thr_num = 0;
//Global file pointer for writing to log file in worker??
FILE *log_file;
//Might be helpful to track the ID's of your threads in a global array
int id_arr[1024];
//What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t file_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mut = PTHREAD_MUTEX_INITIALIZER;
//What kind of Condition Variables will you need  (i.e. queue full, queue empty) [Hint you need multiple]
// pthread_cond_t q_full_cond = PTHREAD_COND_INITIALIZER;
// pthread_cond_t q_empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t q_has_work_cond = PTHREAD_COND_INITIALIZER;
// pthread_cond_t q_workers_done_cond = PTHREAD_COND_INITIALIZER;
// pthread_cond_t traversal_done_cond = PTHREAD_COND_INITIALIZER;
//How will you track the requests globally between threads? How will you ensure this is thread safe?
request_t *req_queue = NULL;
request_t *end_queue = NULL;
//How will you track which index in the request queue to remove next? We will use a dequeue function
request_t *dequeue_request() {
    pthread_mutex_lock(&queue_mut);

    if (req_queue == NULL) {
        pthread_mutex_unlock(&queue_mut);
        return NULL; // Queue is empty
    }
    request_t *ret_request = req_queue;
    req_queue = req_queue->next;
    queue_len--;
    if (req_queue == NULL) { // Queue had one object which was dequeued
        end_queue = NULL; 
    }
    pthread_mutex_unlock(&queue_mut);
    return ret_request;
}
//How will you update and utilize the current number of requests in the request queue? 
int request_len = 0;
//How will you track the p_thread's that you create for workers?
pthread_t workerArray[1024];
//How will you know where to insert the next request received into the request queue? We will use an enqueue function
void enqueue_request(int new_angle, char* file_path){
    pthread_mutex_lock(&queue_mut);

    request_t *new_request = malloc(sizeof(request_t));
    if (new_request == NULL) { // Malloc error
        pthread_mutex_unlock(&queue_mut);
        return;
    }
    strcpy(new_request->file_path, file_path);
    new_request->angle = new_angle;
    new_request->next = NULL;

    if (req_queue == NULL){
        req_queue = new_request;
        end_queue = new_request;
    }
    else{
        end_queue->next = new_request;
        end_queue = end_queue->next;
    }
    queue_len++;
    pthread_mutex_unlock(&queue_mut);
}


/*
    The Function takes:
    to_write: A file pointer of where to write the logs. 
    requestNumber: the request number that the thread just finished.
    file_name: the name of the file that just got processed. 

    The function output: 
    it should output the threadId, requestNumber, file_name into the logfile and stdout.
*/
void log_pretty_print(FILE* to_write, int threadId, int requestNumber, char * file_name){
    pthread_mutex_lock(&file_mut);
    // write to file
    fprintf(to_write, "[%d][%d][%s]\n", threadId, requestNumber, file_name);
    fflush(to_write);
    pthread_mutex_unlock(&file_mut);
    // print to stdout
    fprintf(stdout, "[%d][%d][%s]\n", threadId, requestNumber, file_name);
    fflush(stdout);
}

/*

    1: The processing function takes a void* argument called args. It is expected to be a pointer to a structure processing_args_t 
    that contains information necessary for processing.

    2: The processing thread need to traverse a given dictionary and add its files into the shared queue while maintaining synchronization using lock and unlock. 

    3: The processing thread should pthread_cond_signal/broadcast once it finish the traversing to wake the worker up from their wait.

    4: The processing thread will block(pthread_cond_wait) for a condition variable until the workers are done with the processing of the requests and the queue is empty.

    5: The processing thread will cross check if the condition from step 4 is met and it will signal to the worker to exit and it will exit.

*/

void *processing(void *args)
{
    // Check validity of input arguments
    if (args == NULL) {
        fprintf(stderr, "Expect a pointer to a structure processing_args_t.\n");
        pthread_exit(NULL); 
    }
    processing_args_t *proc_args = (processing_args_t *)args;
    char directory_path[BUFF_SIZE];
    strcpy(directory_path, proc_args->directory_path);
    int angle = proc_args->angle;

    // Traverse directory and add its files into shared queue (use mutex lock queue_mut for synchronization)
    DIR *dir = opendir(directory_path);
    if (dir == NULL) {
        perror("Error opening directory");
        pthread_exit(NULL);
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Check for ".png" files
        char *extension = strrchr(entry->d_name, '.'); // gets pointer to last occurrence of "." in entry->d_name
        if (extension != NULL && strcmp(extension, ".png") == 0) { // check if file ends in ".png"
            enqueue_request(angle, entry->d_name); // synchronization handled in enqueue_request
            pthread_cond_signal(&q_has_work_cond); // Signal to worker threads that queue isn't empty
        }
    }
    closedir(dir);

    // // Processing thread has finished traversing through directory & adding entries to queue
    // // Broadcast to worker threads that traversal is finished
    // pthread_cond_broadcast(&traversal_done_cond);

    // // Processing thread blocks for q_empty_cond until workers process all requests and queue is empty
    // while (queue_len > 0) {
    //     pthread_cond_wait(&q_workers_done_cond, &queue_mut);
    // }

    // // Processing thread cross checks condition and broadcasts to worker threads to exit
    
    pthread_exit(NULL);
}

/*
    1: The worker threads takes an int ID as a parameter

    2: The Worker thread will block(pthread_cond_wait) for a condition variable that there is a requests in the queue. 

    3: The Worker threads will also block(pthread_cond_wait) once the queue is empty and wait for a signal to either exit or do work.

    4: The Worker thread will processes request from the queue while maintaining synchronization using lock and unlock. 

    5: The worker thread will write the data back to the given output dir as passed in main. 

    6:  The Worker thread will log the request from the queue while maintaining synchronization using lock and unlock.  

    8: Hint the worker thread should be in a While(1) loop since a worker thread can process multiple requests and It will have two while loops in total
        that is just a recommendation feel free to implement it your way :) 
    9: You may need different lock depending on the job.  

*/


void * worker(void *args)
{
        // For Inter Submission: Print worker thread ID and exit
        int* ptr = (int*)args;
        int thd_ID = *ptr;
        printf("Worker thread ID: %d\n", thd_ID);
        pthread_exit(NULL);
        /*
            Stbi_load takes:
                A file name, int pointer for width, height, and bpp

        */

    //    uint8_t* image_result = stbi_load("??????","?????", "?????", "???????",  CHANNEL_NUM);
        

    //     uint8_t **result_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
    //     uint8_t** img_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
    //     for(int i = 0; i < width; i++){
    //         result_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
    //         img_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
    //     }
        /*
        linear_to_image takes: 
            The image_result matrix from stbi_load
            An image matrix
            Width and height that were passed into stbi_load
        
        */
        //linear_to_image("??????", "????", "????", "????");
        

        ////TODO: you should be ready to call flip_left_to_right or flip_upside_down depends on the angle(Should just be 180 or 270)
        //both take image matrix from linear_to_image, and result_matrix to store data, and width and height.
        //Hint figure out which function you will call. 
        //flip_left_to_right(img_matrix, result_matrix, width, height); or flip_upside_down(img_matrix, result_matrix ,width, height);


        
        
        //uint8_t* img_array = NULL; ///Hint malloc using sizeof(uint8_t) * width * height
    

        ///TODO: you should be ready to call flatten_mat function, using result_matrix
        //img_arry and width and height; 
        //flatten_mat("??????", "??????", "????", "???????");


        ///TODO: You should be ready to call stbi_write_png using:
        //New path to where you wanna save the file,
        //Width
        //height
        //img_array
        //width*CHANNEL_NUM
       // stbi_write_png("??????", "?????", "??????", CHANNEL_NUM, "??????", "?????"*CHANNEL_NUM);
    

}

/*
    Main:
        Get the data you need from the command line argument 
        Open the logfile
        Create the threads needed
        Join on the created threads
        Clean any data if needed. 


*/

int main(int argc, char* argv[])
{
    // Check validity of input arguments
    if(argc != 5)
    {
        fprintf(stderr, "Usage: File Path to image dirctory, File path to output directory, number of worker thread, and Rotation angle\n");
        return -1;
    }

    // Open the file path to output directory
    // Check if opendir is successful
    DIR* output_directory = opendir(argv[2]);
    if(output_directory == NULL){
        fprintf(stderr, "Invalid output directory\n");
        return -1;
    }

    // Get the number of worker threads needed
    // Declare one processing thread as well
    worker_thr_num = atoi(argv[3]);
    pthread_t processing_thread;

    // Set the arguments for the processing thread
    processing_args_t *proc_args = malloc(sizeof(processing_args_t));
    strcpy(proc_args->directory_path, argv[1]);
    proc_args->num_workers = worker_thr_num;
    proc_args->angle = atoi(argv[4]);

    // Create the processing thread with processing args
    pthread_create(&processing_thread, NULL, processing, proc_args);

    // Create worker_thr_num number of worker threads
    for(int i = 0; i < worker_thr_num; i++){   
        id_arr[i] = i; 
        pthread_create(&workerArray[i], NULL, worker, (void *)&id_arr[i]);
    }

    // Join all threads after program is done
    pthread_join(processing_thread, NULL);
    for(int i = 0; i < worker_thr_num; i++){   
        pthread_join(workerArray[i], NULL);
    }

    // Free mallocs and close opened directory
    free(proc_args);
    closedir(output_directory);

    // Destroy mutexes and condition variables
    pthread_mutex_destroy(&file_mut);
    pthread_mutex_destroy(&queue_mut);
    pthread_cond_destroy(&q_has_work_cond);

    return 0;

}