#include <stdio.h>
#include <stdlib.h>

#include "../include/ringbuf.h"

int main() {
    rbctx_t *ringbuffer_context = malloc(sizeof(rbctx_t));
    if (ringbuffer_context == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }  

    char msg[] = "This is a test message that is too big for the buffer.";
    size_t msg_len = strlen(msg) + 1; 

    /*************************************************************************
     * TEST 1:                                                               *
     * Write a message that is too big for the buffer.                       *
     *************************************************************************/
    printf("Warning: The first few tests might take longer, depending on the specific use of timedwait\n\n");
    printf("--------------------------------------------------------\n");
    printf("Test 1: Write a message that is too big for the buffer.\n");
    
    /* TEST 1.1: write pointer == read pointer */

    /* TEST 1.1.1: message is way to big for the buffer */
    size_t rbuf_size = msg_len - 1; // definitly too small
    char* rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }
    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != RINGBUFFER_FULL) {
        printf("Error: test 1.1.1 failed\n");
        exit(1);
    }

    free(rbuf);
    printf("  + Test 1.1.1 passed\n");

    /* TEST 1.1.2: message is exactly the size of the buffer (still too big) */
    rbuf_size = msg_len + sizeof(size_t); // exactly the size of the message + prefix
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }
    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != RINGBUFFER_FULL) {
        printf("Error: test 1.1.2 failed\n");
        exit(1);
    }

    free(rbuf);
    printf("  + Test 1.1.2 passed\n");

    /* TEST 1.2: write pointer < read pointer */

    /* TEST 1.2.1: message is way to big for the free space */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += msg_len - 1; // distance between read and write pointer too small

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != RINGBUFFER_FULL) {
        printf("Error: test 1.2.1 failed\n");
        exit(1);
    }

    free(rbuf);
    printf("  + Test 1.2.1 passed\n");

    /* TEST 1.2.2: message is exactly the size of the free space (still too big) */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += msg_len +  sizeof(size_t); // distance between read and write pointer is exactly the size of the message + prefix

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != RINGBUFFER_FULL) {
        printf("Error: test 1.2.2 failed\n");
        exit(1);
    }

    free(rbuf);
    printf("  + Test 1.2.2 passed\n");

    /* TEST 1.3: write pointer > read pointer */

    /* TEST 1.3.1: message is way to big for the free space */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->write += msg_len + sizeof(size_t) + 1; // distance between read and write pointer too small

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != RINGBUFFER_FULL) {
        printf("Error: test 1.3.1 failed\n");
        exit(1);
    }

    free(rbuf);
    printf("  + Test 1.3.1 passed\n");

    /* TEST 1.3.2: message is exactly the size of the free space (still too big) */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->write += msg_len + sizeof(size_t); // distance between read and write pointer is exactly the size of the message + prefix (still too small)

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != RINGBUFFER_FULL) {
        printf("Error: test 1.3.2 failed\n");
        exit(1);
    }

    free(rbuf);
    printf("  + Test 1.3.2 passed\n");

    /*************************************************************************
     * TEST 2:                                                               *
     * Write a message that fits in the buffer.                             *
     *************************************************************************/
    printf("--------------------------------------------------------\n");
    printf("Test 2: Write a message that fits in the buffer.\n");

    /* TEST 2.1: write pointer == read pointer */

    /* TEST 2.1.1: no wrapping */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    
    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.1.1 failed. Incorrect return value\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + msg_len + sizeof(size_t)) {
        printf("Error: Test 2.1.1 failed. Incorrect write pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf || ringbuffer_context->read != ringbuffer_context->begin) {
        printf("Error: Test 2.1.1 failed. Incorrect read pointer position\n");
        exit(1);
    }

    size_t read_len;
    memcpy(&read_len, ringbuffer_context->read, sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 2.1.1 failed. Incorrect message length\n");
        exit(1);
    }

    char read_msg[msg_len];
    memcpy(read_msg, ringbuffer_context->read + sizeof(size_t), msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.1.1 failed. Incorrect message\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.1.1 passed\n");

    /* TEST 2.2: write pointer > read pointer */

    /* TEST 2.2.1: no wrapping */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->write += msg_len; // distance between write and end is big enough, but doesn't lead to wrapping

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.2.1 failed. Incorrect return value\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + msg_len + msg_len + sizeof(size_t)) {
        printf("Error: Test 2.2.1 failed. Incorrect write pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf || ringbuffer_context->read != ringbuffer_context->begin) {
        printf("Error: Test 2.2.1 failed. Incorrect read pointer position\n");
        exit(1);
    }

    memcpy(&read_len, ringbuffer_context->write - msg_len - sizeof(size_t), sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 2.2.1 failed. Incorrect message length\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->write - msg_len, msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.2.1 failed. Incorrect message\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.2.1 passed\n");

    /* TEST 2.2.2: no wraping (extra test) */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += 1; // create enough space between read and write pointer for the message
    ringbuffer_context->write += msg_len + sizeof(size_t); // if we write, we land exactly at the end of the buffer

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.2.2 failed. Incorrect return value\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf) {
        printf("Error: Test 2.2.2 failed. Incorrect write pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + 1) {
        printf("Error: Test 2.2.2 failed. Incorrect read pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->begin != (uint8_t*) rbuf) {
        printf("Error: Test 2.2.2 failed. Incorrect begin pointer position\n");
        exit(1);
    }

    memcpy(&read_len, ringbuffer_context->begin + msg_len + sizeof(size_t), sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 2.2.2 failed. Incorrect message length\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->begin + msg_len + 2 * sizeof(size_t), msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.2.2 failed. Incorrect message\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.2.2 passed\n");

    /* TEST 2.2.3: wrapping of prefix */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += rbuf_size - sizeof(size_t); // create enough space between read and write pointer for the message
    ringbuffer_context->write += rbuf_size - sizeof(size_t) + 1; // this induces wrapping of the prefix

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.2.3 failed. Incorrect return value\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + msg_len + 1) {
        printf("Error: Test 2.2.3 failed. Incorrect write pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + rbuf_size - sizeof(size_t)) {
        printf("Error: Test 2.2.3 failed. Incorrect read pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->begin != (uint8_t*) rbuf) {
        printf("Error: Test 2.2.3 failed. Incorrect begin pointer position\n");
        exit(1);
    }

    memcpy(&read_len, ringbuffer_context->begin + rbuf_size - sizeof(size_t) + 1, sizeof(size_t) - 1);
    memcpy(((uint8_t*) &read_len) + sizeof(size_t) - 1, ringbuffer_context->begin, 1);
    if (read_len != msg_len) {
        printf("Error: Test 2.2.3 failed. Incorrect message length\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->begin + 1, msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.2.3 failed. Incorrect message\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.2.3 passed\n");

    /* TEST 2.2.3.2: wrapping of prefix (extra test) */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += rbuf_size - sizeof(size_t); // create enough space between read and write pointer for the message
    ringbuffer_context->write += rbuf_size - 1; // this induces wrapping of the prefix

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.2.3.2 failed. Incorrect return value\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + msg_len + sizeof(size_t) - 1) {
        printf("Error: Test 2.2.3.2 failed. Incorrect write pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + rbuf_size - sizeof(size_t)) {
        printf("Error: Test 2.2.3.2 failed. Incorrect read pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->begin != (uint8_t*) rbuf) {
        printf("Error: Test 2.2.3.2 failed. Incorrect begin pointer position\n");
        exit(1);
    }

    memcpy(&read_len, ringbuffer_context->begin + rbuf_size - 1, 1);
    memcpy(((uint8_t*) &read_len) + 1, ringbuffer_context->begin, sizeof(size_t) - 1);
    if (read_len != msg_len) {
        printf("Error: Test 2.2.3.2 failed. Incorrect message length\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->begin + sizeof(size_t) - 1, msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.2.3.2 failed. Incorrect message\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.2.3.2 passed\n");

    /* TEST 2.2.4: wrapping of message */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += rbuf_size - sizeof(size_t) - 2; // create enough space between read and write pointer for the message
    ringbuffer_context->write += rbuf_size - sizeof(size_t) - 1; // this induces wrapping of the message (but not the prefix)

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.2.4 failed. Incorrect return value\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + msg_len - 1) {
        printf("Error: Test 2.2.4 failed. Incorrect write pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + rbuf_size - sizeof(size_t) - 2) {
        printf("Error: Test 2.2.4 failed. Incorrect read pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->begin != (uint8_t*) rbuf) {
        printf("Error: Test 2.2.4 failed. Incorrect begin pointer position\n");
        exit(1);
    }

    memcpy(&read_len, ringbuffer_context->begin + rbuf_size - sizeof(size_t) - 1, sizeof(size_t));

    if (read_len != msg_len) {
        printf("Error: Test 2.2.4 failed. Incorrect message length\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->begin + rbuf_size - 1, 1);
    memcpy(read_msg + 1, ringbuffer_context->begin, msg_len - 1);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.2.4 failed. Incorrect message\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.2.4 passed\n");

    /* TEST 2.3: write pointer < read pointer */

    /* TEST 2.3.1: no wrapping (it is not possible to have wrapping in this case) */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += msg_len + sizeof(size_t) + 2; // create enough space between read and write pointer for the message
    ringbuffer_context->write += 1; 

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.3.1 failed. Incorrect return value\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + msg_len + sizeof(size_t) + 1) {
        printf("Error: Test 2.3.1 failed. Incorrect write pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + msg_len + sizeof(size_t) + 2) {
        printf("Error: Test 2.3.1 failed. Incorrect read pointer position\n");
        exit(1);
    }

    if (ringbuffer_context->begin != (uint8_t*) rbuf) {
        printf("Error: Test 2.3.1 failed. Incorrect begin pointer position\n");
        exit(1);
    }

    memcpy(&read_len, ringbuffer_context->begin + 1, sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 2.3.1 failed. Incorrect message length\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->begin + sizeof(size_t) + 1, msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.3.1 failed. Incorrect message\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.3.1 passed\n");


    free(ringbuffer_context);

    printf("--------------------------------------------------------\n");
    printf("All tests passed\n");



}