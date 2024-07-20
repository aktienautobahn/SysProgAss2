#include "../include/ringbuf.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>  // For error handling


void ringbuffer_init(rbctx_t *context, void *buffer_location, size_t buffer_size)
{
    //  clear from begin till end 
    context->begin = buffer_location;
    context->end = buffer_location + buffer_size;
    context->read = buffer_location;
    context->write = buffer_location;


    // Initialize mutexes and condition variables
    pthread_mutex_init(&context->mtx, NULL);
    pthread_cond_init(&context->sig, NULL);
}

size_t available_space(rbctx_t *context) {
       return (context->write > context->read) ?
                             (context->write - context->read) :
                             ((context->end-context->begin) - (context->read - context->write)); // wraparound
}


int msg_size_copy(rbctx_t *context, size_t message_len) {
    // helfer function for copying size_t_arr
    
    size_t SIZE = sizeof(size_t);

    char *size_t_arr = (char *)&message_len; // Cast the address of message_len to a char pointer for byte-wise access

    for (size_t i = 0; i < SIZE; i++) {
        if (context->write == context->end) { // Check if the write pointer is at the end of the buffer
            context->write = context->begin; // Wrap around to the beginning
        }
        memcpy(context->write, size_t_arr + i, 1); // Copy one byte at a time
        context->write++; // Increment the write pointer
    }
    if (context->write == context->end) { // Check if the write pointer is at the end of the buffer
            context->write = context->begin; // Wrap around to the beginning
        }
    return 0; // successful process
}

int msg_size_read(rbctx_t *context, size_t *message_len) {
    // Validate pointers for safety.
    if (!context || !message_len || !context->read || !context->begin || !context->end) 
    {
        return EINVAL;  
    }
    // Validate position pointers for safety
    if (context->read > context->end || context->read < context->begin) 
    {
        return EINVAL;
    }    

    size_t SIZE = sizeof(size_t); // 8 byte

    size_t buffer_len_local = context->end - context->begin;

    size_t prefix = SIZE;

    size_t bytes_copied = 0;
    while (prefix > 0) 
    {
        if (context->read >= context->end) {
            context->read = context->begin; // Wrap around if at the end
        }
        // Safe copying with boundary checks
        if ((char*)message_len + bytes_copied < (char*)message_len + sizeof(size_t)) {
            memcpy((char*)message_len + bytes_copied, context->read, 1); // Copy one byte at a time
            bytes_copied++;
        } else {
            // buffer overflow attempt
            break;
        }

        context->read++;
        if (context->read >= context->end) {
            context->read = context->begin; // Additional safety check for read increment
        }
        prefix--;
    }
   
    if (*message_len > buffer_len_local)  // ring buffer length
    {
        return OUTPUT_BUFFER_TOO_SMALL;
    };

    return 0; // successful process
}


size_t static is_buffer_full(rbctx_t *context, size_t msg_len) {
    u_int8_t *read = context->read;
    size_t needed_space = msg_len + sizeof(size_t);
    size_t ringbuffer_size = context->end - context->begin;
    size_t free_bytes = 0;

    if (context->write >= context->read) {
        // When write is ahead of read, or at the same position
        free_bytes = ringbuffer_size - (context->write - read) - 1;
    } else {
        // When read is ahead of write
        free_bytes = read - context->write - 1;
    }
    if (needed_space > free_bytes) return 1;
    else return 0;
}

/* 
• read pointer > write pointer: Der Lesepointer befindet sich hinter dem Schreibpoin- ter.
• read pointer == write pointer: Der Lesepointer ist gleich dem Schreibpointer.
• read pointer < write pointer: Der Lesepointer befindet sich vor dem Schreibpointer.
*/
int is_buffer_empty(rbctx_t *context) {
    // "freezing" the state of the pointers
    uint8_t* local_read = context->read;
    uint8_t* local_write = context->write;
    uint8_t* begin = context->begin;
    uint8_t* end = context->end;
    // 1 = empty
    if (local_read == local_write) return 1; // buffer is empty
    // case if: context->read < context->write (check if still 8 bytes available?)
    else if ( local_write > local_read) return (local_write - local_read) < 8; //(check if still 8 bytes available?)
    else {
        size_t size_wrap = ((end - local_read) + (local_write - begin));
        return size_wrap < 8; // wraparound case
    } 

}

int ringbuffer_write(rbctx_t *context, void *message, size_t message_len)
{
    pthread_mutex_lock(&context->mtx);
    
    // setting timeout
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1; // Wait for up to 1 second

    while (is_buffer_full(context, message_len)) { // buffer is still full
        int res = pthread_cond_timedwait(&context->sig, &context->mtx, &ts);
 
        if (res == ETIMEDOUT) {
        // Handle timeout scenario

            pthread_mutex_unlock(&context->mtx);  // Unlock before returning
            return RINGBUFFER_FULL;
        }

    }
    void *message_start = message; // remembers the start of message
    // iterate till the message length is 0
    msg_size_copy(context, message_len);
    
    
    while (message_len > 0 && context->write != context->read) {
     
        if (context->write == context->end)  // check if the pointer inside the array -> else go to begin
        {
            context->write = context->begin;
        }
        memcpy(context->write, message, 1);
        context->write++;
        message++;
        message_len--;
    }

    if (context->write == context->end)  // check if the pointer inside the array -> else go to begin
    {
        context->write = context->begin;
    }
    
    message = message_start; // puts the message pointer to it's initial state
    pthread_cond_signal(&context->sig); // signal to reader
    pthread_mutex_unlock(&context->mtx);
    return SUCCESS;

}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len)
{
    
    pthread_mutex_lock(&context->mtx);

    if (!buffer || !buffer_len || *buffer_len == 0) { // safety check for buffer len
        // Handle error
        printf("Invalid buffer or context\n");
        pthread_mutex_unlock(&context->mtx);

        return OUTPUT_BUFFER_TOO_SMALL;
    }


    // -------------------- EMPTY BUFFER HANDLER -------------------- //
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // set timer start time
    ts.tv_sec += 1; // set timer end time
    while (is_buffer_empty(context)) // empty buffer condition
    {  
 
        if (pthread_cond_timedwait(&context->sig, &context->mtx, &ts) == ETIMEDOUT) 
        {
            pthread_mutex_unlock(&context->mtx);  // Unlock mutex before returning
            return RINGBUFFER_EMPTY; 
        }
    }

    // -------------------- DEFINE MESSAGE SIZE -------------------- //


    size_t msg_len = 0;
    if (msg_size_read(context, &msg_len) != 0) {

        pthread_mutex_unlock(&context->mtx);
        return OUTPUT_BUFFER_TOO_SMALL; // Define appropriate error handling
    }

    // -------------------- ENSURE BUFFER LEN IS NOT SMALLER THAN MESSAGE SIZE -------------------- //

    if (*buffer_len < msg_len) {
        
        pthread_mutex_unlock(&context->mtx);  // Unlock mutex before returning
        *buffer_len = 0;
        return OUTPUT_BUFFER_TOO_SMALL; 
    }

    // -------------------- COPY MESSAGE LEN BYTE FOR BYTE INTO BUFFER -------------------- //
    // Use a separate pointer for traversing the buffer
    char *buffer_ptr = buffer;

    size_t bytes_read = 0;


    while (msg_len > 0 && context->read != context->write) {
        if (context->read >= context->end) {
            context->read = context->begin; // Wrap around
        }

        *buffer_ptr = *context->read; // Copy data to the buffer
        context->read++;
        buffer_ptr++;
        msg_len--;
        bytes_read++; // 33
    }
    
    if (context->read >= context->end) {
        context->read = context->begin; // Wrap around
    }

    *buffer_len = bytes_read;
    pthread_cond_signal(&context->sig); // signal to writer
    pthread_mutex_unlock(&context->mtx);

    return SUCCESS;
    
}

void ringbuffer_destroy(rbctx_t *context)
{
    /* your solution here */
    context->begin = NULL;
    context->end = NULL;    
    context->read = NULL;    
    context->write = NULL;
    // destroy mutex and signal
    pthread_mutex_destroy(&context->mtx);
    pthread_cond_destroy(&context->sig);
}
