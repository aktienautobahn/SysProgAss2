#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "../include/daemon.h"
#include <pthread.h>
#include "../include/ringbuf.h"

/* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU 
 * changing the code will result in points deduction */

/********************************************************************
* NETWORK TRAFFIC SIMULATION: 
* This section simulates incoming messages from various ports using 
* files. Think of these input files as data sent by clients over the
* network to our computer. The data isn't transmitted in a single 
* large file but arrives in multiple small packets. This concept
* is discussed in more detail in the advanced module: 
* Rechnernetze und Verteilte Systeme
*
* To simulate this parallel packet-based data transmission, we use multiple 
* threads. Each thread reads small segments of the files and writes these 
* smaller packets into the ring buffer. Between each packet, the
* thread sleeps for a random time between 1 and 100 us. This sleep
* simulates that data packets take varying amounts of time to arrive.
*********************************************************************/
typedef struct {
    rbctx_t* ctx;
    connection_t* connection;
} w_thread_args_t;

void* write_packets(void* arg) {
    /* extract arguments */
    rbctx_t* ctx = ((w_thread_args_t*) arg)->ctx;
    size_t from = (size_t) ((w_thread_args_t*) arg)->connection->from;
    size_t to = (size_t) ((w_thread_args_t*) arg)->connection->to;
    char* filename = ((w_thread_args_t*) arg)->connection->filename;

    /* open file */
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", filename);
        exit(1);
    }

    /* read file in chunks and write to ringbuffer with random delay */
    unsigned char buf[MESSAGE_SIZE];
    size_t packet_id = 0;
    size_t read = 1;
    while (read > 0) {
        size_t msg_size = MESSAGE_SIZE - 3 * sizeof(size_t);
        read = fread(buf + 3 * sizeof(size_t), 1, msg_size, fp);
        if (read > 0) {
            memcpy(buf, &from, sizeof(size_t));
            memcpy(buf + sizeof(size_t), &to, sizeof(size_t));
            memcpy(buf + 2 * sizeof(size_t), &packet_id, sizeof(size_t));
            while(ringbuffer_write(ctx, buf, read + 3 * sizeof(size_t)) != SUCCESS){
                usleep(((rand() % 50) + 25)); // sleep for a random time between 25 and 75 us
            }
        }
        packet_id++;
        usleep(((rand() % (100 -1)) + 1)); // sleep for a random time between 1 and 100 us
    }
    fclose(fp);
    return NULL;
}

/* END OF PROVIDED CODE */


/********************************************************************/

/* YOUR CODE STARTS HERE */

    // defining nessesary structs
    typedef struct {
        size_t from_port;
        size_t to_port;
    } connection_r;
    

    typedef struct {
        rbctx_t* ctx;
        connection_r* conn;
    }r_thread_args_t;

    typedef struct {
        // size_t from_port;
        size_t last_packet_id;
        pthread_mutex_t mutex;
        pthread_cond_t signal;
    } port_info_t;

    // declaration of nessecary arrays for holding mutexes and variables
    port_info_t port_array[MAXIMUM_PORT+1];
    pthread_mutex_t file_mutex[MAXIMUM_PORT+1];

    // Initialization of file mutexes
    void initialize_file_mutexes() {
        for (int i = MINIMUM_PORT; i < MAXIMUM_PORT+1; i++) {
            pthread_mutex_init(&file_mutex[i], NULL);
        }
    }
    // Initialization of port mutexes as well as last packet if variables
    void initialize_port_array() {
        for (int i = MINIMUM_PORT; i < MAXIMUM_PORT+1; i++) {
            port_array[i].last_packet_id = -1;
            pthread_mutex_init(&port_array[i].mutex, NULL);
            pthread_cond_init(&port_array[i].signal, NULL);
        }
    } 



// firewall functionality
/**
 * @brief Checks if the string contains the characters of "malicious" in sequence with any characters between them.
 * 
 * This function iterates through the provided string to find the pattern "malicious" where the characters 'm', 'a', 'l', 'i', 'c', 'i', 'o', 'u', 's' appear in sequence,
 * potentially separated by any number of other characters. It returns 1 if the pattern is found, otherwise 0.
 *
 * @param unfiltered_string The string to be checked for the "malicious" pattern.
 * @return int Returns 1 if the "malicious" pattern is found, otherwise returns 0.
 */
int malicious_filter(unsigned char* unfiltered_string, size_t contents_len) {
    const char* malstring = "malicious";
    size_t malstring_len = strlen(malstring);
    size_t malcounter = 0;
    // size_t unfiltered_index = 0;

    for (size_t i = 0; i < contents_len + 1; i++)
    {
        if (unfiltered_string[i] == malstring[malcounter]) {
            malcounter++; // increment malstring counter
            if (malcounter == malstring_len) {
                // printf("Malicious found\n");
                return 1; // all characters found in sequence
            }
        }
    }
    // while(unfiltered_string[unfiltered_index] != '\0') { // traverse the string
    //     if (unfiltered_string[unfiltered_index] == malstring[malcounter]) {
    //         malcounter++; // increment malstring counter
    //         if (malcounter == malstring_len) {
    //             // printf("Malicious found\n");
    //             return 1; // all characters found in sequence
    //         }
    //     }
    //     unfiltered_index++; // increment index of unfiltered string
    // }
    return 0; // no malicious pattern found
}

/**
 * @brief Evaluates connection port conditions to determine if a specific criteria is met.
 *
 * This function checks if the source port and destination port of a connection meet any of the following conditions:
 * 1. The source port is the same as the destination port.
 * 2. The source port or the destination port is 42.
 * 3. The sum of the source and destination ports equals 42.
 * If any of these conditions are met, the function returns 1, otherwise 0.
 *
 * @param conn A connection_t structure that contains the source port (`from`) and destination port (`to`).
 * @return int Returns 1 if any of the specified conditions are met, otherwise returns 0.
 */
int port_filter(int from, int to) {
    if (from == to || from == 42 || to == 42 || from + to == 42) { // TODO add them back  
        return 1;
    }
    return 0; // if neither of conditions is true
}

/**
 * @brief Evaluates whether a packet should be blocked based on specified filters.
 * 
 * This function determines if a packet should be blocked by evaluating it against two filters:
 * 1. The `port_filter` which checks for certain conditions related to the source and destination ports.
 * 2. The `malicious_filter` which checks the contents of the connection for a specific malicious pattern.
 * 
 * The connection is blocked (returns 1) if either of the filters returns 1, indicating a match. Otherwise, it is not blocked (returns 0).
 *
 * @param conn A pointer to a `connection_r` structure containing the connection's source and destination ports.
 * @param contents A pointer to an unsigned char array containing the packet data transmitted over the connection.
 * @return int Returns 1 if the packet should be blocked based on the filter criteria, otherwise returns 0.
 */
int firewall(connection_r *conn, unsigned char* contents, size_t contents_len) {
    if (!port_filter(conn->from_port, conn->to_port) && !malicious_filter(contents, contents_len)) {
        return 0;
    }
    return 1; // connection should be 
}

/**
 * @brief Simulation of port forwarding using file writing. Writes data to a file in a thread-safe manner using mutex locks.
 * 
 * This function appends the provided buffer to a file named after the destination port of the connection.
 * The writing process is made thread-safe by locking a mutex specific to the destination port (actually filename) before attempting to write.
 * If the file does not exist, it is created. If the file cannot be opened, an error is logged and the function returns -1.
 * After writing, the file is closed and the mutex is unlocked.
 *
 * @param conn A pointer to a `connection_r` structure containing the destination port used to name the file.
 * @param buf Pointer to the buffer containing data to be written to the file.
 * @param buffer_len The length of the buffer, i.e., the number of bytes to write.
 * @return int Returns the number of bytes written to the file. If the file cannot be opened, returns -1.
 */
int forwarding(connection_r *conn, void *buf, size_t buffer_len) {
    char filename[21];

    memset(filename, 0, sizeof(filename)); // reset the array
    snprintf(filename, sizeof(filename), "%zu.txt", conn->to_port);
    pthread_mutex_lock(&file_mutex[conn->to_port]);

    /* open file with mode 'a' (append) */
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        pthread_mutex_unlock(&file_mutex[conn->to_port]);

        fprintf(stderr, "Cannot open file with name %s\n", filename);
        return -1;
    }
    int write = fwrite(buf, 1, buffer_len, fp);
    fclose(fp);
    pthread_mutex_unlock(&file_mutex[conn->to_port]);
    return write;
}

void* read_packets(void* arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    /* extract arguments */
    r_thread_args_t *thread_args = (r_thread_args_t *)arg;
    rbctx_t* ctx = thread_args->ctx;
    connection_r* conn = thread_args->conn; 

    /* read ringbuffer in chunks and write to file  ? with / or without random delay */
    unsigned char buf[MESSAGE_SIZE + 3 * sizeof(size_t)];
    size_t buffer_len = sizeof(buf);
    size_t packet_id = 0;
    unsigned char contents[buffer_len];
    memset(&contents, 0, buffer_len); // clearing contents
    do {
        while(ringbuffer_read(ctx, &buf, &buffer_len) != SUCCESS){
            buffer_len = MESSAGE_SIZE + 3 * sizeof(size_t);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            usleep(10); // sleep for 10 us
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        }

        if (buffer_len >  3 * sizeof(size_t)) {
            memcpy(&conn->from_port, buf, sizeof(size_t));
            memcpy(&conn->to_port, buf + sizeof(size_t) , sizeof(size_t));
            memcpy(&packet_id, buf + 2 * sizeof(size_t), sizeof(size_t));
            memcpy(contents, buf + 3 * sizeof(size_t), buffer_len - 3 * sizeof(size_t)); 

            // safety check
            if (conn->from_port > MAXIMUM_PORT || conn->to_port > MAXIMUM_PORT ||
                conn->from_port < MINIMUM_PORT || conn->to_port < MINIMUM_PORT) {
                fprintf(stderr, "Port numbers %zu and/or %zu are too large\n", conn->from_port, conn->to_port);
                exit(1);
            }

            pthread_mutex_lock(&port_array[conn->from_port].mutex);

            while (packet_id != port_array[conn->from_port].last_packet_id + 1) {
                // printf("packet id is on hold: %zu\n", packet_id);
                pthread_cond_wait(&port_array[conn->from_port].signal, &port_array[conn->from_port].mutex);
            }
            // update packet id
            port_array[conn->from_port].last_packet_id = packet_id;

            // firewall: filter on port and "malicious" and decide if drop the message or not, if not , write to the file 
            if (firewall(conn, contents, buffer_len - 3 * sizeof(size_t)) == 0) {

                // Debug: Log the packet information
                char debug_filename[32];
                snprintf(debug_filename, sizeof(debug_filename), "%zu_debug.txt", conn->to_port);
                FILE *debug_fp = fopen(debug_filename, "a");
                if (debug_fp != NULL) {
                    fprintf(debug_fp, "%zu %zu %zu %s\n", conn->from_port, conn->to_port, packet_id, contents);
                    fclose(debug_fp);
                } else {
                    fprintf(stderr, "Cannot open debug file with name %s\n", debug_filename);
                }
                
                size_t write = forwarding(conn, // meta information: ports
                                        &contents,  // buffer (contents)
                                        (buffer_len - 3 * sizeof(size_t)) // buffer length
                                        );
                if (write != buffer_len - 3 * sizeof(size_t)) {
                    // an error occured
                    fprintf(stderr, "Error with forwarding\n");
                }
            } else {
              // Debug: Log the packet information
                char debug_filename[32];
                snprintf(debug_filename, sizeof(debug_filename), "%zu_debug.txt", conn->to_port);
                FILE *debug_fp = fopen(debug_filename, "a");
                if (debug_fp != NULL) {
                    fprintf(debug_fp, "%zu %zu %zu MALICIOUS in %s\n", conn->from_port, conn->to_port, packet_id, contents);
                    fclose(debug_fp);
                } else {
                    fprintf(stderr, "Cannot open debug file with name %s\n", debug_filename);
                }
            }
        

        // unlock packet id mutex
        pthread_mutex_unlock(&port_array[conn->from_port].mutex);

        // broadcast signal
        pthread_cond_broadcast(&port_array[conn->from_port].signal);
        }      
        buffer_len = MESSAGE_SIZE + 1; 
        memset(&buf, 0, buffer_len); // clearing buffer
        memset(&contents, 0, buffer_len - 3 * sizeof(size_t));


    } while(1);

    return NULL;
}


/* YOUR CODE ENDS HERE */

/********************************************************************/

int simpledaemon(connection_t* connections, int nr_of_connections) {
    /* initialize ringbuffer */
    rbctx_t rb_ctx;
    size_t rbuf_size = 1024;
    void *rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        fprintf(stderr, "Error allocation ringbuffer\n");
    }

    ringbuffer_init(&rb_ctx, rbuf, rbuf_size);

    /****************************************************************
    * WRITER THREADS 
    * ***************************************************************/

    /* prepare writer thread arguments */
    w_thread_args_t w_thread_args[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        w_thread_args[i].ctx = &rb_ctx;
        w_thread_args[i].connection = &connections[i];
        /* guarantee that port numbers range from MINIMUM_PORT (0) - MAXIMUMPORT */
        if (connections[i].from > MAXIMUM_PORT || connections[i].to > MAXIMUM_PORT ||
            connections[i].from < MINIMUM_PORT || connections[i].to < MINIMUM_PORT) {
            fprintf(stderr, "Port numbers %d and/or %d are too large\n", connections[i].from, connections[i].to);
            exit(1);
        }
    }

    /* start writer threads */
    pthread_t w_threads[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        pthread_create(&w_threads[i], NULL, write_packets, &w_thread_args[i]);
    }

    /****************************************************************
    * READER THREADS
    * ***************************************************************/

    pthread_t r_threads[NUMBER_OF_PROCESSING_THREADS];

    /* END OF PROVIDED CODE */
    
    /********************************************************************/

    /* YOUR CODE STARTS HERE */
    connection_r conn[NUMBER_OF_PROCESSING_THREADS];

    initialize_port_array(); // initializing last packet id for a port array

    // 1. think about what arguments you need to pass to the processing threads
    r_thread_args_t r_thread_args[NUMBER_OF_PROCESSING_THREADS];
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        r_thread_args[i].ctx = &rb_ctx;
        r_thread_args[i].conn = &conn[i];
    }
    // 2. start the processing threads
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_create(&r_threads[i], NULL, read_packets, &r_thread_args[i]);
    }
    /* YOUR CODE ENDS HERE */

    /********************************************************************/



    /* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU 
     * changing the code will result in points deduction */

    /****************************************************************
     * CLEANUP
     * ***************************************************************/

    /* after 5 seconds JOIN all threads (we should definitely have received all messages by then) */
    printf("daemon: waiting for 5 seconds before canceling reading threads\n");
    sleep(5);
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_cancel(r_threads[i]);
    }

    /* wait for all threads to finish */
    for (int i = 0; i < nr_of_connections; i++) {
        pthread_join(w_threads[i], NULL);
    }

    /* join all threads */
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_join(r_threads[i], NULL);
    }

    /* END OF PROVIDED CODE */



    /********************************************************************/
    
    /* YOUR CODE STARTS HERE */

    // use this section to free any memory, destory mutexe etc.

    pthread_mutex_destroy(&rb_ctx.mtx);
    pthread_cond_destroy(&rb_ctx.sig);

    for (int i = 0; i < MAXIMUM_PORT+1; i++) {
        pthread_mutex_destroy(&file_mutex[i]);
    }
    for (int i = 0; i < MAXIMUM_PORT+1; i++) {
        pthread_mutex_init(&port_array[i].mutex, NULL);
        pthread_cond_init(&port_array[i].signal, NULL);
    }


    /* YOUR CODE ENDS HERE */

    /********************************************************************/



    /* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU 
    * changing the code will result in points deduction */

    free(rbuf);
    ringbuffer_destroy(&rb_ctx);

    return 0;

    /* END OF PROVIDED CODE */
}