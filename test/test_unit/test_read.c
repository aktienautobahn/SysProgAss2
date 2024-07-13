#include <stdio.h>
#include <stdlib.h>

#include "../include/ringbuf.h"

int main() {
    // printf("Remove the following line, only after the unit tests for write are working.\n");
    // printf("exit(1);\n");
    // exit(1); // remove me (this line will not be present in our tests, don't worry)

    rbctx_t *ringbuffer_context = malloc(sizeof(rbctx_t));
    if (ringbuffer_context == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }  

    char msg[] = "This is a test message that is too big for the buffer.";
    size_t msg_len = strlen(msg) + 1; 


    /*************************************************************************
     * TEST 1:                                                               *
     * Try to read from an empty buffer
     *************************************************************************/
    printf("--------------------------------------------------------\n");
    printf("Test 1: Try to read from an empty buffer\n");

    size_t rbuf_size = msg_len - 1; // rbuf size is irrelevant for this test
    char* rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }
    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);

    char buffer[100]; // specific size is irrelevant for this test
    size_t buffer_len = 100;

    if (ringbuffer_read(ringbuffer_context, buffer, &buffer_len) != RINGBUFFER_EMPTY) {
        printf("Error: Test 1.1 failed. Expected RINGBUFFER_EMPTY\n");
        exit(1);
    }

    printf("  + Test 1.1 passed\n");

    ringbuffer_context->read += sizeof(size_t);
    ringbuffer_context->write += sizeof(size_t);

    if (ringbuffer_read(ringbuffer_context, buffer, &buffer_len) != RINGBUFFER_EMPTY) {
        printf("Error: Test 1.2 failed. Expected RINGBUFFER_EMPTY\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 1.2 passed\n");

    /*************************************************************************
     * TEST 2:                                                               *
     * Try to read a message that is too big for the buffer                  *
     *************************************************************************/

    printf("--------------------------------------------------------\n");
    printf("Test 2: Try to read a message that is too big for the buffer\n");

    /* TEST 2.1: read pointer < write pointer */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    
    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + sizeof(size_t) + msg_len) {
        printf("Error: Test 2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf || ringbuffer_context->read != ringbuffer_context->begin) {
        printf("Error: Test 2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    size_t read_len;
    memcpy(&read_len, ringbuffer_context->read, sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    char read_msg[msg_len];
    memcpy(read_msg, ringbuffer_context->read + sizeof(size_t), msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    char buffer_too_small[msg_len - 1];
    buffer_len = msg_len - 1;

    if (ringbuffer_read(ringbuffer_context, buffer_too_small, &buffer_len) != OUTPUT_BUFFER_TOO_SMALL) {
        printf("Error: Test 2.1.1 failed. Expected OUTPUT_BUFFER_TOO_SMALL\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.1 passed\n");

    /* TEST 2.2: read pointer > write pointer */
    rbuf_size = 3 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->write = (uint8_t*) rbuf + msg_len + sizeof(size_t);
    
    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + 2 * (msg_len + sizeof(size_t))) {
        printf("Error: Test 2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }


    if (ringbuffer_context->read != (uint8_t*) rbuf || ringbuffer_context->read != ringbuffer_context->begin) {
        printf("Error: Test 2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    memcpy(&read_len, ringbuffer_context->write - msg_len - sizeof(size_t), sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->write - msg_len, msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    
    ringbuffer_context->read = (uint8_t*) rbuf + msg_len + sizeof(size_t); // simulate that read pointer is there where we wrote the message
    ringbuffer_context->write = (uint8_t*) rbuf; // advance write pointer to the beginning (simulate wrap around)

    buffer_len = msg_len - 1;

    if (ringbuffer_read(ringbuffer_context, buffer_too_small, &buffer_len) != OUTPUT_BUFFER_TOO_SMALL) {
        printf("Error: Test 2.2 failed. Expected OUTPUT_BUFFER_TOO_SMALL\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 2.2 passed\n");


    /*************************************************************************
     * TEST 3:                                                               *
     * Try to read a message that fits the buffer                            *
    *************************************************************************/

    printf("--------------------------------------------------------\n");
    printf("Test 3: Try to read a message that fits the buffer\n");

    /* TEST 3.1: read pointer < write pointer */
    rbuf_size = 2 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 3.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + sizeof(size_t) + msg_len) {
        printf("Error: Test 3.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf || ringbuffer_context->read != ringbuffer_context->begin) {
        printf("Error: Test 3.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    memcpy(&read_len, ringbuffer_context->read, sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 3.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->read + sizeof(size_t), msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 3.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    buffer_len = msg_len;

    if (ringbuffer_read(ringbuffer_context, buffer, &buffer_len) != SUCCESS) {
        printf("Error: Test 3.1 failed. Expected SUCCESS\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + sizeof(size_t) + msg_len) {
        printf("Error: Test 3.1 failed. Read pointer at wrong position\n");
        exit(1);
    }

    if (strcmp(buffer, msg) != 0) {
        printf("Error: Test 3.1 failed. Incorrect message read\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 3.1 passed\n");


    /* TEST 3.2: read pointer > write pointer */

    /* TEST 3.2.1: prefix wrapped */
    rbuf_size = 3 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read = (uint8_t*) rbuf + 3 * (msg_len + sizeof(size_t)) - sizeof(size_t); // ensure that there is enough space to write the message
    ringbuffer_context->write = (uint8_t*) rbuf + 3 * (msg_len + sizeof(size_t)) - sizeof(size_t) + 1; // this enduces a wrap around of the prefix

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 3.2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + msg_len + 1) {
        printf("Error: Test 3.2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + 3 * (msg_len + sizeof(size_t)) - sizeof(size_t)) {
        printf("Error: Test 3.2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    ringbuffer_context->read = (uint8_t*) rbuf + 3 * (msg_len + sizeof(size_t)) - sizeof(size_t) + 1; // simulate that read pointer is there where we wrote the message
    memcpy(&read_len, ringbuffer_context->read, sizeof(size_t) - 1);
    memcpy(((uint8_t*) &read_len) + sizeof(size_t) - 1, rbuf, 1);

    if (read_len != msg_len) {
        printf("Error: Test 3.2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    memcpy(read_msg, (uint8_t*) rbuf + 1, msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 3.2.1 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    buffer_len = msg_len;

    if (ringbuffer_read(ringbuffer_context, buffer, &buffer_len) != SUCCESS) {
        printf("Error: Test 3.2.1 failed. Expected SUCCESS\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + msg_len + 1) {
        printf("Error: Test 3.2.1 failed. Read pointer at wrong position\n");
        exit(1);
    }

    if (strcmp(buffer, msg) != 0) {
        printf("Error: Test 3.2.1 failed. Incorrect message read\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 3.2.1 passed\n");

    /* TEST 3.2.2: message wrapped */
    rbuf_size = 3 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read = (uint8_t*) rbuf + rbuf_size - 2 * sizeof(size_t) - 1; // ensure that there is enough space to write the message
    ringbuffer_context->write = (uint8_t*) rbuf + rbuf_size - 2 * sizeof(size_t); // this enduces a wrap around of the message

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 3.2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + msg_len - sizeof(size_t)) {
        printf("Error: Test 3.2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + rbuf_size - 2 * sizeof(size_t) - 1) {
        printf("Error: Test 3.2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    ringbuffer_context->read = (uint8_t*) rbuf + rbuf_size - 2 * sizeof(size_t); // simulate that read pointer is there where we wrote the message
    memcpy(&read_len, ringbuffer_context->read, sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 3.2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->read + sizeof(size_t), sizeof(size_t));
    memcpy(read_msg + sizeof(size_t), rbuf, msg_len - sizeof(size_t));

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 3.2.2 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    buffer_len = msg_len;

    if (ringbuffer_read(ringbuffer_context, buffer, &buffer_len) != SUCCESS) {
        printf("Error: Test 3.2.2 failed. Expected SUCCESS\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + msg_len - sizeof(size_t)) {
        printf("Error: Test 3.2.2 failed. Read pointer at wrong position\n");
        exit(1);
    }

    if (strcmp(buffer, msg) != 0) {
        printf("Error: Test 3.2.2 failed. Incorrect message read\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 3.2.2 passed\n");

    /* TEST 3.2.3: no wrap around */
    rbuf_size = 3 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += 2 * (msg_len + sizeof(size_t)) - 2; // ensure that there is enough space to write the message
    ringbuffer_context->write += 2 * (msg_len + sizeof(size_t)) - 1; // this enduces a wrap around of the message

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 3.2.3 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf + rbuf_size - 1) {
        printf("Error: Test 3.2.3 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + 2 * (msg_len + sizeof(size_t)) - 2) {
        printf("Error: Test 3.2.3 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    ringbuffer_context->read = (uint8_t*) rbuf + 2 * (msg_len + sizeof(size_t)) - 1; // simulate that read pointer is there where we wrote the message

    memcpy(&read_len, ringbuffer_context->read, sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 3.2.3 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->read + sizeof(size_t), msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 3.2.3 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    buffer_len = msg_len;

    if (ringbuffer_read(ringbuffer_context, buffer, &buffer_len) != SUCCESS) {
        printf("Error: Test 3.2.3 failed. Expected SUCCESS\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + rbuf_size - 1) {
        printf("Error: Test 3.2.3 failed. Read pointer at wrong position\n");
        exit(1);
    }

    if (strcmp(buffer, msg) != 0) {
        printf("Error: Test 3.2.3 failed. Incorrect message read\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 3.2.3 passed\n");

    /* TEST 3.2.4: no wrap around (extra case) */
    rbuf_size = 3 * (msg_len + sizeof(size_t)); // generally message could fit
    rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        printf("Error: malloc failed\n");
        exit(1);
    }

    ringbuffer_init(ringbuffer_context, rbuf, rbuf_size);
    ringbuffer_context->read += 2 * (msg_len + sizeof(size_t)) - 1; // ensure that there is enough space to write the message
    ringbuffer_context->write += 2 * (msg_len + sizeof(size_t)); // this enduces a wrap around of the message

    if (ringbuffer_write(ringbuffer_context, msg, msg_len) != SUCCESS) {
        printf("Error: Test 3.2.4 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->write != (uint8_t*) rbuf) {
        printf("Error: Test 3.2.4 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf + 2 * (msg_len + sizeof(size_t)) - 1) {
        printf("Error: Test 3.2.4 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    ringbuffer_context->read = (uint8_t*) rbuf + 2 * (msg_len + sizeof(size_t)); // simulate that read pointer is there where we wrote the message

    memcpy(&read_len, ringbuffer_context->read, sizeof(size_t));
    if (read_len != msg_len) {
        printf("Error: Test 3.2.4 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    memcpy(read_msg, ringbuffer_context->read + sizeof(size_t), msg_len);

    if (strcmp(read_msg, msg) != 0) {
        printf("Error: Test 3.2.4 failed. Write failed. Fix write first.\n");
        exit(1);
    }

    buffer_len = msg_len;

    if (ringbuffer_read(ringbuffer_context, buffer, &buffer_len) != SUCCESS) {
        printf("Error: Test 3.2.4 failed. Expected SUCCESS\n");
        exit(1);
    }

    if (ringbuffer_context->read != (uint8_t*) rbuf) {
        printf("Error: Test 3.2.4 failed. Read pointer not wrapped around\n");
        exit(1);
    }

    if (strcmp(buffer, msg) != 0) {
        printf("Error: Test 3.2.4 failed. Incorrect message read\n");
        exit(1);
    }

    free(rbuf);

    printf("  + Test 3.2.4 passed\n");

    free(ringbuffer_context);

    printf("--------------------------------------------------------\n");
    printf("All tests passed\n");

}